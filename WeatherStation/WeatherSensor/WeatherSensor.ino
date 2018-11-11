#include <SPI.h>
#include <RH_ASK.h>		  // für Funkübertragung
#include <Wire.h>         // i²c-Schnittstelle
#include "BMP180.h"       // patched Temperatur-/Luftdruck-Sensor
#include "dht.h"	      // patched Temperatur-/Feuchtigkeit-Sensor
#include <AS_BH1750.h>    // Helligkeitssensor

#include <ssd1306.h>      // OLED-Display 128x64
#include <font6x8.h>      // oled font

#define LOG_TRANSMIT
unsigned long counter;
BMP180 bmp180;            // Temperatur- und Druck-Sensor
AS_BH1750 bh1750;         // Helligkeitssensor

#define DHT_POWERPIN 8
#define DHT_GNDPIN 6
#define DHT_DATAPIN 7
#define WIRE_DATAPIN 12
#define LEDPIN 13

#define SENSORID 1

DHT dht(DHT_DATAPIN, DHT22);        // Temperatur- und Feuchte-Sensor
RH_ASK driver(5000, 0, WIRE_DATAPIN, 0);

long delay_ms = 100;
long delay_sensor = 2000;
int samplesForPressure = 1;
#define TEMPERATURE 0
#define PRESSURE 1
#define BRIGHTNESS 2
#define HUMIDITY 3
#define VOLTAGE 4

char prefix[6] = "TPBHV";
char postfix[5][4] = { "C", "hPa", "lux", "%", "V" };

#define CURRENT 0
#define MIN 1
#define MAX 2

float values[3][5];

bool transmitDataNow;

void setup() {
	Serial.begin(115200);
	counter = 4711;
	pinMode(LEDPIN, OUTPUT);
	pinMode(3, INPUT_PULLUP);
	if (!driver.init())
		Serial.println("init of RF transmitter failed");

	transmitDataNow = false;

	bmp180.init();
	bh1750.begin();
	pinMode(DHT_GNDPIN, OUTPUT);
	digitalWrite(DHT_GNDPIN, LOW);
	pinMode(DHT_POWERPIN, OUTPUT);
	digitalWrite(DHT_POWERPIN, LOW);
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
		break;
		f = readTemperature();
		setValue(TEMPERATURE, f / 10);
		break;

	case 1: // Luftdruck von BMP180
		break;
		l = readPressure();
		setValue(PRESSURE, bmp180.formatPressure(l));
		break;

	case 2: // Luftfeuchtigkeit von DHT22
		digitalWrite(DHT_POWERPIN, HIGH);
		delay(500); // spin up sensor
		f = dht.readHumidity();
		digitalWrite(DHT_POWERPIN, LOW); // shut down sensor
		setValue(HUMIDITY, f);
		break;

	case 3: // Temperatur von DHT22
		digitalWrite(DHT_POWERPIN, HIGH);
		delay(500); // spin up sensor
		f = dht.readTemperature();
		digitalWrite(DHT_POWERPIN, LOW);
		setValue(TEMPERATURE, f);
		break;

	case 4: // Helligkeit von BH1750
		break;
		f = readLightLevel();
		setValue(BRIGHTNESS, f);
		break;

	case 5: // eigene Spannung
		f = readVoltage();
		setValue(VOLTAGE, f);
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
	uint8_t buf[RH_ASK_MAX_MESSAGE_LEN];

	buf[0] = '0' + SENSORID;
	buf[1] = ':';
	int msglen = strlen(msg);
	for (int i = 0; i <= msglen; i++)
	{
		buf[i+2] = msg[i];
	}
#ifdef	LOG_TRANSMIT	
	Serial.print("message to send: [");
	Serial.print((char*)buf);
	Serial.print("] ");
	Serial.print(msglen+2);
	Serial.println(" Zeichen");
#endif
	//Sending the string
	driver.send((uint8_t*)buf, msglen+2);
	driver.waitPacketSent();
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

float readVoltage() {
	// Read 1.1V reference against AVcc
	// set the reference to Vcc and the measurement to the internal 1.1V reference
#if defined(__AVR_ATmega32U4__) || defined(__AVR_ATmega1280__) || defined(__AVR_ATmega2560__)
	ADMUX = _BV(REFS0) | _BV(MUX4) | _BV(MUX3) | _BV(MUX2) | _BV(MUX1);
#elif defined (__AVR_ATtiny24__) || defined(__AVR_ATtiny44__) || defined(__AVR_ATtiny84__)
	ADMUX = _BV(MUX5) | _BV(MUX0);
#elif defined (__AVR_ATtiny25__) || defined(__AVR_ATtiny45__) || defined(__AVR_ATtiny85__)
	ADMUX = _BV(MUX3) | _BV(MUX2);
#else
	ADMUX = _BV(REFS0) | _BV(MUX3) | _BV(MUX2) | _BV(MUX1);
#endif  

	delay(2); // Wait for Vref to settle
	ADCSRA |= _BV(ADSC); // Start conversion
	while (bit_is_set(ADCSRA, ADSC)); // measuring

	uint8_t low = ADCL; // must read ADCL first - it then locks ADCH  
	uint8_t high = ADCH; // unlocks both

	long result = (high << 8) | low;
	Serial.print("voltage raw data is ");
	Serial.println(result);
	result = 1230000L / result; // Calculate Vcc (in mV); 1125300 = 1.1*1023*1000
	return result / 1000.; // Vcc in millivolts
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
	printValue(4, 42, VOLTAGE, values[index][VOLTAGE]);
	// ssd1306_printFixed(4, 48, "Guenters", STYLE_BOLD);
	ssd1306_printFixed(4, 58, "Wetterstation", STYLE_BOLD);
}

void writeValue(char *buffer, int type, float value, bool transmitData)
{
	char number_buffer[10];
	dtostrf(value, -6, 2, number_buffer);
	buffer[0] = prefix[type];
	buffer[1] = ':';
	strcpy(buffer + 2, number_buffer);
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
			// ssd1306_displayOff();
		}
	}
}

