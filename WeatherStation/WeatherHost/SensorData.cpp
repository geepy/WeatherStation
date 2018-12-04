#include <Arduino.h>
#include "SensorData.h"

const char* SENSOR_DEFAULT_NAME = "(unbenannt)";
SensorData::SensorData() {
	this->SensorName = SENSOR_DEFAULT_NAME;
	for (int i = 0; i < NUM_SENSORTYPES; i++) {
		this->Value[i] = SENSORDATA_NO_DATA;
	}
}

void SensorData::WriteToSerial() {
	char number_buffer[10];

	String output = String("Data from Sensor ");
	output += this->SensorName;
	output += ": Temperature=" + String(this->Value[SENSORTYPE_TEMPERATURE], 1);
	output += "; Pressure=" + String(this->Value[SENSORTYPE_PRESSURE], 1);
	output += "; Humidity=" + String(this->Value[SENSORTYPE_HUMIDITY], 1);
	output += "; Brightness=" + String(this->Value[SENSORTYPE_BRIGHTNESS], 1);
	output += "; Voltage=" + String(this->Value[SENSORTYPE_VOLTAGE], 1);
	Serial.println(output);
}