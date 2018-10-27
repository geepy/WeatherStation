/**

BMP180.h

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
#ifndef BMP180_h
#define BMP180_h

 // includes
#include "Arduino.h"

// nice
typedef uint8_t byte;

// i2c address
#define BMP180_I2C_ADDRESS              0x77

// register
#define BMP180_MEASURE_VALUE_XLSB         0xF8
#define BMP180_MEASURE_VALUE_LSB          0xF7
#define BMP180_MEASURE_VALUE_MSB          0xF6
#define BMP180_CONTROL_REGISTER           0xF4
#define BMP180_SOFT_RESET_REGISTER          0xE0
#define BMP180_CHIP_ID_REGISTER           0xD0

// values
#define BMP180_SOFT_RESET             0xB6
#define BMP180_MEASURE_TEMPERATURE          0x2E
#define BMP180_MEASURE_PRESSURE           0x34
#define BMP180_CHIP_ID                0x55

// resolutions
#define BMP180_OVERSAMPLING_ULTRA_LOW_POWER     0x00
#define BMP180_OVERSAMPLING_STANDARD        0x01
#define BMP180_OVERSAMPLING_HIGH_RESOLUTION     0x02
#define BMP180_OVERSAMPLING_ULTRA_HIGH_RESOLUTION 0x03

// calibration data
#define BMP180_CALIBRATION_DATA_AC1         0xAA
#define BMP180_CALIBRATION_DATA_AC2         0xAC
#define BMP180_CALIBRATION_DATA_AC3         0xAE
#define BMP180_CALIBRATION_DATA_AC4         0xB0
#define BMP180_CALIBRATION_DATA_AC5         0xB2
#define BMP180_CALIBRATION_DATA_AC6         0xB4
#define BMP180_CALIBRATION_DATA_B1          0xB6
#define BMP180_CALIBRATION_DATA_B2          0xB8
#define BMP180_CALIBRATION_DATA_MB          0xBA
#define BMP180_CALIBRATION_DATA_MC          0xBC
#define BMP180_CALIBRATION_DATA_MD          0xBE

// class definition
class BMP180 {
public:
	void init();
	void init(int sda, int scl);
	byte getID();
	bool hasValidID();
	void reset();
	long measureTemperature();
	long measurePressure(byte oversampling);
	long compensateTemperature(long UT);
	long compensatePressure(long UP, int oversampling);
	float formatTemperature(long T);
	float formatPressure(long P);
	void setSamplingMode(byte samplingMode);
	float getPressure();
	float getTemperature();
private:
	byte readID();
	void readCalibrationData();
	void selectRegister(byte reg);
	byte readByteFromRegister(byte reg);
	unsigned int readIntFromRegister(byte reg);
	unsigned long readLongFromRegister(byte reg);
	void measure(byte measureID);
	long calculateB5(long UT);
	byte _ID;
	int16_t _AC1;
	int16_t _AC2;
	int16_t _AC3;
	int16_t _B1;
	int16_t _B2;
	int16_t _MB;
	int16_t _MC;
	int16_t _MD;
	uint16_t _AC4;
	uint16_t _AC5;
	uint16_t _AC6;
	long _B5;
	byte _samplingMode;
};

#endif

