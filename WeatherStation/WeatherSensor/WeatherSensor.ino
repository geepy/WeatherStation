#include <VirtualWire.h>  // für Funkübertragung
#include <Wire.h>         // i²c-Schnittstelle
#include "BMP180.h"       // patched Temperatur-/Luftdruck-Sensor
#include "dht.h"	      // patched Temperatur-/Feuchtigkeit-Sensor
#include <AS_BH1750.h>    // Helligkeitssensor

#include <ssd1306.h>      // OLED-Display 128x64
#include <font6x8.h>      // oled font

// #define LOG_TRANSMIT
unsigned long counter;
BMP180 bmp180;            // Temperatur- und Druck-Sensor
AS_BH1750 bh1750;         // Helligkeitssensor
DHT dht(9, DHT22);        // Temperatur- und Feuchte-Sensor

#define LEDPIN 13

long delay_ms = 100;
long delay_sensor = 2000;
int samplesForPressure = 1;
#define TEMPERATURE 0
#define PRESSURE 1
#define BRIGHTNESS 2
#define HUMIDITY 3

char prefix[5] = "TPBH";
char postfix[4][4] = { "C", "hPa", "lux", "%" };

#define CURRENT 0
#define MIN 1
#define MAX 2

float values[3][4];

bool transmitDataNow;

void setup() {
	Serial.begin(115200);
	counter = 4711;
	pinMode(LEDPIN, OUTPUT);
	pinMode(3, INPUT_PULLUP);
	vw_setup(5000);  // 5kBaud
	vw_set_tx_pin(12);

	transmitDataNow = false;

	bmp180.init();
	bh1750.begin();
	dht.begin();

	memset(values, 0, sizeof(values));
	initDisplay();
}

int cycle = 0;
int displayPage = CURRENT;
unsigned long lastSensorCheck = 0;

void loop() {
	char *msg;
	char number_buffer[20];
	long l = 0;
	float f = 0;

	checkButton();
	if (millis() - lastSensorCheck < delay_sensor)
	{
		return;
	}
	lastSensorCheck += delay_sensor; // don't slide
	cycle++; // nächster Sensor
	switch (cycle)
	{
	case 0: // Temperatur von BMP180
		f = readTemperature();
		setValue(TEMPERATURE, f / 10);
		break;

	case 1: // Luftdruck von BMP180
		l = readPressure();
		setValue(PRESSURE, bmp180.formatPressure(l));
		break;

	case 4: // Helligkeit von BH1750
		f = readLightLevel();
		setValue(BRIGHTNESS, f);
		break;

	case 3: // Temperatur von DHT22
		f = dht.readTemperature();
		setValue(TEMPERATURE, f);
		break;

	case 2: // Luftfeuchtigkeit von DHT22
		f = dht.readHumidity();
		setValue(HUMIDITY, f);
		break;

	default: // am Ende angekommen: Zähler zurücksetzen
		transmitMessage("I:nix neues");
		cycle = -1;
		break;
	}
	refreshDisplay(displayPage);
	delay(delay_ms);  // Eine Sekunde Pause, danach startet der Sketch von vorne
}

void test()
{
}

void printConstantMessage(char* msg)
{
	sprintf(msg, "Es sind 23 Grad hier");
	int len = strlen(msg);
}

void transmitMessage(char* msg)
{
	uint8_t buf[VW_MAX_MESSAGE_LEN];
#ifdef LOG_TRANSMIT
	Serial.print("message to send: [");
	Serial.print(msg);
	Serial.print("] ");
#endif
	int msglen = strlen(msg);
	int i;
	for (i = 0; i < msglen; i++)
	{
		buf[i] = msg[i];
	}
#ifdef	LOG_TRANSMIT	
	Serial.print(msglen);
	Serial.println(" Zeichen");
#endif
	//Sending the string
	vw_send((uint8_t*)msg, msglen);
	vw_wait_tx();
}

int readTemperature()
{
	long ut = bmp180.measureTemperature();
	int t = bmp180.compensateTemperature(ut);
	return t;
}

long readPressure()
{
	byte samplingMode = BMP180_OVERSAMPLING_HIGH_RESOLUTION;
	long up;

	long p = 0;
	for (int i = 0; i < samplesForPressure; i++) {
		up = bmp180.measurePressure(samplingMode);
		p += bmp180.compensatePressure(up, samplingMode);
	}
	p = p / samplesForPressure;

	return p;
}

float readLightLevel()
{
	return bh1750.readLightLevel();
}


void initDisplay()
{
	sh1106_128x64_i2c_init();
	ssd1306_setFixedFont(ssd1306xled_font6x8);
	ssd1306_clearScreen();

	refreshDisplay(CURRENT);
}

void refreshDisplay(int index)
{
	char buffer[30];

	// sh1106_clearScreen();
	switch (index)
	{
	case CURRENT:
		strcpy(buffer, "   ");
		break;
	case MIN:
		strcpy(buffer, "min");
		break;
	case MAX:
		strcpy(buffer, "max");
		break;
	}

	ssd1306_printFixed(100, 8, buffer, STYLE_BOLD);
	printValue(4, 10, TEMPERATURE, values[index][TEMPERATURE]);
	printValue(4, 18, PRESSURE, values[index][PRESSURE]);
	printValue(4, 26, BRIGHTNESS, values[index][BRIGHTNESS]);
	printValue(4, 34, HUMIDITY, values[index][HUMIDITY]);
	ssd1306_printFixed(4, 48, "Guenters", STYLE_BOLD);
	ssd1306_printFixed(4, 58, "Wetterstation", STYLE_BOLD);
}

void writeValue(char *buffer, int type, float value, bool transmitData)
{
	char number_buffer[10];
	dtostrf(value, 6, 1, number_buffer);
	buffer[0] = prefix[type];
	buffer[1] = ':';
	strcpy(buffer + 2, number_buffer);
	strcat(buffer, postfix[type]);
	if (transmitData)
	{
		transmitMessage(buffer);
	}
}

void printValue(int x, int y, int type, float value)
{
	char buffer[30];
	writeValue(buffer, type, value, false);
	ssd1306_printFixed(x, y, buffer, STYLE_NORMAL);
}

void setValue(int index, float value)
{
	char buffer[30];

	values[CURRENT][index] = value;
	if (values[MAX][index] == 0 || value > values[MAX][index])
	{
		values[MAX][index] = value;
	}
	if (values[MIN][index] == 0 || value < values[MIN][index])
	{
		values[MIN][index] = value;
	}
	writeValue(buffer, index, value, true);
}

unsigned long buttonPressedAt = 0;
unsigned long actionTakenAt = 0;
#define PRESSED LOW
#define LOOSE HIGH
int lastButtonState = LOOSE;
void checkButton()
{
	int buttonState = digitalRead(3);
	digitalWrite(LEDPIN, 1 - buttonState);
	unsigned long delta = millis() - buttonPressedAt;
	if (buttonState == PRESSED)
	{
		if (lastButtonState == LOOSE) {
			lastButtonState = PRESSED;
			buttonPressedAt = millis();
			actionTakenAt = 0;
			ssd1306_displayOn();
		}
		// avoid prell
		if (millis() - buttonPressedAt > 50) {
			if (millis() - buttonPressedAt > 5000) {
				ssd1306_displayOff();
			}
			else if (millis() - actionTakenAt > 700)
			{
				displayPage++;
				if (displayPage > 2)
				{
					displayPage = 0;
				}
				actionTakenAt = millis();
			}
		}
	}
	else
	{
		lastButtonState = LOOSE;
		if (millis() - actionTakenAt >= 5000)
		{
			displayPage = CURRENT;
		}
		if (millis() - actionTakenAt > 20000)
		{
			ssd1306_displayOff();
		}
	}
}

