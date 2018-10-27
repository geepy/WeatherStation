/**

BMP180.cpp

Copyright by Christian Paul, 2014

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.

 */
#include "Arduino.h"
#include "BMP180.h"
#include <Wire.h>

 // nice
typedef uint8_t byte;

/**
 * Init the Chip.
 * Starts I2C communication, read chip's ID and calibration data.
 */
void BMP180::init() {
	Wire.begin();
	setSamplingMode(BMP180_OVERSAMPLING_STANDARD);
	_ID = readID();
	readCalibrationData();
}

void BMP180::init(int sda, int scl) {
	Wire.begin(sda, scl);
	setSamplingMode(BMP180_OVERSAMPLING_STANDARD);
	_ID = readID();
	readCalibrationData();
}

/**
 * Read the ID of the chip.
 */
byte BMP180::readID() {
	return readByteFromRegister(BMP180_CHIP_ID_REGISTER);
}

/**
 * Get the ID of the chip.
 */
byte BMP180::getID() {
	return _ID;
}

/**
 * Check if the ID is valid.
 * ID must be 0x55.
 */
bool BMP180::hasValidID() {
	return getID() == BMP180_CHIP_ID;
}

/**
 * Read calibration data.
 */
void BMP180::readCalibrationData() {
	_AC1 = readIntFromRegister(BMP180_CALIBRATION_DATA_AC1);
	_AC2 = readIntFromRegister(BMP180_CALIBRATION_DATA_AC2);
	_AC3 = readIntFromRegister(BMP180_CALIBRATION_DATA_AC3);
	_AC4 = readIntFromRegister(BMP180_CALIBRATION_DATA_AC4);
	_AC5 = readIntFromRegister(BMP180_CALIBRATION_DATA_AC5);
	_AC6 = readIntFromRegister(BMP180_CALIBRATION_DATA_AC6);
	_B1 = readIntFromRegister(BMP180_CALIBRATION_DATA_B1);
	_B2 = readIntFromRegister(BMP180_CALIBRATION_DATA_B2);
	_MB = readIntFromRegister(BMP180_CALIBRATION_DATA_MB);
	_MC = readIntFromRegister(BMP180_CALIBRATION_DATA_MC);
	_MD = readIntFromRegister(BMP180_CALIBRATION_DATA_MD);
}

/**
 * Make a soft reset.
 */
void BMP180::reset() {
	Wire.beginTransmission(BMP180_I2C_ADDRESS);
	Wire.write(BMP180_SOFT_RESET_REGISTER);
	Wire.write(BMP180_SOFT_RESET);
	Wire.endTransmission();
}

/**
 * Select register for reading operation.
 */
void BMP180::selectRegister(byte reg) {
	Wire.beginTransmission(BMP180_I2C_ADDRESS);
	Wire.write(reg);
	Wire.endTransmission();
}

/**
 * Read a byte from a register.
 */
byte BMP180::readByteFromRegister(byte reg) {
	byte b;
	selectRegister(reg);
	Wire.beginTransmission(BMP180_I2C_ADDRESS);
	Wire.requestFrom(BMP180_I2C_ADDRESS, 1);
	b = Wire.read();
	Wire.endTransmission();
	return b;
}

/**
 * Read an integer from a register.
 */
unsigned int BMP180::readIntFromRegister(byte reg) {
	unsigned int i;
	selectRegister(reg);
	Wire.beginTransmission(BMP180_I2C_ADDRESS);
	Wire.requestFrom(BMP180_I2C_ADDRESS, 2);
	i = Wire.read();
	i = i << 8 | Wire.read();
	Wire.endTransmission();
	return i;
}

/**
 * Read a long from a register.
 * Int true, it's not a long - it's a 19bit value within three bytes.
 */
unsigned long BMP180::readLongFromRegister(byte reg) {
	unsigned long l;
	selectRegister(reg);
	Wire.beginTransmission(BMP180_I2C_ADDRESS);
	Wire.requestFrom(BMP180_I2C_ADDRESS, 3);
	l = Wire.read();
	l = l << 8 | Wire.read();
	l = l << 8 | Wire.read();
	Wire.endTransmission();
	return l;
}

/**
 * Starts a measure.
 */
void BMP180::measure(byte measureID) {
	Wire.beginTransmission(BMP180_I2C_ADDRESS);
	Wire.write(BMP180_CONTROL_REGISTER);
	Wire.write(measureID);
	Wire.endTransmission();
}

/**
 * Starts the measure of the temperature.
 * The measured value must be compensated by the calibration data.
 */
long BMP180::measureTemperature() {
	measure(BMP180_MEASURE_TEMPERATURE);
	delay(5);
	return (long)readIntFromRegister(BMP180_MEASURE_VALUE_MSB);
}

/**
 * Starts the measure of the pressure.
 * The measured value must be compensated by the calibration data.
 */
long BMP180::measurePressure(byte oversampling) {
	measure(BMP180_MEASURE_PRESSURE | (oversampling << 6));
	switch (oversampling) {
	case 0: delay(5); break;
	case 1: delay(8); break;
	case 2: delay(14); break;
	case 3: delay(26); break;
	}
	long p = (long)readLongFromRegister(BMP180_MEASURE_VALUE_MSB);
	p = p >> (8 - oversampling);
	return p;
}

/**
 * Calculate B5
 *
long BMP180::calculateB5(long UT) {
  long X1 = (UT - (long)_AC6) * (long)_AC5 >> 15;
  long X2 = ((long)_MC << 11) / (X1 + (long)_MD);
  return X1 + X2;
}
*/
long BMP180::calculateB5(long UT) {
	long X1 = ((UT - _AC6) * _AC5);
	X1 = X1 >> 15;
	long X2 = ((long)_MC << 11) / (X1 + _MD);
	return X1 + X2;
}

/**
 * Compensate the measured temperature with the calibration data.
 */
long BMP180::compensateTemperature(long UT) {
	_B5 = calculateB5(UT);
	return (_B5 + 8) >> 4;
}

/**
 * Compensate the measured pressure with the calibration data.
 * The temperature must be measured and compensated before this operation - need valid B5.
 *
long BMP180::compensatePressure(long UP, int oversampling) {
  long B6, X1, X2, X3, B3, p;
  unsigned long B4, B7;
  B6 = _B5 - 4000;
  X1 = ((long)_B2 * (B6 * B6 >> 12)) >> 11;
  X2 = ((long)_AC2 * B6) >> 11;
  X3 = X1 + X2;
  B3 = ((((long)_AC1 * 4 + X3) << oversampling) + 2) >> 2;
  X1 = (long)_AC3 * B6 >> 13;
  X2 = ((long)_B1 * (B6 * B6 >> 12)) >> 16;
  X3 = ((X1 + X2) + 2) >> 2;
  B4 = (unsigned long)_AC4 * (unsigned long)(X3 + 32768) >> 15;
  B7 = ((unsigned long)UP - B3) * (50000 >> oversampling);
  if (B7 < 0x80000000)
	p = (B7 * 2) / B4;
  else
	p = (B7 / B4) * 2;
  X1 = (p >> 8) * (p >> 8);
  X1 = (X1 * 3038) >> 16;
  X2 = (-7357 * p) >> 16;
  p = p + ((X1 + X2 + 3791) >> 4);
  return p;
}
*/
long BMP180::compensatePressure(long UP, int oversampling) {
	long B6, X1, X2, X3, B3, p;
	unsigned long B4, B7;
	B6 = _B5 - 4000;
	X1 = (_B2 * (B6 * B6 >> 12)) >> 11;
	X2 = (_AC2 * B6) >> 11;
	X3 = X1 + X2;
	B3 = (((_AC1 * 4 + X3) << oversampling) + 2) >> 2;
	X1 = _AC3 * B6 >> 13;
	X2 = (_B1 * (B6 * B6 >> 12)) >> 16;
	X3 = ((X1 + X2) + 2) >> 2;
	B4 = _AC4 * (unsigned long)(X3 + 32768) >> 15;
	B7 = ((unsigned long)UP - B3) * (50000 >> oversampling);
	if (B7 < 0x80000000)
		p = (B7 * 2) / B4;
	else
		p = (B7 / B4) * 2;
	X1 = (p >> 8) * (p >> 8);
	X1 = (X1 * 3038) >> 16;
	X2 = (-7357 * p) >> 16;
	p = p + ((X1 + X2 + 3791) >> 4);
	return p;
}

/**
 * Format the temperature
 */
float BMP180::formatTemperature(long T) {
	return (float)T / 10;
}

/**
 * Format the pressure
 */
float BMP180::formatPressure(long P) {
	return (float)P / 100;
}

/**
 * Set the sampling mode.
 */
void BMP180::setSamplingMode(byte samplingMode) {
	_samplingMode = samplingMode;
}

/**
 * Get the temperature.
 */
float BMP180::getTemperature() {
	int ut = measureTemperature();
	long t = compensateTemperature(ut);
	return formatTemperature(t);
}

/**
 * Get the pressure.
 */
float BMP180::getPressure() {
	long up = measurePressure(_samplingMode);
	long p = compensatePressure(up, _samplingMode);
	return formatPressure(p);
}

