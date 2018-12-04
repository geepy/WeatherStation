/*
 Name:		WeatherSensorWifi.ino
 Created:	10/26/2018 5:32:48 PM
 Author:	Günter
*/

// #define LOG
// #define DEBUG
#define SLEEP
#define HAS_BME280
//#define HAS_DHT22
//#define HAS_BMP180
//#define HAS_BH1750

#define MY_SENSOR_ID 1

#if MY_SENSOR_ID == 0
#define MY_HOSTNAME "drinnen"
#elif MY_SENSOR_ID == 1
#define MY_HOSTNAME "draussen"
#elif MY_SENSOR_ID == 2
#define MY_HOSTNAME "schildkroeten"
#endif 


#include <SPI.h>
#include <Wire.h>         // i²c-Schnittstelle
#include <EEPROM.h>		  // non-volatile memory
#ifdef HAS_BME280
#include "BME280.h"
#endif
#ifdef HAS_BMP180
#include "BMP180.h"       // patched Temperatur-/Luftdruck-Sensor
#endif
#ifdef HAS_DHT22
#include "dht.h"	      // patched Temperatur-/Feuchtigkeit-Sensor
#endif
#ifdef HAS_BH1750
#include <AS_BH1750.h>    // Helligkeitssensor
#endif

#include <ESP8266WiFi.h>
#include "WifiIdentifier.h"

#pragma region global constants

const char* ssid = WIFI_SSID;
const char* password = WIFI_PWD;
const char* servername = "wetterstation";

#ifdef DEBUG
#define SECONDS_BETWEEN_TICKS 10
#define MAX_TICKS_BETWEEN_TRANSMISSION 2
#else
#define SECONDS_BETWEEN_TICKS 180
#define MAX_TICKS_BETWEEN_TRANSMISSION 10
#endif

#define delay_ms (SECONDS_BETWEEN_TICKS * 1000)


#define SENSOR_CLOCK D1
#define SENSOR_DATA D2

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
	float voltage = NO_SENSOR_DATA;
} State;

#pragma region global objects
#ifdef HAS_BH1750
AS_BH1750 bh1750;							// Helligkeitssensor i²c 23
#endif
#ifdef HAS_BME280
BME280I2C::Settings sensorSettings;
BME280I2C bme280(sensorSettings);           // Temperatur-, Druck- und Feuchtigkeits-Sensor i²c 76
#endif
#ifdef HAS_BMP180
BMP180 bmp180;								// Temperatur und Druck-Sensor auf i²c 77
#endif
#ifdef HAS_DHT22
DHT dht(D5, DHT22);							// Temperatur- und Feuchte-Sensor SPI
#endif

WiFiClient client;
IPAddress myIp(192, 168, 178, 200 + MY_SENSOR_ID);
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
	Wire.begin(D2, D1); // Ports setzen - BME-Treiber macht das nicht...
#ifdef SLEEP
	pinMode(LED_BUILTIN, OUTPUT);
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
	InitSensors();
	LoadValuesFromEeprom();
	GetSensorData();
	State.ticksSinceLastTransmission++;
	boolean mustTransmit = State.ticksSinceLastTransmission > MAX_TICKS_BETWEEN_TRANSMISSION;

	if (DataChanged() || mustTransmit) {
		long startTime = millis();
		if (WiFiStart()) {
			ReportData();
			WiFiStop();
		}
#ifdef LOG
		Serial.printf("\r\n Transmission took %ld milliseconds\r\n", millis() - startTime);
#endif
	}
#ifndef SLEEP
	delay(delay_ms);
#endif
}

void InitSensors() {
	if (!bme280.begin()) {
		LogText("cannot initialize BME280.");
	}
	else {
		DumpText("found chip, it is ");
		DumpValue((int)bme280.chipID());
	}

#ifdef HAS_BH1750
	bh1750.begin();
#endif
#ifdef HAS_BMP180
	bmp180.init(SENSOR_DATA, SENSOR_CLOCK); // später, damit die richtigen Pins für i²c gesetzt sind
#endif
#ifdef HAS_DHT22
	dht.begin();
#endif
}

void GetSensorData() {
	float value;
#ifdef HAS_BME280
	DumpText("reading temperature [BME]: ");
	value = readTemperaturefromBME280();
	DumpValue(value);
	if (value != State.temperature) {
		State.temperature = value;
		State.temperatureHasChanged = true;
	}
#endif

#ifdef HAS_BMP180
	DumpText("reading temperature [BMP]: ");
	value = readTemperaturefromBMP180();
	DumpValue(value);
	if (value != State.temperature) {
		State.temperature = value;
		State.temperatureHasChanged = true;
	}
#endif
#ifdef HAS_DHT22
	DumpText("reading temperature [DHT]: ");
	value = readTemperatureFromDHT22();
	DumpValue(value);
	if (value != State.temperature) {
		State.temperature = value;
		State.temperatureHasChanged = true;
	}
#endif
	delay(1000); // dont ask sensor more often than every 2 seconds
#ifdef HAS_BME280
	DumpText("reading humidity [BME]: ");
	value = readHumidityfromBME280();
	DumpValue(value);
	if (value != State.humidity) {
		State.humidity = value;
		State.humidityHasChanged = true;
	}
#endif

#ifdef HAS_DHT22
	DumpText("reading humidity [DHT]: ");
	value = readHumidityfromDHT22();
	DumpValue(value);
	if (value != State.humidity) {
		State.humidity = value;
		State.humidityHasChanged = true;
	}
#endif
#ifdef HAS_BME280
	DumpText("reading pressure [BME]: ");
	value = readPressurefromBME280();
	DumpValue(value);
	if (value != State.pressure) {
		State.pressure = value;
		State.pressureHasChanged = true;
	}
#endif
#ifdef HAS_BMP180
	DumpText("reading pressure [BMP]: ");
	value = readPressurefromBMP180();
	DumpValue(value);
	if (value != State.pressure) {
		State.pressure = value;
		State.pressureHasChanged = true;
	}
#endif

	DumpText("reading Voltage: ");
	value = ESP.getVcc();
	DumpValue(value);
	// value += 300; // NodeMCU Amica
	value += 230; // LOLIN D1 mini
	DumpValue(value);
	if (abs(State.voltage - value) > 10) {
		State.voltage = value / 1000.;
	}
}

boolean DataChanged() {
	return State.humidityHasChanged 
		|| State.pressureHasChanged 
		|| State.temperatureHasChanged;
}

void ReportData() {
	if (State.temperature != NO_SENSOR_DATA) {
		SendData("temperature", State.temperature);
		State.temperatureHasChanged = false;
	}
#if defined(HAS_BME280) || defined(HAS_BMP180)
	if (State.pressure != NO_SENSOR_DATA) {
		SendData("pressure", State.pressure);
		State.pressureHasChanged = false;
	}
#endif
#if defined(HAS_BME280) || defined(HAS_DHT22)
	if (State.humidity != NO_SENSOR_DATA) {
		SendData("humidity", State.humidity);
		State.humidityHasChanged = false;
	}
#endif
	SendData("voltage", State.voltage);
}

void SendData(const char* type, float value) {
	long startTime = millis();
	for (int i = 0; i < 5; i++) {
		if (client.connect(hostIp, 80)) {
			String data = String("PUT /sensordata/") + MY_SENSOR_ID + "/" + type + "/" + String(value, 4) + " HTTP/1.1";
			LogText(data.c_str());
			client.println(data);
			client.print(String("Host: ") + servername + "\r\n");
			client.print("Connection: close\r\n\r\n");
			long timestamp = millis();
			do
			{
				if (millis() - timestamp > 1000) {
					LogText(" -> no response from server, aborting.");
					break;
				}
				delay(10);
			} while (client.connected());
			if (client.available())
			{
				String line = client.readStringUntil('\n');
				LogText("Response:");
				LogText(line.c_str());
			}
			break;
		}
		DumpText(" ->no connection made, retrying");
		delay(1000);
	}
	client.stop();
	LogText("Transmission ended in [ms]");
	LogValue(millis() - startTime);
}

//////////////////
// Connect to WiFi
//////////////////
bool WiFiStart()
{
	long startTime = millis();
	// Connect to WiFi network
#ifdef LOG
	Serial.printf("Connecting to '%s' ", WIFI_SSID);
#endif
	WiFi.forceSleepWake();
	WiFi.persistent(false); // don't read or write configuration to flash
	WiFi.mode(WIFI_STA);
	WiFi.config(myIp, gatewayIp, subnetMaskIp);
	WiFi.hostname(MY_HOSTNAME);
	WiFi.begin(WIFI_SSID, WIFI_PWD);
	for (int retry = 0; retry < 20; retry++) {
		if (WiFi.status() == WL_CONNECTED)
		{
			LogText("connected on IP ");
			LogText(WiFi.localIP().toString().c_str());
			return true;
		}
		Serial.print(".");
		delay(1000);
	}
	LogText("No Wifi connection possible");
	return false;
}

void WiFiStop()
{
	WiFi.disconnect(true);
	WiFi.forceSleepBegin();
}

#ifdef HAS_BMP180
float readTemperaturefromBMP180()
{
	long ut = bmp180.measureTemperature();
	int t = bmp180.compensateTemperature(ut);
	return bmp180.formatTemperature(t);
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
#endif

#ifdef HAS_BME280
float readTemperaturefromBME280()
{
	return bme280.temp();
}

float readPressurefromBME280()
{
	return bme280.pres() / 100.;
}

float readHumidityfromBME280()
{
	return bme280.hum();
}
#endif

#ifdef HAS_DHT22
float readTemperatureFromDHT22() {
	return dht.readTemperature();
}

float readHumidityfromDHT22()
{
	return dht.readHumidity();
}
#endif

#ifdef HAS_BH1750
float readBrightness()
{
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
	char buffer[10];
	dtostrf(value, 5, 2, buffer);
	Serial.println(buffer);
#endif
}

void LogText(const char* text) {
#ifdef LOG
	Serial.println(text);
#endif
}

void DumpValue(float value) {
#ifdef DEBUG
	char buffer[10];
	dtostrf(value, 5, 2, buffer);
	Serial.println(buffer);
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

