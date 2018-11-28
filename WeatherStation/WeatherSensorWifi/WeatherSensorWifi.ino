/*
 Name:		WeatherSensorWifi.ino
 Created:	10/26/2018 5:32:48 PM
 Author:	Günter
*/

#include <SPI.h>
#include <BME280I2C.h>
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

#define delay_ms (90 * 1000)

const int FULL_VOLTAGE = 2800;
const int LOW_VOLTAGE = 2000;

#define LOG
#define DEBUG
#define SLEEP
#define HAS_PRESSURE_SENSOR

#ifdef DEBUG
#define SECONDS_BETWEEN_TICKS 10
#define MAX_TICKS_BETWEEN_TRANSMISSION 2
#else
#define SECONDS_BETWEEN_TICKS 120
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
#ifdef HAS_PRESSURE_SENSOR
	float pressure = NO_SENSOR_DATA;
	boolean pressureHasChanged = false;
#endif
	float humidity = NO_SENSOR_DATA;
	boolean humidityHasChanged = false;
	float voltage = NO_SENSOR_DATA;
} State;

#pragma region global objects
AS_BH1750 bh1750;         // Helligkeitssensor
BME280I2C::Settings sensorSettings;
BME280I2C bme280(sensorSettings);            // Temperatur- und Druck-Sensor
BMP180 bmp180;
DHT dht(D7, DHT22);        // Temperatur- und Feuchte-Sensor
WiFiClient client;
IPAddress myIp(192, 168, 178, 25);
IPAddress subnetMaskIp(255,255,255,0);
IPAddress gatewayIp(192, 168, 178, 1);
IPAddress hostIp(192, 168, 178, 27); 
ADC_MODE(ADC_VCC);//Set the ADCC to read supply voltage.
#pragma endregion

// the setup function runs once when you press reset or power the board
void setup() {
#ifdef LOG
	Serial.begin(115200);
#endif
	Wire.begin(D2, D1);
#ifdef SLEEP
	digitalWrite(LED_BUILTIN, HIGH);
	loop();
	digitalWrite(LED_BUILTIN, LOW);
#ifdef LOG
	Serial.printf("Going into deep sleep for %d seconds\r\n", SECONDS_BETWEEN_TICKS);
	Serial.flush();
	Serial.end();
#endif
	SaveValuesToEeprom();
	ESP.deepSleep(SECONDS_BETWEEN_TICKS * 1e6); // 1e6 is *10^6 microseconds = secs
#endif
	// nach dem sleep beginnt wieder setup()
}

// the loop function runs over and over again until power down or reset
void loop() {
	pinMode(SENSOR_POWER, OUTPUT);
	digitalWrite(SENSOR_POWER, HIGH); // power up temperature sensor
	delay(1000); // wait until working
	//bh1750.begin();
	//bmp180.init(SENSOR_DATA, SENSOR_CLOCK); // später, damit die richtigen Pins für i²c gesetzt sind
	dht.begin();
	LoadValuesFromEeprom();
	GetSensorData();
	digitalWrite(SENSOR_POWER, LOW);
	State.ticksSinceLastTransmission++;
	boolean mustTransmit = State.ticksSinceLastTransmission > MAX_TICKS_BETWEEN_TRANSMISSION;

	if (DataChanged() || mustTransmit) {
		if (WiFiStart()) {
			ReportData();
			WiFiStop();
		}
	}
#ifndef SLEEP
	delay(delay_ms);
#endif
}

void GetSensorData() {
	float value;
	if (!bme280.begin()) {
#ifdef DEBUG
		Serial.println("cannot initialize BME280.");
#endif
	}
	else {
#ifdef DEBUG
		Serial.print("found chip, it is ");
		Serial.println((int)bme280.chipID());
#endif
	}

#ifdef DEBUG
	char buffer[10];
	Serial.print("reading temperature: ");
#endif
	value = readTemperaturefromBME280();
	if (value != State.temperature) {
		State.temperature = value;
		State.temperatureHasChanged = true;
	}
	delay(1000); // dont ask sensor more often than every 2 seconds
#ifdef DEBUG
	dtostrf(value, 5, 2, buffer);
	Serial.println(buffer);
	Serial.print("reading humidity: ");
#endif
	value = readHumidityfromBME280();
	if (value != State.humidity) {
		State.humidity = value;
		State.humidityHasChanged = true;
	}
#ifdef DEBUG
	dtostrf(value, 5, 2, buffer);
	Serial.println(buffer);
	Serial.print("reading pressure: ");
#endif
	value = readPressurefromBME280();
	if (value != State.pressure) {
		State.pressure = value;
		State.pressureHasChanged = true;
	}
#ifdef DEBUG
	dtostrf(value, 5, 2, buffer);
	Serial.println(buffer);
	Serial.print("reading Voltage: ");
#endif

	value = ESP.getVcc();
#ifdef DEBUG
	dtostrf(value, 5, 2, buffer);
	Serial.print(" raw:");
	Serial.print(buffer);
#endif
	// value += 300; // NodeMCU Amica
	value += 230; // LOLIN D1 mini
#ifdef DEBUG
	dtostrf(value, 5, 2, buffer);
	Serial.print(" normalized:");
	Serial.print(buffer);
#endif
	if (abs(State.voltage - value) > 10) {
		State.voltage = value / 1000.;
	}
}

boolean DataChanged() {
	return State.humidityHasChanged 
#ifdef HAS_PRESSURE_SENSOR
		|| State.pressureHasChanged 
#endif
		|| State.temperatureHasChanged;
}

void ReportData() {
	if (State.temperature != NO_SENSOR_DATA) {
		SendData("temperature", State.temperature);
		State.temperatureHasChanged = false;
	}
#ifdef HAS_PRESSURE_SENSOR
	if (State.pressure != NO_SENSOR_DATA) {
		SendData("pressure", State.pressure);
		State.pressureHasChanged = false;
	}
#endif
	if (State.humidity != NO_SENSOR_DATA) {
		SendData("humidity", State.humidity);
		State.humidityHasChanged = false;
	}
	SendData("Voltage", State.voltage);
}

void SendData(const char* type, float value) {
	Serial.println("trying to connect");
	for (int i = 0; i < 5; i++) {
		if (client.connect(hostIp, 80)) {
			char number_buffer[20];

			Serial.println("  ...connected, send data");
			dtostrf(value, -19, 4, number_buffer);
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
	WiFi.mode(WIFI_STA);
	WiFi.config(myIp, gatewayIp, subnetMaskIp);
	WiFi.begin(WIFI_SSID, WIFI_PWD);
	for (int retry = 0; retry < 20; retry++) {
		if (WiFi.status() == WL_CONNECTED)
		{
			Serial.print("WiFi connected on IP ");
			Serial.println(WiFi.localIP());
			return true;
		}
		Serial.print(".");
		delay(2000);
	}
	Serial.println("No Wifi connection possible");
	return false;
}

void WiFiStop()
{
	WiFi.disconnect(true);
}
float readTemperaturefromBMP180()
{
	long ut = bmp180.measureTemperature();
	int t = bmp180.compensateTemperature(ut);
	return bmp180.formatTemperature(t);
}

float readTemperaturefromBME280()
{
	return bme280.temp();
}

float readTemperatureFromDHT22() {
	return dht.readTemperature();
}

float readPressurefromBMP180()
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

float readPressurefromBME280()
{
	return bme280.pres()/100.;
}

float readHumidityfromDHT()
{
	return dht.readHumidity();
}

float readHumidityfromBME280()
{
	bme280.hum();
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

