#include "Log.h"
#include <SPI.h>

#include <gfxfont.h>
#include <Adafruit_SPITFT_Macros.h>
#include <Adafruit_SPITFT.h>
#include <Adafruit_GFX.h>
#include <GxFont_GFX.h>
#include <GxEPD.h>

#include <GxGDEW027W3/GxGDEW027W3.h>      // 2.7" b/w
#include <GxIO/GxIO_SPI/GxIO_SPI.h>
#include <GxIO/GxIO.h>

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

NTP ntp = NTP();
unsigned long timeOffset = 0;
date_time_t lastTime;
date_time_t currentTime;

EpaperDisplay display = EpaperDisplay();

void setup()
{
	Serial.begin(115200);
	Serial.println();
	Serial.println("setup");
	display.init();
	display.drawFrames();
	WiFiStart();
	lastTime = ntp.GetTimeStruct(ntp.GetLocalTime());
	currentTime = lastTime;
	display.drawTimeAndDate(currentTime);

	SensorData state = SensorData();
	state.temperature = -45.67;
	state.humidity = 99.9;
	state.pressure = 1234;
	display.drawSensorData(0, state);
	display.drawSensorData(1, state);
	display.drawSensorData(2, state);

}

void loop()
{
	delay(1000);
	currentTime.second++;
	if (currentTime.second > 59) {
		currentTime.second = 0;
		currentTime.minute++;
		if (currentTime.minute > 59) {
			currentTime = ntp.GetTimeStruct(ntp.GetLocalTime());
		}
	}
	if (currentTime.minute != lastTime.minute) {
		lastTime = currentTime;
		display.drawTimeAndDate(currentTime);

		// refresh complete display every 10 minutes to avoid darkening
		if (currentTime.minute % 10 == 0) {
			display.update();
		}
	}
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
			LogText(String("WiFi connected on IP ") + WiFi.localIP());
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

