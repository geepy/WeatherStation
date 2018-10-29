/*-----------------------------------------------------------
NTP & time routines for ESP8266 
    for ESP8266 adapted Arduino IDE

by Stefan Thesen 05/2015 - free for anyone

code for ntp adopted from Michael Margolis
code for time conversion based on http://stackoverflow.com/
-----------------------------------------------------------*/

// note: all timing relates to 01.01.2000 otherwise function name indicates differently

#ifndef _DEF_TIME_NTP_ST_
#define _DEF_TIME_NTP_ST_

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <WiFiUdp.h>

const int NTP_PACKET_SIZE = 48;       // NTP time stamp is in the first 48 bytes of the message

typedef struct
{
    unsigned char second; // 0-59
    unsigned char minute; // 0-59
    unsigned char hour;   // 0-23
    unsigned char day;    // 1-31
    unsigned char month;  // 1-12
    unsigned char year;   // 0-99 (representing 2000-2099)
}
date_time_t;

class NTP {
public:
	unsigned long GetTimestamp();
	date_time_t GetTimeStruct(unsigned long secondsSince1970);
	String GetTimeString(unsigned long secondsSince1970);
	unsigned long GetLocalTime();

private:
	unsigned long sendNTPpacket(IPAddress& address);
	String epoch_to_string(unsigned int epoch);
	void epoch_to_date_time(date_time_t* date_time, unsigned int epoch);
	unsigned int date_time_to_epoch(date_time_t* date_time);
	int epoch_to_weekday(unsigned int epoch);
	WiFiUDP udp;  // A UDP instance to let us send and receive packets over UDP
	byte packetBuffer[NTP_PACKET_SIZE];  //buffer to hold incoming and outgoing packets
};

#endif
