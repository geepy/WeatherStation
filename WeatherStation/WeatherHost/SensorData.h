#ifndef __ISDEFINED_SENSORDATA
#define __ISDEFINED_SENSORDATA

#define SENSORDATA_NO_DATA -1000.

class SensorData
{
 protected:


 public:
	 const char* SensorName;
	 unsigned long timestamp;
	 float Temperature;
	 float Pressure;
	 float Brightness;
	 float Humidity;
	 float Voltage;

	 SensorData();
	 void WriteToSerial();
};


#endif // !__ISDEFINED_SENSORDATA