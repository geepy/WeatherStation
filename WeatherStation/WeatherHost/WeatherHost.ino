// Load Wi-Fi library
#include <SPI.h>
#include <RH_ASK.h>
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

struct UriData {
	int sensorId;
	int sensorType;
	float sensorValue;
};

// forward declarations
bool ProcessSensorData(UriData& data);
void ProcessSerialSensorData(const unsigned char* buf);
void RenderSensorData(WiFiClient client, UriData& data);
bool ParseUri(String& uri, UriData& result);

const char* SENSORDATA_HEADER = "sensordata";
// storage
#define SENSOR0_NAME "Drinnen"
#define SENSOR1_NAME "Drau&szlig;en"
#define SENSOR2_NAME "Schildkr&ouml;ten"
SensorData sensors[3];

NTP* ntp = new NTP();
unsigned long timeOffset = 0;

// init for 5kBaud and RX on pin 4
RH_ASK driver(2000, D6, D7, 0);

void setup() {
	Serial.begin(115200);

	sensors[0].SensorName = SENSOR0_NAME;
	sensors[1].SensorName = SENSOR1_NAME;
	sensors[2].SensorName = SENSOR2_NAME;
	// Connect to Wi-Fi network with SSID and password
	if (!driver.init())
		Serial.println("init of RF receiver failed");
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
						UriData data;
						if (ParseUri(stem, data)) {
							if (method == "PUT") {

								if (!ProcessSensorData(data))
								{
									client.println("HTTP/1.1 500 BAD URI");
								}
								else {
									RenderDataReceived(client);
								}
							}
							else {
								RenderSensorData(client, data);
							}
							break;
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
	uint8_t buf[RH_ASK_MAX_MESSAGE_LEN];
	uint8_t buflen = sizeof(buf);

	if (driver.recv(buf, &buflen)) // Non-blocking
	{
		buf[buflen] = 0;
		driver.printBuffer("raw serial data:", buf, buflen);
		ProcessSerialSensorData(buf);
	}
}

bool ParseUri(String& uri, UriData& result) {
	uri.toLowerCase();
	if (uri.indexOf(SENSORDATA_HEADER) < 0) {
		Serial.println("no sensor data");
		return false;
	}
	// /SENSORDATA/
	int offset = uri.indexOf("/", 1) + 1; // don't count leading '/'
	if (offset < 1) {
		Serial.println("..illegal stem");
		return false;
	}
	// /SENSORDATA/{nameOrId}/
	int nextOffset = uri.indexOf("/", offset) + 1;
	int sensorId = 0;
	if (nextOffset == offset + 2) {
		result.sensorId = uri[offset] - '0';
	}
	// else search for sensorName and optionally create an index for it
	else {
		Serial.println("..symbolic sensor name not supported yet");
		return false;
	}
	if (sensorId < 0 || sensorId > 3) {
		Serial.println("..illegal sensor id");
		return false;
	}
	offset = nextOffset;
	// /SENSORDATA/{nameOrId}/{type}/
	nextOffset = uri.indexOf("/", offset) + 1;
	if (offset < 1) {
		Serial.println("..sensor type not found");
		return false;
	}
	if (nextOffset < 1) {
		nextOffset = uri.length();
	}
	String sensorType = uri.substring(offset, nextOffset - 1);
	Serial.print("..found sensor type ");
	Serial.println(sensorType);
	if (sensorType.startsWith("temp"))
	{
		result.sensorType = SENSORTYPE_TEMPERATURE;
	}
	else if (sensorType.startsWith("press")) {
		result.sensorType = SENSORTYPE_PRESSURE;
	}
	else if (sensorType.startsWith("hum")) {
		result.sensorType = SENSORTYPE_HUMIDITY;
	}
	else if (sensorType.startsWith("bright")) {
		result.sensorType = SENSORTYPE_BRIGHTNESS;
	}
	else if (sensorType.startsWith("volt")) {
		result.sensorType = SENSORTYPE_VOLTAGE;
	}
	else {
		Serial.printf("unknown sensor type '%s'\n", sensorType.c_str());
		return false;
	}
	if (uri.length() > nextOffset && !uri.endsWith("nan")) {
		result.sensorValue = uri.substring(nextOffset).toFloat();
	}
	else {
		result.sensorValue = NAN;
	}
	return true;
}

bool ProcessSensorData(UriData& data)
{
	if (data.sensorValue > (data.sensorType == SENSORTYPE_PRESSURE ? 1500 : 110) || data.sensorValue < -50) {
		data.sensorValue = SENSORDATA_NO_DATA;
	}
	sensors[data.sensorId].Value[data.sensorType] = data.sensorValue;
	sensors[data.sensorId].timestamp = millis() / 1000 + timeOffset;
	sensors[data.sensorId].WriteToSerial();
	return true;
}

void ProcessSerialSensorData(const unsigned char* buf) {
	char sensorType = buf[2];
	if (buf[1] == ':' && buf[3] == ':') {
		int sensorId = buf[0] - '0';
		if (sensorId < 0 || sensorId > 2) {
			Serial.println("..illegal sensor id");
		}
		else {
			sensors[sensorId].timestamp = millis() / 1000 + timeOffset;
			String sensorData = String((char*)(buf + 4));
			float sensorValue = sensorData.toFloat();
			Serial.printf("got serial data from sensor %d of type %c with value ", sensorId, sensorType);
			dtostrf(sensorValue, 8, 1, (char*)buf);
			Serial.println((char*)buf);
			switch (sensorType) {
			case 'V':
				sensors[sensorId].Value[SENSORTYPE_VOLTAGE] = sensorValue;
				break;
			case 'T':
				sensors[sensorId].Value[SENSORTYPE_TEMPERATURE] = sensorValue;
				break;
			case 'H':
				sensors[sensorId].Value[SENSORTYPE_HUMIDITY] = sensorValue;
				break;
			}
		}
	}
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



void RenderSensorData(WiFiClient client, UriData& data) {
	client.println("HTTP/1.1 200 OK");
	client.println("Content-type:text/plain");
	client.println("Connection: close");
	client.println();
	client.println(String(sensors[data.sensorId].Value[data.sensorType], 1));
	client.println();
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
	client->print(".sensorReadings { font-size:24pt; font-weight:250; width:50 % ; padding-right:12px; padding-left:12px; }");
	client->print(".sensor { background-color:#f8f8f8; }");
	client->print(".smaller { font-size:18pt; }");
	client->print(".footnote { font-size:12pt; }");
	client->print(".left { text-align:right; }");
	client->print(".right { text-align:left; }");
	client->print(".footnote { font-size:12pt; }");
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
	client->println("<div class='sensor'><h3>"); client->print(sensor->SensorName); client->print("</h3>");
	client->print("<table width='100%'>");
	WriteSensorValue(client, "Temperatur", sensor->Value[SENSORTYPE_TEMPERATURE], 1, "Grad");
	WriteSensorValue(client, "Luftfeuchtigkeit", sensor->Value[SENSORTYPE_HUMIDITY], 0, "%");
	WriteSensorValue(client, "Luftdruck", sensor->Value[SENSORTYPE_PRESSURE], 0, "hPa");
	WriteSensorValue(client, "Helligkeit", sensor->Value[SENSORTYPE_BRIGHTNESS], 0, "cd");
	WriteSensorValue(client, "Spannung", sensor->Value[SENSORTYPE_VOLTAGE], 2, "V");
	client->printf("<tr><td class='footnote left'>Letzter Kontakt vor </td><td class='footnote right'>%s</td</tr>", TimeDifferenceToNow(sensor->timestamp).c_str());
	client->println("</table></div>");
}

void WriteSensorValue(WiFiClient* client, const char* description, float data, int precision, const char* suffix)
{
	char number_buffer[10];
	if (data != SENSORDATA_NO_DATA) {
		dtostrf(data, 6, precision, number_buffer);
		for (int i = 0; i < sizeof(number_buffer); i++) {
			if (number_buffer[i] == '.') {
				number_buffer[i] = ',';
			}
		}
		client->printf("<tr><td class='sensorReadings left'>%s</td><td class='sensorReadings right'>%s %s</td></tr>", description, number_buffer, suffix);
	}
}

void GetCurrentTime()
{
	unsigned long currentTime = ntp->GetLocalTime(); // current time in seconds since 1.1.1970
	timeOffset = currentTime - (millis() / 1000);
	Serial.printf("current epoch is %ld, millis is %ld, offset is %ld\n", currentTime, millis(), timeOffset);
}

void WriteCurrentTime(WiFiClient* client)
{
	Serial.print("TIME: es ist ");
	unsigned long epoch = millis() / 1000 + timeOffset;
	Serial.print(epoch);
	Serial.print(" -> ");
	String s = ntp->GetTimeString(epoch);
	client->println(s);
	Serial.println(s);
}

String TimeDifferenceToNow(unsigned long then)
{
	unsigned int seconds = (millis() / 1000 + timeOffset) - then;
	if (seconds < 0) { return "noch nie"; }
	String result = "";
	if (seconds < 60) {
		result += seconds;
		result += " Sekunden";
	}
	else {
		seconds /= 60;
		if (seconds > 60) {
			result += (seconds / 60);
			result += " Stunden und ";
		}
		result += (seconds % 60);
		result += " Minuten";
	}
	return result;
}