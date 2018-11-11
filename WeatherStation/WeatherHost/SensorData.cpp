#include <Arduino.h>
#include "SensorData.h"

const char* SENSOR_DEFAULT_NAME = "(unbenannt)";
SensorData::SensorData() {
	this->SensorName = SENSOR_DEFAULT_NAME;
	this->Brightness =
		this->Humidity =
		this->Pressure =
		this->Temperature =
		this->Voltage = SENSORDATA_NO_DATA;
}

void SensorData::WriteToSerial() {
	char number_buffer[10];

	Serial.print("Data from Sensor ");
	Serial.print(this->SensorName);
	Serial.print(": ");
	Serial.print("Temperature=");
	dtostrf(this->Temperature, 6, 1, number_buffer);
	Serial.print(number_buffer);
	Serial.print("; Pressure=");
	dtostrf(this->Pressure, 6, 1, number_buffer);
	Serial.print(number_buffer);
	Serial.print("Humidity=");
	dtostrf(this->Humidity, 6, 1, number_buffer);
	Serial.print(number_buffer);
	Serial.print("Brightness=");
	dtostrf(this->Brightness, 6, 1, number_buffer);
	Serial.print(number_buffer);
	Serial.print("Voltage=");
	dtostrf(this->Voltage, 6, 1, number_buffer);
	Serial.print(number_buffer);
	Serial.println();
}