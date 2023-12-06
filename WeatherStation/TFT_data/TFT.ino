// setup for TFT driver

#include <SPI.h>
#include <TFT_eSPI.h>
#include <GxFont_GFX.h>
#include <GxEPD.h>
#include <ESP8266WiFi.h>

#include "WifiIdentifier.h"
#include "time_ntp.h"
#include "SensorData.h"
#include "display.h"
#include "Log.h"

#define MY_HOSTNAME "Wetterdisplay"

#define LOG


WiFiClient client;
IPAddress myIp(192, 168, 178, 100);
IPAddress subnetMaskIp(255, 255, 255, 0);
IPAddress gatewayIp(192, 168, 178, 1);
IPAddress hostIp(192, 168, 178, 27);
PROGMEM const char* servername = "wetterstation";

NTP ntp = NTP();
unsigned long timeOffset = 0;
date_time_t lastTime;
date_time_t currentTime;
SensorData* state[3];

TftDisplay display = TftDisplay();

void GetData(SensorData& state);
float GetData(int8_t id, const char* type);

unsigned long lastMinute;

void setup()
{
	state[0] = new SensorData();
	state[1] = new SensorData();
	state[2] = new SensorData();

	Serial.begin(115200);
	Serial.println();
	Serial.println("setup");
	display.init();
	display.drawFrames();
	WiFiStart();
	client.setTimeout(300);
	lastTime = ntp.GetTimeStruct(ntp.GetLocalTime());
	currentTime = lastTime;
	lastMinute = millis();
	display.drawTimeAndDate(currentTime);

	DisplaySensorData(0);
	DisplaySensorData(1);
	DisplaySensorData(2);
}

bool gotTime = false;

void loop()
{
	// only process data every minute
	// since processing may take up to a minute, a delay() is no solution
	if (millis() - lastMinute < 1000)
		return;
	lastMinute += 1000;

	currentTime.second++;
	if (currentTime.second > 59) {
		currentTime.second = 0;
		currentTime.minute++;
		if (!gotTime || currentTime.minute > 59) {
			gotTime = false;
			unsigned long newTime = ntp.GetLocalTime();
			if (newTime > 1000000) {
				currentTime = ntp.GetTimeStruct(newTime);
				gotTime = true;
			}
		}
		lastTime = currentTime;
		display.drawTimeAndDate(currentTime);
		DisplaySensorData(0);
		DisplaySensorData(1);
		DisplaySensorData(2);

		// refresh complete display every 10 minutes to avoid darkening
		if (currentTime.minute % 20 == 0) {
			display.update();
		}
	}
}

void DisplaySensorData(uint8_t index) {
	SensorData newState = SensorData();
	bool hasChanges = false;
	GetData(index, newState);
	if (newState.temperature != state[index]->temperature) {
		hasChanges = true;
		state[index]->temperature = newState.temperature;
	}
	if (newState.humidity != state[index]->humidity) {
		hasChanges = true;
		state[index]->humidity = newState.humidity;
	}
	if (newState.pressure != state[index]->pressure) {
		hasChanges = true;
		state[index]->pressure = newState.pressure;
	}
	if (hasChanges)
		display.drawSensorData(index, *(state[index]));
}

void GetData(int8_t id, SensorData& state) {
	state.temperature = GetData(id, "temperature");
	state.pressure = GetData(id, "pressure");
	state.humidity = GetData(id, "humidity");
	state.voltage = GetData(id, "voltage");
}

float GetData(int8_t id, const char* type) {
	float result = 12.34;
	char number_buffer[20];

	LogText("trying to connect");
	if (client.connect(hostIp, 80)) {
		LogText("  ...connected, load data");

		String data = String("GET /sensordata/") + String(id) + "/" + type + " HTTP/1.1";
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
				String response = client.readString();
				int beginOfContent = response.indexOf("\n\r\n") + 2;
				result = response.substring(beginOfContent).toFloat();
				break;
			}
		}
		client.stop();
	}
	else
	{
		LogText("  no connection made");
	}
	return result;
}

//////////////////
// Connect to WiFi
//////////////////
bool WiFiStart()
{
	// Connect to WiFi network
	LogText(String("Connecting to ") + WIFI_SSID);
	WiFi.mode(WIFI_STA);
	WiFi.config(myIp, gatewayIp, subnetMaskIp);
	WiFi.hostname(MY_HOSTNAME);
	WiFi.begin(WIFI_SSID, WIFI_PWD);
	for (int retry = 0; retry < 20; retry++) {
		if (WiFi.status() == WL_CONNECTED)
		{
			LogText(String("WiFi connected on IP ") + WiFi.localIP().toString());
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

