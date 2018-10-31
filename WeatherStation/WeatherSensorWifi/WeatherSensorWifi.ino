/*
 Name:		WeatherSensorWifi.ino
 Created:	10/26/2018 5:32:48 PM
 Author:	Günter
*/

#include <Wire.h>         // i²c-Schnittstelle
#include <EEPROM.h>		  // non-volatile memory
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

// #define DEBUG
// #define SLEEP

#ifdef DEBUG
#define SECONDS_BETWEEN_TICKS 10
#define MAX_TICKS_BETWEEN_TRANSMISSION 2
#else
#define SECONDS_BETWEEN_TICKS 60
#define MAX_TICKS_BETWEEN_TRANSMISSION 10
#endif

#define SENSOR_POWER D6
#define SENSOR_CLOCK 5
#define SENSOR_DATA 4

const int samplesForPressure = 4;
#pragma endregion

#define NO_SENSOR_DATA 200

struct {
	int ticksSinceLastTransmission = 0;
	float temperature = NO_SENSOR_DATA;
	boolean temperatureHasChanged = false;
	float pressure = NO_SENSOR_DATA;
	boolean pressureHasChanged = false;
	float humidity = NO_SENSOR_DATA;
	boolean humidityHasChanged = false;
} State;

#pragma region global objects
AS_BH1750 bh1750;         // Helligkeitssensor
BMP180 bmp180;            // Temperatur- und Druck-Sensor
DHT dht(D7, DHT22);        // Temperatur- und Feuchte-Sensor
WiFiClient client;
#pragma endregion

// the setup function runs once when you press reset or power the board
void setup() {
	Serial.begin(115200);
#ifdef SLEEP
	loop();
	Serial.printf("Going into deep sleep for %d seconds\r\n", SECONDS_BETWEEN_TICKS);
	SaveValuesToEeprom();
	ESP.deepSleep(SECONDS_BETWEEN_TICKS * 1e6); // 1e6 is *10^6 microseconds = secs
#endif
	// nach dem sleep beginnt wieder setup()
}

// the loop function runs over and over again until power down or reset
void loop() {
	pinMode(SENSOR_POWER, OUTPUT);
	digitalWrite(SENSOR_POWER, HIGH); // power up temperature sensor
	//bh1750.begin();
	//bmp180.init(SENSOR_DATA, SENSOR_CLOCK); // später, damit die richtigen Pins für i²c gesetzt sind
	dht.begin();
	LoadValuesFromEeprom();
	GetSensorData();
	pinMode(SENSOR_POWER, LOW);
	State.ticksSinceLastTransmission++;
	boolean mustTransmit = State.ticksSinceLastTransmission > MAX_TICKS_BETWEEN_TRANSMISSION;

	if (DataChanged() || mustTransmit) {
		if (WiFiStart()) {
			ReportData(mustTransmit);
		}
	}
#ifndef SLEEP
	delay(delay_ms);
#endif
}

void GetSensorData() {
	float value;
	value = readTemperatureFromDHT22();
	if (value != State.temperature) {
		State.temperature = value;
		State.temperatureHasChanged = true;
	}
	value = readHumidity();
	if (value != State.humidity) {
		State.humidity = value;
		State.humidityHasChanged = true;
	}
	//value = readPressure();
	//SendData("pressure", value);
	//value = readBrightness();
	//SendData("brightness", value);
}

boolean DataChanged() {
	return State.humidityHasChanged || State.pressureHasChanged || State.temperatureHasChanged;
}

void ReportData(boolean force) {
	if ((force || State.temperatureHasChanged) && State.temperature != NO_SENSOR_DATA) {
		SendData("temperature", State.temperature);
		State.temperatureHasChanged = false;
	}
	if ((force || State.pressureHasChanged) && State.pressure != NO_SENSOR_DATA) {
		SendData("pressure", State.pressure);
		State.pressureHasChanged = false;
	}
	if ((force || State.humidityHasChanged) && State.humidity != NO_SENSOR_DATA) {
		SendData("humidity", State.humidity);
		State.humidityHasChanged = false;
	}
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
		Serial.print(".");
		delay(1000);
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

void LoadValuesFromEeprom()
{
	EEPROM.begin(sizeof(State));
	EEPROM.get(0, State);
}

void SaveValuesToEeprom()
{
	EEPROM.put(0, State);
	EEPROM.commit();
}

