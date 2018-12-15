/*
 Name:		WeatherSensorWifi.ino
 Created:	10/26/2018 5:32:48 PM
 Author:	Günter
*/

#pragma region switches

#define SENSORID 1

#define LOG
//#define DEBUG
#define SLEEP

#pragma endregion


#pragma region global constants

#if SENSORID==-1
#define MY_HOSTNAME "Testbed"
#define HAS_BME280
#define HAS_BMP180
#define HAS_BH1750
#define HAS_DHT22
#elif SENSORID==0
#define MY_HOSTNAME "drinnen"
#define HAS_BMP180
#elif SENSORID==1
#define MY_HOSTNAME "draussen"
#define HAS_BME280
#elif SENSORID==2
#define MY_HOSTNAME "Schildkroeten"
#define HAS_BME280
#endif

#include <SPI.h>
#include <Wire.h>         // i²c-Schnittstelle
#include <EEPROM.h>		  // non-volatile memory
#include <ESP8266WiFi.h>
#include "WifiIdentifier.h"

#ifdef HAS_BME280
#include "BME280.h"
#endif // HAS_BME280

#ifdef HAS_BMP180
#include "BMP180.h"       // patched Temperatur-/Luftdruck-Sensor
#endif // HAS_BME280
#ifdef HAS_DHT22
#include "dht.h"	      // patched Temperatur-/Feuchtigkeit-Sensor
#endif // HAS_BME280
#ifdef HAS_BH1750
#include <AS_BH1750.h>    // Helligkeitssensor
#endif // HAS_BME280

// Replace with your network credentials
const char* ssid = WIFI_SSID;
const char* password = WIFI_PWD;
const char* servername = "wetterstation";

#define delay_ms (90 * 1000)

const int FULL_VOLTAGE = 2800;
const int LOW_VOLTAGE = 2000;

#ifdef DEBUG
#define SECONDS_BETWEEN_TICKS 10
#else
#define SECONDS_BETWEEN_TICKS 120
#endif

#define SENSOR_POWER D6
#define SENSOR_CLOCK D1
#define SENSOR_DATA D2

const int samplesForPressure = 4;
#pragma endregion

#define NO_SENSOR_DATA 200

struct {
	float temperature = NO_SENSOR_DATA;
	float pressure = NO_SENSOR_DATA;
	float humidity = NO_SENSOR_DATA;
	float voltage = NO_SENSOR_DATA;
} State;

WiFiClient client;
IPAddress myIp(192, 168, 178, 100 + SENSORID);
IPAddress subnetMaskIp(255, 255, 255, 0);
IPAddress gatewayIp(192, 168, 178, 1);
IPAddress hostIp(192, 168, 178, 27);
ADC_MODE(ADC_VCC);//Set the ADCC to read supply voltage.
#pragma endregion

// the setup function runs once when you press reset or power the board
void setup() {
#ifdef LOG
	Serial.begin(115200);
#endif
#ifdef SLEEP
	digitalWrite(LED_BUILTIN, HIGH);
	loop();
	digitalWrite(LED_BUILTIN, LOW);
#ifdef LOG
	Serial.printf("Going into deep sleep for %d seconds\r\n", SECONDS_BETWEEN_TICKS);
	Serial.flush();
	Serial.end();
#endif
	ESP.deepSleep(SECONDS_BETWEEN_TICKS * 1e6); // 1e6 is *10^6 microseconds = secs
#endif
	// nach dem sleep beginnt wieder setup()
}

// the loop function runs over and over again until power down or reset
void loop() {
	pinMode(SENSOR_POWER, OUTPUT);
	digitalWrite(SENSOR_POWER, HIGH); // power up temperature sensor
	delay(1000); // wait until working
	GetSensorData();
	digitalWrite(SENSOR_POWER, LOW);

	{
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
#ifdef HAS_BME280
	GetSensorDataFromBME280();
#endif
#ifdef HAS_BMP180
	GetSensorDataFromBMP180();
#endif
#ifdef HAS_DHT22
	GetSensorDataFromDHT22();
#endif

	String debugString = String("reading Voltage: ");
	float value = ESP.getVcc();
	debugString += " raw:" + String(value, 3);
	// value += 300; // NodeMCU Amica
	value += 230; // LOLIN D1 mini
	debugString += " normalized:" + String(value, 3);
#ifdef DEBUG
	Serial.print(debugString);
#endif
	State.voltage = value / 1000.;
}

#ifdef HAS_BME280
void GetSensorDataFromBME280()
{
	BME280I2C::Settings sensorSettings;
	BME280I2C bme280(sensorSettings);            // Temperatur- und Druck-Sensor

	float value;
	String debugString = String("getting sensor data: ");

	Wire.begin(SENSOR_DATA, SENSOR_CLOCK);

	if (!bme280.begin()) {
		debugString += "cannot initialize BME280.\n";
	}
	else {
		debugString += "found chip, it is " + String((int)bme280.chipID()) + "\n";
	}
	debugString += "reading temperature: ";
	value = bme280.temp();
	if (value != State.temperature) {
		State.temperature = value;
	}
	debugString += String(value, 3) + "\n";
	debugString += "reading humidity: ";
	value = bme280.hum();
	if (value != State.humidity) {
		State.humidity = value;
	}
	debugString += String(value, 3) + "\n";
	debugString += "reading pressure: ";
	value = bme280.pres() / 100.;
	if (value != State.pressure) {
		State.pressure = value;
	}
	debugString += String(value, 3);
#ifdef DEBUG
	Serial.println(debugString);
#endif
}
#endif

#ifdef HAS_BMP180
void GetSensorDataFromBMP180()
{
	float value;
	String debugString = String("getting sensor data from BMP180: ");

	BMP180 bmp180;

	bmp180.init(SENSOR_DATA, SENSOR_CLOCK);
	debugString += "reading temperature: ";
	long ut = bmp180.measureTemperature();
	int t = bmp180.compensateTemperature(ut);
	value = t / 100.;
	if (value != State.temperature) {
		State.temperature = value;
	}
	debugString += String(value, 3) + "\n";
	debugString += "reading pressure: ";

	byte samplingMode = BMP180_OVERSAMPLING_HIGH_RESOLUTION;
	long up;

	long p = 0;
	for (int i = 0; i < samplesForPressure; i++) {
		up = bmp180.measurePressure(samplingMode);
		p += bmp180.compensatePressure(up, samplingMode);
	}
	p = p / samplesForPressure;

	value = bmp180.formatPressure(p);

	if (value != State.pressure) {
		State.pressure = value;
	}
	debugString += String(value, 3);
#ifdef DEBUG
	Serial.println(debugString);
#endif
}
#endif

#ifdef HAS_DHT22
void GetSensorDataFromDHT22()
{
	float value;

	String debugString = String("getting sensor data from DHT22: ");

	DHT dht(D7, DHT22);        // Temperatur- und Feuchte-Sensor

	dht.begin();

	debugString += "reading temperature: ";
	value = dht.readTemperature();
	if (value != State.temperature) {
		State.temperature = value;
	}
	debugString += String(value, 3) + "\n";
	debugString += "reading humidity: ";
	delay(2000); // dont query sensor more than every 2 seconds
	value = dht.readHumidity();

	if (value != State.humidity) {
		State.humidity = value;
	}
	debugString += String(value, 3);
#ifdef DEBUG
	Serial.println(debugString);
#endif
}
#endif


void ReportData() {
	if (State.temperature != NO_SENSOR_DATA) {
		SendData("temperature", State.temperature);
	}
	if (State.pressure != NO_SENSOR_DATA) {
		SendData("pressure", State.pressure);
	}
	if (State.humidity != NO_SENSOR_DATA) {
		SendData("humidity", State.humidity);
	}
	SendData("Voltage", State.voltage);
}

void SendData(const char* type, float value) {
	LogText("trying to connect");
	for (int i = 0; i < 5; i++) {
		if (client.connect(hostIp, 80)) {
			char number_buffer[20];

			LogText("  ...connected, send data");
			String data = String("PUT /sensordata/") + String(SENSORID) + "/" + type + "/" + String(value, 2) + " HTTP/1.1";		
			LogText(data);
			client.println(data);
			client.print(String("Host: ") + servername + "\r\n");
			client.print("Connection: close\r\n\r\n");
			LogText("  ...awaiting response");
			long timestamp = millis();
			while (client.connected())
			{
				if (millis() - timestamp > 1000) {
					LogText("->no response from server, aborting.");
					break;
				}
				if (client.available())
				{
					String line = client.readStringUntil('\n');
					LogText(String("  response: ")+line);
				}
			}
			break;
		}
		LogText("  ->no connection made, retrying");
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
	LogText(String("Connecting to ")+WIFI_SSID);
	WiFi.mode(WIFI_STA);
	WiFi.config(myIp, gatewayIp, subnetMaskIp);
	WiFi.hostname(MY_HOSTNAME);
	WiFi.begin(WIFI_SSID, WIFI_PWD);
	for (int retry = 0; retry < 20; retry++) {
		if (WiFi.status() == WL_CONNECTED)
		{
			LogText(String("WiFi connected on IP ")+WiFi.localIP());
			return true;
		}
		LogText(".");
		delay(2000);
	}
	LogText("No Wifi connection possible");
	return false;
}

void WiFiStop()
{
	WiFi.disconnect(true);
}
float readTemperaturefromBMP180()
{
}

#ifdef HAS_BH1750
float readBrightness()
{
	AS_BH1750 bh1750;
	Wire.begin(SENSOR_DATA, SENSOR_CLOCK);
	bh1750.begin();
	return bh1750.readLightLevel();
}
#endif

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


void LogValue(float value) {
#ifdef LOG
	Serial.println(String(value, 3));
#endif
}

void LogText(const char* text) {
#ifdef LOG
	Serial.println(text);
#endif
}

void LogText(String& text) {
#ifdef LOG
	Serial.println(text);
#endif
}

void DumpValue(float value) {
#ifdef DEBUG
	Serial.println(String(value, 2));
#endif
}

void DumpValue(int value) {
#ifdef DEBUG
	Serial.println(value);
#endif
}

void DumpText(const char* text) {
#ifdef DEBUG
	Serial.println(text);
#endif
}

