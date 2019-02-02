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

#include "Open_Sans_Semibold_18.h"
#include "Open_Sans_Condensed_18.h"
#include "Open_Sans_Semibold_26.h"
#include "Open_Sans_Semibold_32.h"
#include "Open_Sans_SemiBold_36.h"
#include "Bitmaps.h"

#include <ESP8266WiFi.h>
#include "WifiIdentifier.h"
#include "time_ntp.h"
#include "SensorData.h"

#define MY_HOSTNAME "Wetterdisplay"

#define LOG


WiFiClient client;
IPAddress myIp(192, 168, 178, 100);
IPAddress subnetMaskIp(255, 255, 255, 0);
IPAddress gatewayIp(192, 168, 178, 1);
IPAddress hostIp(192, 168, 178, 27);

GxIO_Class io(SPI, /*CS=D8*/ SS, /*DC=D3*/ 0, /*RST=D4*/ 2); // arbitrary selection of D3(=0), D4(=2), selected for default of GxEPD_Class
GxEPD_Class display(io /*RST=D4*/ /*BUSY=D2*/); // default selection of D4(=2), D2(=4)

NTP ntp = NTP();
unsigned long timeOffset = 0;
date_time_t lastTime;
date_time_t currentTime;


void drawDate(int day, int month, int year, bool updateWindow = true);
void drawTime(int hours, int minutes, bool updateWindow = true);

void drawSensorData(int index, SensorData& state);

void setup()
{
	Serial.begin(115200);
	Serial.println();
	Serial.println("setup");
	display.init(115200); // enable diagnostic output on Serial
	display.setTextColor(GxEPD_BLACK);
	display.setRotation(2);
	drawFrames();
	WiFiStart();
	lastTime = ntp.GetTimeStruct(ntp.GetLocalTime());
	currentTime = lastTime;
	drawTimeAndDate();

	SensorData state = SensorData();
	state.temperature = -45.67;
	state.humidity = 99.9;
	state.pressure = 1234;
	drawSensorData(0, state);
	drawSensorData(1, state);
	drawSensorData(2, state);

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
		drawTimeAndDate();
	}
}

void drawRect(int x, int y, int w, int h) {
	display.drawRect(x, y, h, w, GxEPD_BLACK);
	display.drawRect(x + 1, y + 1, h - 2, w - 2, GxEPD_WHITE);
	display.drawRect(x + 2, y + 2, h - 4, w - 4, GxEPD_BLACK);
	display.updateWindow(x, y, h + 1, w + 1);
}
void drawFrames() {
	display.drawRect(0, 0, GxEPD_WIDTH, GxEPD_HEIGHT, GxEPD_BLACK);

	// oben links
	display.drawRect(110, 0, 66, 76, GxEPD_BLACK);
	// oben mitte
	display.drawRect(110, 75, 66, 114, GxEPD_BLACK);
	// oben rechts
	display.drawRect(110, 188, 66, 76, GxEPD_BLACK);
	// mitte links
	display.drawRect(0, 0, 111, 88, GxEPD_BLACK);
	// mitte mitte
	display.drawRect(0, 87, 111, 90, GxEPD_BLACK);
	// mitte rechts
	display.drawRect(0, 176, 111, 88, GxEPD_BLACK);
	display.update();
}

void drawText(const char *text, int y, int x, bool updateWindow = true)
{
	int16_t  x1, y1;
	uint16_t w, h;

	uint8_t oldRotation = display.getRotation();
	display.setRotation(3);

	LogText(String("drawing '") + text + "' to coordinates" + String(x) + " x " + String(y));
	display.setTextColor(GxEPD_BLACK);
	display.getTextBounds(text, x, 176 - y, &x1, &y1, &w, &h);
	display.setCursor(x, 176 - y);
	display.print(text);
	if (updateWindow) {
		LogText(String("  updating window (") + String(x1) + " x " + String(y1) + " size " + String(w) + " x " + String(h));
		display.updateWindow(x1, y1, w, h);
	}

	display.setRotation(oldRotation);
}

void drawTextRightAligned(const char *text, int y, int x, bool updateWindow = true)
{
	int16_t  x1, y1;
	uint16_t w, h;

	uint8_t oldRotation = display.getRotation();
	display.setRotation(3);

	LogText(String("drawing '") + text + "' right aligned to coordinates" + String(x) + " x " + String(y));
	display.setTextColor(GxEPD_BLACK);
	display.getTextBounds(text, 0, 176 - y, &x1, &y1, &w, &h);
	display.setCursor(x - w, 176 - y);
	display.print(text);
	if (updateWindow) {
		LogText(String("  updating window (") + String(x1) + " x " + String(y1) + " size " + String(w) + " x " + String(h));
		display.updateWindow(x + x1, y1, w, h);
	}

	display.setRotation(oldRotation);
}


void drawTime(int hours, int minutes, bool updateWindow)
{
	char buffer[10];
	display.setFont(&Open_Sans_SemiBold_38);
	sprintf(buffer, "%02d:%02d", hours, minutes);
	drawText(buffer, 116, 78, updateWindow);
}

void drawDate(int day, int month, int year, bool updateWindow)
{
	char buffer[12];
	display.setFont(&Open_Sans_SemiBold_18);
	sprintf(buffer, "%2d.%02d.%04d", day, month, year);
	drawText(buffer, 158, 78, updateWindow);
}

void drawTimeAndDate() {
	display.fillRect(111, 76, 64, 112, GxEPD_WHITE);
	drawTime(lastTime.hour, lastTime.minute, false);
	drawDate(lastTime.day, lastTime.month, lastTime.year, false);
	display.updateWindow(111, 76, 64, 112);
}

const unsigned char *Bmps[3] = { Bmp_Haus, Bmp_Baum, Bmp_Turtle };

void drawSensorData(int index, SensorData& data)
{
	int x1 = 1 + (88 * index),
		y1 = 1,
		w = 84,
		h = 102;

	int border = w - 24;
	display.fillRect(y1, x1, h, w, GxEPD_WHITE);
	display.drawBitmap(y1 + 84, x1 + 2, Bmps[index], 16, 16, GxEPD_BLACK);
	display.setFont(&Open_Sans_SemiBold_26);
	String value = String(data.temperature, 1);
	drawTextRightAligned(value.c_str(), 58, x1 + border + 1, false);
	drawText("C", 58, x1 + border + 4 , false);
	value = String(data.humidity, 1);
	drawTextRightAligned(value.c_str(), 28, x1 + border - 1, false);
	drawText("%", 28, x1 + border + 2, false);
	display.setFont(&Open_Sans_SemiBold_18);
	value = String(data.pressure, 0);
	drawTextRightAligned(value.c_str(), 4, x1 + border - 10, false);
	display.setFont(&Open_Sans_Condensed_Bold_18);
	drawText("hPa", 4, x1 + border -8, false);
	display.updateWindow(y1, x1, h, w);
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

