// 
// 
// 

#include "arduino.h"
#include <stdio.h>
#include "SensorData.h"
#include <SPI.h>
#include <TFT_eSPI.h>

#include <GxFont_GFX.h>

#include "Open_Sans_Semibold_18.h"
#include "Open_Sans_Condensed_18.h"
#include "Open_Sans_Semibold_26.h"
#include "Open_Sans_Semibold_32.h"
#include "Open_Sans_SemiBold_36.h"
#include "Bitmaps.h"

#include "display.h"
#include "time_ntp.h"
#include "Log.h"



TftDisplay::TftDisplay() {
	_display = TFT_eSPI();

}

void TftDisplay::init() {
	display->init(); 
	display->setTextColor(TFT_WHITE);
	display->setRotation(2);
}
void TftDisplay::update() {
	// display->update();
}
void TftDisplay::drawRect(int x, int y, int w, int h) {
	display->drawRect(x, y, h, w, GxEPD_BLACK);
	display->drawRect(x + 1, y + 1, h - 2, w - 2, GxEPD_WHITE);
	display->drawRect(x + 2, y + 2, h - 4, w - 4, GxEPD_BLACK);
}

void TftDisplay::drawFrames() {
	display->drawRect(0, 0, TFT_WIDTH, TFT_HEIGHT, GxEPD_BLACK);

	// oben links
	display->drawRect(110, 0, 66, 76, GxEPD_BLACK);
	// oben mitte
	display->drawRect(110, 75, 66, 114, GxEPD_BLACK);
	// oben rechts
	display->drawRect(110, 188, 66, 76, GxEPD_BLACK);
	// mitte links
	display->drawRect(0, 0, 111, 88, GxEPD_BLACK);
	// mitte mitte
	display->drawRect(0, 87, 111, 90, GxEPD_BLACK);
	// mitte rechts
	display->drawRect(0, 176, 111, 88, GxEPD_BLACK);
}

void TftDisplay::drawText(const char *text, int y, int x, bool updateWindow = true)
{
	int16_t  x1, y1;
	uint16_t w, h;

	uint8_t oldRotation = display->getRotation();
	display->setRotation(3);

	LogText(String("drawing '") + text + "' to coordinates" + String(x) + " x " + String(y));
	display->setTextColor(GxEPD_BLACK);
	display->setCursor(x, 176 - y);
	display->print(text);

	display->setRotation(oldRotation);
}

void TftDisplay::drawTextRightAligned(const char *text, int y, int x, bool updateWindow = true)
{
	int16_t  x1, y1;
	uint16_t w, h;

	uint8_t oldRotation = display->getRotation();
	display->setRotation(3);

	LogText(String("drawing '") + text + "' right aligned to coordinates" + String(x) + " x " + String(y));
	display->setTextColor(GxEPD_BLACK);
	display->setCursor(x - w, 176 - y);
	display->print(text);

	display->setRotation(oldRotation);
}


void TftDisplay::drawTime(int hours, int minutes, bool updateWindow)
{
	char buffer[10];
	display->setFreeFont(&Open_Sans_SemiBold_38);
	sprintf(buffer, "%02d:%02d", hours, minutes);
	drawText(buffer, 116, 78, updateWindow);
}

void TftDisplay::drawDate(int day, int month, int year, bool updateWindow)
{
	char buffer[12];
	display->setFreeFont(&Open_Sans_SemiBold_18);
	sprintf(buffer, "%2d.%02d.%04d", day, month, year);
	drawText(buffer, 158, 78, updateWindow);
}

void TftDisplay::drawTimeAndDate(date_time_t date) {
	display->fillRect(111, 76, 64, 112, GxEPD_WHITE);
	drawTime(date.hour, date.minute, false);
	drawDate(date.day, date.month, date.year, false);
}

const unsigned char *Bmps[3] = { Bmp_Haus, Bmp_Baum, Bmp_Turtle };

void TftDisplay::drawSensorData(int index, SensorData& data)
{
	int x1 = 1 + (88 * index),
		y1 = 1,
		w = 84,
		h = 102;

	int border = w - 24;
	display->fillRect(y1, x1, h, w, GxEPD_WHITE);
	display->drawBitmap(y1 + 84, x1 + 2, Bmps[index], 16, 16, GxEPD_BLACK);
	display->setFreeFont(&Open_Sans_SemiBold_26);
	String value = String(data.temperature, 1);
	drawTextRightAligned(value.c_str(), 58, x1 + border + 1, false);
	drawText("C", 58, x1 + border + 4, false);
	value = String(data.humidity, 1);
	drawTextRightAligned(value.c_str(), 28, x1 + border - 1, false);
	drawText("%", 28, x1 + border + 2, false);
	display->setFreeFont(&Open_Sans_SemiBold_18);
	value = String(data.pressure, 0);
	drawTextRightAligned(value.c_str(), 4, x1 + border - 10, false);
	display->setFreeFont(&Open_Sans_Condensed_Bold_18);
	drawText("hPa", 4, x1 + border - 8, false);
}


