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

const char* SENSORDATA_HEADER = "get /sensordata/";
// storage
SensorData sensors[3];

void setup() {
	Serial.begin(115200);
	// Initialize the output variables as outputs
	pinMode(output0, OUTPUT);
	pinMode(output5, OUTPUT);
	pinMode(output4, OUTPUT);
	// Set outputs to LOW
	digitalWrite(output0, LOW);
	digitalWrite(output5, LOW);
	digitalWrite(output4, LOW);

	// Connect to Wi-Fi network with SSID and password
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

void loop() {
	WiFiClient client = server.available();   // Listen for incoming clients

	if (client) {                             // If a new client connects,
		Serial.println("New Client.");          // print a message out in the serial port
		String currentLine = "";                // make a String to hold incoming data from the client
		while (client.connected()) {            // loop while the client's connected
			if (client.available()) {             // if there's bytes to read from the client,
				char c = client.read();             // read a byte, then
				Serial.write(c);                    // print it out the serial monitor
				header += c;
				if (c == '\n') {                    // if the byte is a newline character
				  // if the current line is blank, you got two newline characters in a row.
				  // that's the end of the client HTTP request, so send a response:
					if (currentLine.length() == 0) {
						// HTTP headers always start with a response code (e.g. HTTP/1.1 200 OK)
						// and a content-type so the client knows what's coming, then a blank line:
						client.println("HTTP/1.1 200 OK");
						client.println("Content-type:text/html");
						client.println("Connection: close");
						client.println();

						String lowercaseheader = header.substring(0); // copy
						lowercaseheader.toLowerCase();

						if (lowercaseheader.indexOf(SENSORDATA_HEADER) >= 0) {
							Serial.print("receiving sensor data: ");
							Serial.println(header);
							ProcessSensorData(header.substring(strlen(SENSORDATA_HEADER)));
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
	int sensorNameLength = uri.indexOf("/");
	if (sensorNameLength < 1) {
		Serial.println("..illegal uri");
		return;
	}
	int sensorId = 0;
	if (sensorNameLength == 1) {
		int sensorId = uri[0] - '0';
	}
	// else search for sensorName and optionally create an index for it
	else {
		Serial.println("..symbolic sensor name not supported yet");
		return;
	}
	uri = uri.substring(sensorNameLength + 1);
	int sensorTypeLength = uri.indexOf("/");
	if (sensorTypeLength < 1) {
		Serial.println("..sensor type not found");
		return;
	}
	String sensorType = uri.substring(0, sensorTypeLength);
	Serial.print("..found sensor type ");
	Serial.println(sensorType);
	if (sensorType.indexOf("temp") == 0) {

	}
}

void RenderPage(WiFiClient client)
{
	// Display the HTML web page
	client.println("<!DOCTYPE html><html>");
	client.println("<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">");
	client.println("<link rel=\"icon\" href=\"data:,\">");
	// CSS to style the on/off buttons 
	// Feel free to change the background-color and font-size attributes to fit your preferences
	client.println("<style>html { font-family: Helvetica; display: inline-block; margin: 0px auto; text-align: center;}");
	client.println(".button { background-color: #195B6A; border: none; color: white; padding: 16px 40px;");
	client.println("text-decoration: none; font-size: 30px; margin: 2px; cursor: pointer;}");
	client.println(".button2 {background-color: #77878A;}</style></head>");

	// Web Page Heading
	client.println("<body><h1>ESP8266 Web Server</h1>");

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
	client.println("</body></html>");

	// The HTTP response ends with another blank line
	client.println();
}