// Load Wi-Fi library
#include <ESP8266WiFi.h>
#include "WifiIdentifier.h"

#include "SensorData.h"
#include "time_ntp.h"


// Replace with your network credentials
const char* ssid = WIFI_SSID;
const char* password = WIFI_PWD;
const char* hostname = "Wetterstation";

// Set web server port number to 80
WiFiServer server(80);

// Variable to store the HTTP request
String header;

const char* SENSORDATA_HEADER = "/sensordata/";
// storage
const char* SENSOR0_NAME = "Drinnen";
const char* SENSOR1_NAME = "Drau&szlig;en";
const char* SENSOR2_NAME = "Schildkr&ouml;ten";
SensorData sensors[3];

NTP* ntp = new NTP();
unsigned long timeOffset = 0;

void setup() {
	Serial.begin(115200);

	sensors[0].SensorName = SENSOR0_NAME;
	sensors[1].SensorName = SENSOR1_NAME;
	sensors[2].SensorName = SENSOR2_NAME;
	// Connect to Wi-Fi network with SSID and password
	ConnectToWifi();
	GetCurrentTime();
}

void loop() {
	WiFiClient client = server.available();   // Listen for incoming clients

	if (client) {                             // If a new client connects,
		Serial.println("New Client.");          // print a message out in the serial port
		String method = "";
		boolean hasMethod = false;
		String stem = "";
		boolean hasUri = false;
		String currentLine = "";                // make a String to hold incoming data from the client
		long timestamp = millis();
		while (client.connected()) {            // loop while the client's connected
			if (millis() - timestamp > 5000) {
				Serial.println("->client not responsive, closing connection");
				break;
			}
			if (client.available()) {             // if there's bytes to read from the client,
				char c = client.read();             // read a byte, then
				Serial.write(c);                    // print it out the serial monitor
				header += c;
				if (c == '\n') {            // if the byte is a newline character
					if (!hasUri) {
						method = header.substring(0, header.indexOf(" "));
						stem = header.substring(method.length() + 1, header.indexOf(" ", method.length() + 1));
						hasUri = true;			
					}
					// if the current line is blank, you got two newline characters in a row.
					// that's the end of the client HTTP request, so send a response:
					if (currentLine.length() == 0) {
						stem.toLowerCase();

						if (stem.indexOf(SENSORDATA_HEADER) >= 0) {
							Serial.print("receiving sensor data: ");
							Serial.println(stem);
							if (!ProcessSensorData(stem.substring(strlen(SENSORDATA_HEADER))))
							{
								client.println("HTTP/1.1 500 BAD URI");
								break;
							}
							else {
								RenderDataReceived(client);
								break;
							}
						}
						RenderPage(&client);
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
		stem = "";
		// Close the connection
		client.stop();
		Serial.println("Client disconnected.");
		Serial.println("");
	}
}

bool ProcessSensorData(String uri)
{
	Serial.print("..processing sensor data ");
	Serial.println(uri);
	int offset = uri.indexOf("/");
	if (offset < 1) {
		Serial.println("..illegal stem");
		return false;
	}
	int sensorId = 0;
	if (offset == 1) {
		sensorId = uri[0] - '0';
	}
	// else search for sensorName and optionally create an index for it
	else {
		Serial.println("..symbolic sensor name not supported yet");
		return false;
	}
	if (sensorId < 0 || sensorId > 2) {
		Serial.println("..illegal sensor id");
		return false;
	}
	uri = uri.substring(offset + 1);
	offset = uri.indexOf("/");
	if (offset < 1) {
		Serial.println("..sensor type not found");
		return false;
	}
	String sensorType = uri.substring(0, offset);
	Serial.print("..found sensor type ");
	Serial.println(sensorType);
	uri = uri.substring(offset + 1);
	float sensorValue = uri.toFloat();
	if (uri == "nan") {
		// ignore, keep last value
	}
	else if (sensorType.indexOf("temp") == 0) {
		sensors[sensorId].Temperature = sensorValue;
		sensors[sensorId].timestamp = millis() / 1000 + timeOffset;
	}
	else if (sensorType.indexOf("press") == 0) {
		sensors[sensorId].Pressure = sensorValue;
		sensors[sensorId].timestamp = millis() / 1000 + timeOffset;
	}
	else if (sensorType.indexOf("humid") == 0) {
		sensors[sensorId].Humidity = sensorValue;
		sensors[sensorId].timestamp = millis() / 1000 + timeOffset;
	}
	else if (sensorType.indexOf("bright") == 0) {
		sensors[sensorId].Brightness = sensorValue;
		sensors[sensorId].timestamp = millis() / 1000 + timeOffset;
	}
	else {
		Serial.print("..unsupported sensor type: ");
		Serial.println(sensorType);
		return false;
	}
	sensors[sensorId].WriteToSerial();
	return true;
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

void RenderDataReceived(WiFiClient client) {
	client.println("HTTP/1.1 200 OK");
	client.println("Content-type:text/plain");
	client.println("Connection: close");
	client.println();
	client.println("data received");

	client.println();
}

void RenderPage(WiFiClient* client)
{
	client->println("HTTP/1.1 200 OK");
	client->println("Content-type:text/html");
	client->println("Connection: close");
	client->println();

	// Display the HTML web page
	client->println("<!DOCTYPE html><html>");
	client->println("<head>");
	client->println("<meta http-equiv='refresh' content='60'>");
	client->println("<meta name='viewport' content='width = 640, height=960,initial - scale = 1'>");
	client->println("<link href = \"data:image/x-icon;base64,AAABAAEAEBAQAAEABAAoAQAAFgAAACgAAAAQAAAAIAAAAAEABAAAAAAAgAAAAAAAAAAAAAAAEAAAAAAAAAAA0vkAAAAAAADX/wAAWPYAAFLnACsqKwAAWvwAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAEREREREREREREREWYRERERERZjFmFhEREREWNmZmFhERZmYCImZmEREWYgESJmEREWYiEiEiYWEWZiICIiJmYRYWIlIhImYREUQiIiIiYRERZmICIiZhYRFhZiIiZmYRERFmNmZmEREREWFmEWZhERERERZhERERERERERERERH//wAA/n8AAPEvAAD4CwAAwAMAAOAHAADABQAAgAEAAKADAADABwAAwAUAANADAADwDwAA9McAAP5/AAD//wAA\" rel = \"icon\" type = \"image/x-icon\" / >");
	client->println("<link href = \"data:image/x-icon;base64,AAABAAEAEBAQAAEABAAoAQAAFgAAACgAAAAQAAAAIAAAAAEABAAAAAAAgAAAAAAAAAAAAAAAEAAAAAAAAAAA0vkAAAAAAADX/wAAWPYAAFLnACsqKwAAWvwAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAEREREREREREREREWYRERERERZjFmFhEREREWNmZmFhERZmYCImZmEREWYgESJmEREWYiEiEiYWEWZiICIiJmYRYWIlIhImYREUQiIiIiYRERZmICIiZhYRFhZiIiZmYRERFmNmZmEREREWFmEWZhERERERZhERERERERERERERH//wAA/n8AAPEvAAD4CwAAwAMAAOAHAADABQAAgAEAAKADAADABwAAwAUAANADAADwDwAA9McAAP5/AAD//wAA\" rel = \"shortcut icon\" type = \"image/x-icon\" / >");
	// CSS 
	client->println("<style>");
	client->print("html { font-family: Helvetica; font-size:20px; display: inline-block; margin: 0px auto; text-align: center;} ");
	client->print("h3 { font-size:24pt; font-weight:bold; text-align:left; align:left;} ");
	client->print(".sensorType { font-size:24pt; font-weight:250; text-align:right; width:50 % ; padding-right:24px; }");
	client->print(".sensorValue { font-size:24pt; font-weight:100; text-align:left; }");
	client->println("</style>");
	client->println("</head>");

	// Web Page Heading
	client->println("<body><h1>Online-Wetterstation Lessingstra&szlig;e 36</h1>");

	WriteCurrentTime(client);
	
	WriteSensorData(client, &sensors[0]);
	WriteSensorData(client, &sensors[1]);
	WriteSensorData(client, &sensors[2]);

	client->println("</body></html>");

	// The HTTP response ends with another blank line
	client->println();
}

void WriteSensorData(WiFiClient* client, SensorData* sensor) {
	client->println("<h3>"); client->print(sensor->SensorName); client->print("</h3>");
	client->print("<table width='100%'>");
	WriteSensorValue(client, "Temperatur", sensor->Temperature, "Grad");
	WriteSensorValue(client, "Luftfeuchtigkeit", sensor->Humidity, "%");
	WriteSensorValue(client, "Luftdruck", sensor->Pressure, "hPa");
	WriteSensorValue(client, "Helligkeit", sensor->Brightness, "cd");
	String lastTime = ntp->GetTimeString(sensor->timestamp);
	client->printf("<tr><td>Letzter Kontakt</td><td>%s</td</tr>", lastTime.c_str());
	client->println("</table>");
}

void WriteSensorValue(WiFiClient* client, const char* description, float data, const char* suffix)
{
	char number_buffer[10];
	if (data != SENSORDATA_NO_DATA) {
		dtostrf(data, 6, 1, number_buffer);
		client->printf("<tr><td class='sensorType'>%s</td><td class='sensorValue'>%s %s</td></tr>", description, number_buffer, suffix);
	}
}

void GetCurrentTime()
{
	unsigned long currentTime = ntp->GetLocalTime(); // current time in seconds since 1.1.1970
	timeOffset = currentTime - (millis()/1000);
	Serial.printf("current epoch is %ld, millis is %ld, offset is %ld\n", currentTime, millis(), timeOffset);
}

void WriteCurrentTime(WiFiClient* client)
{
	Serial.print("TIME: es ist ");
	unsigned long epoch = millis()/1000 + timeOffset;
	Serial.print(epoch);
	Serial.print(" -> ");
	String s = ntp->GetTimeString(epoch);
	client->println(s);
	Serial.println(s);
}