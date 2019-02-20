// 
// 
// 

#include "arduino.h"
#include <stdio.h>
#include "SensorData.h"

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

#include "display.h"
#include "time_ntp.h"
#include "Log.h"



EpaperDisplay::EpaperDisplay() {
	io = new GxIO_Class(SPI, /*CS=D8*/ SS, /*DC=D3*/ 0, /*RST=D4*/ 2); // arbitrary selection of D3(=0), D4(=2), selected for default of GxEPD_Class
	display = new GxEPD_Class(*io /*RST=D4*/ /*BUSY=D2*/); // default selection of D4(=2), D2(=4)
}

void EpaperDisplay::init() {
	display->init(115200); // enable diagnostic output on Serial
	display->setTextColor(GxEPD_BLACK);
	display->setRotation(2);
}
void EpaperDisplay::update() {
	display->update();
}
void EpaperDisplay::drawRect(int x, int y, int w, int h) {
	display->drawRect(x, y, h, w, GxEPD_BLACK);
	display->drawRect(x + 1, y + 1, h - 2, w - 2, GxEPD_WHITE);
	display->drawRect(x + 2, y + 2, h - 4, w - 4, GxEPD_BLACK);
	display->updateWindow(x, y, h + 1, w + 1);
}

void EpaperDisplay::drawFrames() {
	display->drawRect(0, 0, GxEPD_WIDTH, GxEPD_HEIGHT, GxEPD_BLACK);

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
	display->update();
}

void EpaperDisplay::drawText(const char *text, int y, int x, bool updateWindow = true)
{
	int16_t  x1, y1;
	uint16_t w, h;

	uint8_t oldRotation = display->getRotation();
	display->setRotation(3);

	LogText(String("drawing '") + text + "' to coordinates" + String(x) + " x " + String(y));
	display->setTextColor(GxEPD_BLACK);
	display->getTextBounds(text, x, 176 - y, &x1, &y1, &w, &h);
	display->setCursor(x, 176 - y);
	display->print(text);
	if (updateWindow) {
		LogText(String("  updating window (") + String(x1) + " x " + String(y1) + " size " + String(w) + " x " + String(h));
		display->updateWindow(x1, y1, w, h);
	}

	display->setRotation(oldRotation);
}

void EpaperDisplay::drawTextRightAligned(const char *text, int y, int x, bool updateWindow = true)
{
	int16_t  x1, y1;
	uint16_t w, h;

	uint8_t oldRotation = display->getRotation();
	display->setRotation(3);

	LogText(String("drawing '") + text + "' right aligned to coordinates" + String(x) + " x " + String(y));
	display->setTextColor(GxEPD_BLACK);
	display->getTextBounds(text, 0, 176 - y, &x1, &y1, &w, &h);
	display->setCursor(x - w, 176 - y);
	display->print(text);
	if (updateWindow) {
		LogText(String("  updating window (") + String(x1) + " x " + String(y1) + " size " + String(w) + " x " + String(h));
		display->updateWindow(x + x1, y1, w, h);
	}

	display->setRotation(oldRotation);
}


void EpaperDisplay::drawTime(int hours, int minutes, bool updateWindow)
{
	char buffer[10];
	display->setFont(&Open_Sans_SemiBold_38);
	sprintf(buffer, "%02d:%02d", hours, minutes);
	drawText(buffer, 116, 78, updateWindow);
}

void EpaperDisplay::drawDate(int day, int month, int year, bool updateWindow)
{
	char buffer[12];
	display->setFont(&Open_Sans_SemiBold_18);
	sprintf(buffer, "%2d.%02d.%04d", day, month, year);
	drawText(buffer, 158, 78, updateWindow);
}

void EpaperDisplay::drawTimeAndDate(date_time_t date) {
	display->fillRect(111, 76, 64, 112, GxEPD_WHITE);
	drawTime(date.hour, date.minute, false);
	drawDate(date.day, date.month, date.year, false);
	display->updateWindow(111, 76, 64, 112);
}

const unsigned char *Bmps[3] = { Bmp_Haus, Bmp_Baum, Bmp_Turtle };

void EpaperDisplay::drawSensorData(int index, SensorData& data)
{
	int x1 = 1 + (88 * index),
		y1 = 1,
		w = 84,
		h = 102;

	int border = w - 24;
	display->fillRect(y1, x1, h, w, GxEPD_WHITE);
	display->drawBitmap(y1 + 84, x1 + 2, Bmps[index], 16, 16, GxEPD_BLACK);
	display->setFont(&Open_Sans_SemiBold_26);
	String value = String(data.temperature, 1);
	drawTextRightAligned(value.c_str(), 58, x1 + border + 1, false);
	drawText("C", 58, x1 + border + 4, false);
	value = String(data.humidity, 1);
	drawTextRightAligned(value.c_str(), 28, x1 + border - 1, false);
	drawText("%", 28, x1 + border + 2, false);
	display->setFont(&Open_Sans_SemiBold_18);
	value = String(data.pressure, 0);
	drawTextRightAligned(value.c_str(), 4, x1 + border - 10, false);
	display->setFont(&Open_Sans_Condensed_Bold_18);
	drawText("hPa", 4, x1 + border - 8, false);
	display->updateWindow(y1, x1, h, w);
}


