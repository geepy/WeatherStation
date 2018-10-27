/*
 Name:		WeatherSensorWifi.ino
 Created:	10/26/2018 5:32:48 PM
 Author:	Günter
*/

#include <Wire.h>         // i²c-Schnittstelle
#include "BMP180.h"       // patched Temperatur-/Luftdruck-Sensor
#include "dht.h"	      // patched Temperatur-/Feuchtigkeit-Sensor
#include <AS_BH1750.h>    // Helligkeitssensor
#include <ESP8266WiFi.h>
#include "WifiIdentifier.h"

#pragma region global constants

// Replace with your network credentials
const char* ssid = WIFI_SSID;
const char* password = WIFI_PWD;
const char* hostname = "Draussen";
const char* servername = "wetterstation";

const int delay_ms = 10000;

const int SENSOR_POWER = D5;
#define SENSOR_CLOCK 5
#define SENSOR_DATA 4

const int samplesForPressure = 4;
#pragma endregion

float temp = 10;

#pragma region global objects
AS_BH1750 bh1750;         // Helligkeitssensor
BMP180 bmp180;            // Temperatur- und Druck-Sensor
DHT dht(D7, DHT22);        // Temperatur- und Feuchte-Sensor
WiFiClient client;
#pragma endregion

// the setup function runs once when you press reset or power the board
void setup() {
	Serial.begin(115200);
	bh1750.begin();
	bmp180.init(SENSOR_DATA, SENSOR_CLOCK); // später, damit die richtigen Pins für i²c gesetzt sind
	dht.begin();
	if (WiFiStart()) {
		RunOneLoop();
	}
	Serial.println("Going into deep sleep for 20 seconds");
	ESP.deepSleep(20e6); // 20e6 is 20*10^6 microseconds = 20 secs
	// nach dem sleep beginnt wieder setup()
}

// the loop function runs over and over again until power down or reset
void loop() {
	RunOneLoop();
	delay(delay_ms);
}

void RunOneLoop() {
	float value;
	value = readTemperatureFromDHT22();
	SendData("temperature", value);
	//value = readPressure();
	//SendData("pressure", value);
	value = readHumidity();
	SendData("humidity", value);
	value = readBrightness();
	SendData("brightness", value);
}

void SendData(const char* type, float value) {
	Serial.println("trying to connect");
	for (int i = 0; i < 5; i++) {
		if (client.connect(servername, 80)) {
			char number_buffer[10];

			Serial.println("  ...connected, send data");
			dtostrf(value, 6, 1, number_buffer);
			String valueString = String(number_buffer);
			valueString.trim();
			String data = String("GET /sensordata/1/") + type + "/" + valueString + " HTTP/1.1";		Serial.println(data);
			client.println(data);
			client.print(String("Host: ") + servername + "\r\n");
			client.print("Connection: close\r\n\r\n");
			Serial.println("  ...awaiting response");
			long timestamp = millis();
			while (client.connected())
			{
				if (millis() - timestamp > 1000) {
					Serial.println("->no response from server, aborting.");
					break;
				}
				if (client.available())
				{
					String line = client.readStringUntil('\n');
					Serial.print("  response: ");
					Serial.println(line);
				}
			}
			break;
		}
		Serial.println("  ->no connection made, retrying");
		delay(3000);
	}
	client.stop();
}

	//////////////////
	// Connect to WiFi
	//////////////////
	bool WiFiStart()
	{
		// Connect to WiFi network
		Serial.print("Connecting to ");
		Serial.println(WIFI_SSID);
		WiFi.begin(WIFI_SSID, WIFI_PWD);
		for (int retry = 0; retry < 20; retry++) {
			if (WiFi.status() == WL_CONNECTED)
			{
				Serial.println("WiFi connected");

				// Start the server
				Serial.print("Server started - IP: ");
				Serial.println(WiFi.localIP());
				return true;
			}
		}
		Serial.println("No Wifi connection possible");
		return false;
	}
float readTemperaturefromBMP180()
{
	long ut = bmp180.measureTemperature();
	int t = bmp180.compensateTemperature(ut);
	return bmp180.formatTemperature(t);
}

float readTemperatureFromDHT22() {
	return dht.readTemperature();
}

float readPressure()
{
	byte samplingMode = BMP180_OVERSAMPLING_HIGH_RESOLUTION;
	long up;

	long p = 0;
	for (int i = 0; i < samplesForPressure; i++) {
		up = bmp180.measurePressure(samplingMode);
		p += bmp180.compensatePressure(up, samplingMode);
	}
	p = p / samplesForPressure;

	return bmp180.formatPressure(p);
}

float readHumidity()
{
	return dht.readHumidity();
}

float readBrightness()
{
	return bh1750.readLightLevel();
}



/*******************************************/

uint8_t portArray[] = { 16, 5, 4, 0, 2, 14, 12, 13 };
//String portMap[] = {"D0", "D1", "D2", "D3", "D4", "D5", "D6", "D7"}; //for Wemos
String portMap[] = { "GPIO16", "GPIO5", "GPIO4", "GPIO0", "GPIO2", "GPIO14", "GPIO12", "GPIO13" };

void scanPorts() {
	for (uint8_t i = 0; i < sizeof(portArray); i++) {
		for (uint8_t j = 0; j < sizeof(portArray); j++) {
			if (i != j) {
				Serial.print("Scanning (SDA : SCL) - " + portMap[i] + " : " + portMap[j] + " - ");
				Wire.begin(portArray[i], portArray[j]);
				check_if_exist_I2C();
			}
		}
	}
}

void check_if_exist_I2C() {
	byte error, address;
	int nDevices;
	nDevices = 0;
	for (address = 1; address < 127; address++) {
		// The i2c_scanner uses the return value of
		// the Write.endTransmisstion to see if
		// a device did acknowledge to the address.
		Wire.beginTransmission(address);
		error = Wire.endTransmission();

		if (error == 0) {
			Serial.print("I2C device found at address 0x");
			if (address < 16)
				Serial.print("0");
			Serial.print(address, HEX);
			Serial.println("  !");

			nDevices++;
		}
		else if (error == 4) {
			Serial.print("Unknow error at address 0x");
			if (address < 16)
				Serial.print("0");
			Serial.println(address, HEX);
		}
	} //for loop
	if (nDevices == 0)
		Serial.println("No I2C devices found");
	else
		Serial.println("**********************************\n");
	//delay(1000);           // wait 1 seconds for next scan, did not find it necessary
}
