#pragma once

class SensorData
{
	public:
		const float NO_SENSOR_DATA = 200;
		float temperature;
		float pressure;
		float humidity;
		float voltage;

		SensorData();
	~SensorData();
};

