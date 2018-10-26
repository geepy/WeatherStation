/*
 Name:		WeatherSensorWifi.ino
 Created:	10/26/2018 5:32:48 PM
 Author:	Günter
*/

#include <ESP8266WiFi.h>
#include "WifiIdentifier.h"

// Replace with your network credentials
const char* ssid = WIFI_SSID;
const char* password = WIFI_PWD;
const char* hostname = "Draussen";
const char* servername = "wetterstation";

const int delay_ms = 10000;

float temp = 10;

WiFiClient client;

// the setup function runs once when you press reset or power the board
void setup() {
	Serial.begin(115200);
}

// the loop function runs over and over again until power down or reset
void loop() {
	SendData();
	temp += 0.1;
	delay(delay_ms);
}

void SendData() {
	Serial.println("trying to connect");
	if (client.connect(servername, 80)) {
		char number_buffer[10];

		Serial.println("  ...connected, send data");
		dtostrf(temp, 6, 1, number_buffer);
		String value = String(number_buffer);
		value.trim();
		String data = String("GET /sensordata/1/temperature/")+value+" HTTP/1.1" ;
		Serial.println(data);
		client.println(data);
		client.print(String("Host: ") + servername + "\r\n");
		client.print("Connection: close\r\n\r\n");
		Serial.println("  ...awaiting response");
		while (client.connected())
		{
			if (client.available())
			{
				String line = client.readStringUntil('\n');
				Serial.print("  response: ");
				Serial.println(line);
			}
		}
	}
	client.stop();
}
