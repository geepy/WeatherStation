/*-----------------------------------------------------------
NTP & time routines for ESP8266
	for ESP8266 adapted Arduino IDE

by Stefan Thesen 05/2015 - free for anyone

code for ntp adopted from Michael Margolis
code for time conversion based on http://stackoverflow.com/
-----------------------------------------------------------*/

// note: all timing relates to 01.01.1970

#include <ESP8266WiFi.h>
#include <WifiUdp.h>
#include "time_ntp.h"

// NTP specifics
IPAddress timeServer(129, 6, 15, 28); // time.nist.gov NTP server
unsigned int ntpPort = 2390;          // local port to listen for UDP packets


date_time_t NTP::GetTimeStruct(unsigned long secondsSince1970) {
	date_time_t result;
	epoch_to_date_time(&result, secondsSince1970);
	return result;
}

String NTP::GetTimeString(unsigned long secondsSince1970) {
	return epoch_to_string(secondsSince1970);
}

int lastsundayofmonth[7] = { 28,29,30,31,25,26,27 };

// local time here means CET/CEST
unsigned long NTP::GetLocalTime() {
	date_time_t dateAndTime;
	date_time_t firstOfMonth;
	unsigned long now = GetTimestamp();
	unsigned long firstOfMonthTimestamp;
	int dayOfTimeChange;
	epoch_to_date_time(&dateAndTime, now);
	switch (dateAndTime.month) {
	case 11:
	case 12:
	case  1:
	case  2:
		now += 3600; // 1 h while in CET
		break;
	case 4:
	case 5:
	case 6:
	case 7:
	case 8:
	case 9:
		now += 7200; // 2 h while in CEST
		break;
	case 3:
	case 10:
		now += 3600;
		firstOfMonth.year = dateAndTime.year;
		firstOfMonth.month = dateAndTime.month;
		firstOfMonth.day = 1;
		firstOfMonth.hour = firstOfMonth.minute = firstOfMonth.second = 0;
		firstOfMonthTimestamp = date_time_to_epoch(&firstOfMonth);
		dayOfTimeChange = lastsundayofmonth[epoch_to_weekday(firstOfMonthTimestamp)];
		if ((dateAndTime.day < dayOfTimeChange && dateAndTime.month == 10)
			||
			(dateAndTime.day >= dayOfTimeChange && dateAndTime.month == 3))
		{
			now += 3600;
		}
		break;
	}
	return now;
}
static unsigned short days[4][12] =
{
	{   0,  31,  59,  90, 120, 151, 181, 212, 243, 273, 304, 334},
	{ 365, 396, 424, 455, 485, 516, 546, 577, 608, 638, 669, 699},
	{ 730, 762, 790, 821, 851, 882, 912, 943, 974,1004,1035,1065},
	{1096,1127,1155,1186,1216,1247,1277,1308,1339,1369,1400,1430},
};



unsigned long NTP::date_time_to_epoch(date_time_t* date_time)
{
	unsigned long second = date_time->second;  // 0-59
	unsigned long minute = date_time->minute;  // 0-59
	unsigned long hour = date_time->hour;    // 0-23
	unsigned long day = date_time->day - 1;   // 0-30
	unsigned long month = date_time->month - 1; // 0-11
	unsigned long year = date_time->year;    // 0-99
	return (((year / 4 * (365 * 4 + 1) + days[year % 4][month] + day) * 24 + hour) * 60 + minute) * 60 + second;
}

unsigned long NTP::GetTimestamp()
{
	unsigned long ulSecs1970;

	WiFi.hostByName("0.pool.ntp.org", timeServer);
	Serial.println("NTP: starting udp conversation");
	udp.begin(ntpPort);
	sendNTPpacket(timeServer); // send an NTP packet to a time server
	int cb = 0, repeat = 0;
	while (!cb && repeat < 20)  // try for 2 sec
	{
		Serial.println("NTP: checking for response...");
		cb = udp.parsePacket();
		delay(100);
		repeat++;
	}

	if (!cb)
	{
		Serial.println("NTP: no response - exiting");
		ulSecs1970 = 0;
	}
	else
	{
		Serial.println("NTP: reading data from udp");
		udp.read(packetBuffer, NTP_PACKET_SIZE); // read the packet into the buffer

		//the timestamp starts at byte 40 of the received packet and is four bytes,
		// or two words, long. First, esxtract the two words:
		unsigned long highWord = word(packetBuffer[40], packetBuffer[41]);
		unsigned long lowWord = word(packetBuffer[42], packetBuffer[43]);

		// combine the four bytes (two words) into a long integer
		// this is NTP time (seconds since Jan 1 1900):
		ulSecs1970 = highWord << 16 | lowWord;
		Serial.print("NTP: raw data is ");
		Serial.print(ulSecs1970);
		Serial.print(", seconds sincs 1970 is ");
		ulSecs1970 -= 2208988800UL; // go from 1900 to 1970
		Serial.println(ulSecs1970);
	}
	return(ulSecs1970);
}

// send an NTP request to the time server at the given address
unsigned long NTP::sendNTPpacket(IPAddress& address)
{
	// set all bytes in the buffer to 0
	memset(packetBuffer, 0, NTP_PACKET_SIZE);
	// Initialize values needed to form NTP request
	// (see URL above for details on the packets)
	packetBuffer[0] = 0b11100011;   // LI, Version, Mode
	packetBuffer[1] = 0;     // Stratum, or type of clock
	packetBuffer[2] = 6;     // Polling Interval
	packetBuffer[3] = 0xEC;  // Peer Clock Precision

	// 8 bytes of zero for Root Delay & Root Dispersion
	packetBuffer[12] = 49;
	packetBuffer[13] = 0x4E;
	packetBuffer[14] = 49;
	packetBuffer[15] = 52;

	// all NTP fields have been given values, now
	// you can send a packet requesting a timestamp:
	udp.beginPacket(address, 123); //NTP requests are to port 123
	udp.write(packetBuffer, NTP_PACKET_SIZE);
	udp.endPacket();
}


void NTP::epoch_to_date_time(date_time_t* date_time, unsigned long epoch)
{
	unsigned long inputValue = epoch;
	date_time->second = epoch % 60; epoch /= 60;
	date_time->minute = epoch % 60; epoch /= 60;
	date_time->hour = epoch % 24; epoch /= 24;

	unsigned int years = epoch / (365 * 4 + 1) * 4; epoch %= 365 * 4 + 1;

	unsigned int year;
	for (year = 3; year > 0; year--)
	{
		if (epoch >= days[year][0])
			break;
	}

	unsigned int month;
	for (month = 11; month > 0; month--)
	{
		if (epoch >= days[year][month])
			break;
	}

	date_time->year = years + year + 1970;
	date_time->month = month + 1;
	date_time->day = epoch - days[year][month] + 1;
	Serial.printf("NTP: transform %lu to %d . %d . %d %d : %d : %d", inputValue,
		date_time->day, date_time->month, date_time->year, date_time->hour, date_time->minute, date_time->second);
	Serial.println();
}

String NTP::epoch_to_string(unsigned long epoch)
{
	date_time_t date_time;
	epoch_to_date_time(&date_time, epoch);
	String s;
	int i;

	char buffer[21];
	sprintf(buffer, "%02d:%02d:%02d %02d.%02d.%04d",
		date_time.hour,
		date_time.minute,
		date_time.second,
		date_time.day,
		date_time.month,
		date_time.year);

	return(String(buffer));
}

// returns weekday, 0= monday, 6=sunday 
int NTP::epoch_to_weekday(unsigned int epoch)
{
	return ((epoch / 86400) % 7 + 4) % 7;
}

