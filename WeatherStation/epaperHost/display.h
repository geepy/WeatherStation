// display.h

#ifndef _DISPLAY_h
#define _DISPLAY_h

#include "arduino.h"
#include "SensorData.h"
#include "time_ntp.h"

class EpaperDisplay {
public:
	EpaperDisplay();
	void init();
	void drawSensorData(int index, SensorData& data);
	void drawTimeAndDate(date_time_t date);
	void drawFrames();
	void drawText(const char *text, int y, int x, bool updateWindow);
	void update();

private:
	GxIO_Class* io;
	GxEPD_Class* display;

	void drawRect(int x, int y, int w, int h);
	void drawTextRightAligned(const char *text, int y, int x, bool updateWindow);
	void drawDate(int day, int month, int year, bool updateWindow);
	void drawTime(int hours, int minutes, bool updateWindow);
};
#endif
