// Log.h

#ifndef _LOG_h
#define _LOG_h

#if defined(ARDUINO) && ARDUINO >= 100
	#include "arduino.h"
#else
	#include "WProgram.h"
#endif

void LogValue(float value);
void LogText(const char* text);
void LogText(String& text);
void DumpValue(float value);
void DumpValue(int value);
void DumpText(const char* text);

#endif

