// Load Wi-Fi library
#include <ESP8266WiFi.h>
#include "WifiIdentifier.h"

#include "SensorData.h"


// Replace with your network credentials
const char* ssid = WIFI_SSID;
const char* password = WIFI_PWD;
const char* hostname = "Wetterstation";

// Set web server port number to 80
WiFiServer server(80);

// Variable to store the HTTP request
String header;

// Auxiliar variables to store the current output state
String output5State = "off";
String output4State = "off";

// Assign output variables to GPIO pins
const int output0 = D0;
const int output5 = D5;
const int output4 = D4;

const char* SENSORDATA_HEADER = "/sensordata/";
// storage
const char* SENSOR0_NAME = "Drinnen";
const char* SENSOR1_NAME = "Drau&szlig;en";
const char* SENSOR2_NAME = "Schildkr&ouml;ten";
SensorData sensors[3];

void setup() {
	Serial.begin(115200);

	sensors[0].SensorName = SENSOR0_NAME;
	sensors[1].SensorName = SENSOR1_NAME;
	sensors[2].SensorName = SENSOR2_NAME;
	// Connect to Wi-Fi network with SSID and password
	ConnectToWifi();
}

void loop() {
	WiFiClient client = server.available();   // Listen for incoming clients

	if (client) {                             // If a new client connects,
		Serial.println("New Client.");          // print a message out in the serial port
		String method = "";
		boolean hasMethod = false;
		String uri = "";
		boolean hasUri = false;
		String currentLine = "";                // make a String to hold incoming data from the client
		while (client.connected()) {            // loop while the client's connected
			if (client.available()) {             // if there's bytes to read from the client,
				char c = client.read();             // read a byte, then
				Serial.write(c);                    // print it out the serial monitor
				header += c;
				if (c == ' ') {
					if (!hasMethod) {
						hasMethod = true;
					}
					else if (!hasUri) {
						hasUri = true;
					}
				}
				else {
					if (!hasMethod) {
						method += c;
					} else
					if (!hasUri) {
						uri += c;
					}
				}
				if (c == '\n') {                    // if the byte is a newline character
					hasUri = true; // URI is only in the first line
				  // if the current line is blank, you got two newline characters in a row.
				  // that's the end of the client HTTP request, so send a response:
					if (currentLine.length() == 0) {
						// HTTP headers always start with a response code (e.g. HTTP/1.1 200 OK)
						// and a content-type so the client knows what's coming, then a blank line:
						client.println("HTTP/1.1 200 OK");
						client.println("Content-type:text/html");
						client.println("Connection: close");
						client.println();

						uri.toLowerCase();

						if (uri.indexOf(SENSORDATA_HEADER) >= 0) {
							Serial.print("receiving sensor data: ");
							Serial.println(uri);
							ProcessSensorData(uri.substring(strlen(SENSORDATA_HEADER)));
						}
						// turns the GPIOs on and off
						else if (header.indexOf("GET /5/on") >= 0) {
							Serial.println("GPIO 5 on");
							output5State = "on";
							digitalWrite(output5, HIGH);
						}
						else if (header.indexOf("GET /5/off") >= 0) {
							Serial.println("GPIO 5 off");
							output5State = "off";
							digitalWrite(output5, LOW);
						}
						else if (header.indexOf("GET /4/on") >= 0) {
							Serial.println("GPIO 4 on");
							output4State = "on";
							digitalWrite(output4, HIGH);
						}
						else if (header.indexOf("GET /4/off") >= 0) {
							Serial.println("GPIO 4 off");
							output4State = "off";
							digitalWrite(output4, LOW);
						}
						RenderPage(client);
						break;
					}
					else { // if you got a newline, then clear currentLine
						currentLine = "";
					}
				}
				else if (c != '\r') {  // if you got anything else but a carriage return character,
					currentLine += c;      // add it to the end of the currentLine
				}
			}
		}
		// Clear the header variable
		header = "";
		uri = "";
		// Close the connection
		client.stop();
		Serial.println("Client disconnected.");
		Serial.println("");
	}
}

void ProcessSensorData(String uri)
{
	Serial.print("..processing sensor data ");
	Serial.println(uri);
	int offset = uri.indexOf("/");
	if (offset < 1) {
		Serial.println("..illegal uri");
		return;
	}
	int sensorId = 0;
	if (offset == 1) {
		sensorId = uri[0] - '0';
	}
	// else search for sensorName and optionally create an index for it
	else {
		Serial.println("..symbolic sensor name not supported yet");
		return;
	}
	if (sensorId < 0 || sensorId > 2) {
		Serial.println("..illegal sensor id");
		return;
	}
	uri = uri.substring(offset + 1);
	offset = uri.indexOf("/");
	if (offset < 1) {
		Serial.println("..sensor type not found");
		return;
	}
	String sensorType = uri.substring(0, offset);
	Serial.print("..found sensor type ");
	Serial.println(sensorType);
	uri = uri.substring(offset + 1);
	float sensorValue = uri.toFloat();
	if (sensorType.indexOf("temp") == 0) {
		sensors[sensorId].Temperature = sensorValue;
	}
	else if (sensorType.indexOf("press") == 0) {
		sensors[sensorId].Pressure = sensorValue;
	}
	else if (sensorType.indexOf("humid") == 0) {
		sensors[sensorId].Humidity = sensorValue;
	}
	else if (sensorType.indexOf("bright") == 0) {
		sensors[sensorId].Brightness = sensorValue;
	}
	sensors[sensorId].WriteToSerial();
}

void ConnectToWifi()
{
	Serial.print("Connecting to ");
	Serial.println(ssid);
	WiFi.hostname(hostname);
	WiFi.begin(ssid, password);
	while (WiFi.status() != WL_CONNECTED) {
		delay(500);
		Serial.print(".");
	}

	// Print local IP address and start web server
	Serial.println("");
	Serial.println("WiFi connected.");
	Serial.println("IP address: ");
	Serial.println(WiFi.localIP());
	server.begin();
}
void RenderPage(WiFiClient client)
{
	// Display the HTML web page
	client.println("<!DOCTYPE html><html>");
	client.println("<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">");
	client.println("<link href = \"data:image/x-icon;base64,AAABAAEAEBAQAAEABAAoAQAAFgAAACgAAAAQAAAAIAAAAAEABAAAAAAAgAAAAAAAAAAAAAAAEAAAAAAAAAAA0vkAAAAAAADX/wAAWPYAAFLnACsqKwAAWvwAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAEREREREREREREREWYRERERERZjFmFhEREREWNmZmFhERZmYCImZmEREWYgESJmEREWYiEiEiYWEWZiICIiJmYRYWIlIhImYREUQiIiIiYRERZmICIiZhYRFhZiIiZmYRERFmNmZmEREREWFmEWZhERERERZhERERERERERERERH//wAA/n8AAPEvAAD4CwAAwAMAAOAHAADABQAAgAEAAKADAADABwAAwAUAANADAADwDwAA9McAAP5/AAD//wAA\" rel = \"icon\" type = \"image/x-icon\" / >");
	// CSS to style the on/off buttons 
	// Feel free to change the background-color and font-size attributes to fit your preferences
	client.println("<style>html { font-family: Helvetica; display: inline-block; margin: 0px auto; text-align: center;}");
	client.println(".button { background-color: #195B6A; border: none; color: white; padding: 16px 40px;");
	client.println("text-decoration: none; font-size: 30px; margin: 2px; cursor: pointer;}");
	client.println(".button2 {background-color: #77878A;}</style></head>");

	// Web Page Heading
	client.println("<body><h1>Online-Wetterstation Lessingstra&szlig;e 36</h1>");

	WriteSensorHeader(client);
	WriteSensorData(client, &sensors[0]);
	WriteSensorData(client, &sensors[1]);
	WriteSensorData(client, &sensors[2]);
	WriteSensorFooter(client);

	WriteGPIOStateAndButtons(client);

	client.println("</body></html>");

	// The HTTP response ends with another blank line
	client.println();
}

void WriteSensorHeader(WiFiClient client) {
	client.println("<table>");
	client.println("<thead><th></th><th>Temperatur</th><th>Luftfeuchtigkeit</th><th>Luftdruck</th><th>Helligkeit</th></thead>");
}

void WriteSensorFooter(WiFiClient client) {
	client.println("</table>"); 
}

void WriteSensorData(WiFiClient client, SensorData* sensor) {
	client.print("<tr>");
	client.print("<td><b>"); client.print(sensor->SensorName); client.print("</b></td>");
	WriteSensorValue(client, sensor->Temperature, "Grad");
	WriteSensorValue(client, sensor->Humidity, "%");
	WriteSensorValue(client, sensor->Pressure, "hPa");
	WriteSensorValue(client, sensor->Brightness, "cd");
	client.println("</tr>");
}

void WriteSensorValue(WiFiClient client, float data, const char* suffix)
{
	char number_buffer[10];
	client.print("<td>");
	if (data != SENSORDATA_NO_DATA) {
		dtostrf(data, 6, 1, number_buffer);
		client.print(number_buffer);
		client.print(" ");
		client.print(suffix);
	}
	else {
		client.print("(n/a)");
	}
	client.print("</td>");
}

void WriteGPIOStateAndButtons(WiFiClient client) {
	// Display current state, and ON/OFF buttons for GPIO 5  
	client.println("<p>GPIO 5 - State " + output5State + "</p>");
	// If the output5State is off, it displays the ON button       
	if (output5State == "off") {
		client.println("<p><a href=\"/5/on\"><button class=\"button\">ON</button></a></p>");
	}
	else {
		client.println("<p><a href=\"/5/off\"><button class=\"button button2\">OFF</button></a></p>");
	}

	// Display current state, and ON/OFF buttons for GPIO 4  
	client.println("<p>GPIO 4 - State " + output4State + "</p>");
	// If the output4State is off, it displays the ON button       
	if (output4State == "off") {
		client.println("<p><a href=\"/4/on\"><button class=\"button\">ON</button></a></p>");
	}
	else {
		client.println("<p><a href=\"/4/off\"><button class=\"button button2\">OFF</button></a></p>");
	}
}