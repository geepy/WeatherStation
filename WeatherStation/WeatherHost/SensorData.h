#ifndef __ISDEFINED_SENSORDATA
#define __ISDEFINED_SENSORDATA

#define SENSORDATA_NO_DATA -1000.

#define SENSORTYPE_TEMPERATURE 0
#define SENSORTYPE_PRESSURE 1
#define SENSORTYPE_HUMIDITY 2
#define SENSORTYPE_VOLTAGE 3
#define SENSORTYPE_BRIGHTNESS 4
#define NUM_SENSORTYPES 5
class SensorData
{
 protected:


 public:
	 const char* SensorName;
	 unsigned long timestamp;
	 float Value[NUM_SENSORTYPES];

	 SensorData();
	 void WriteToSerial();
};


#endif // !__ISDEFINED_SENSORDATA