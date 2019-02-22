// 
// 
// 

#include "Log.h"

// #define LOG

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


