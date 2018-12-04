#define BME280_S32_t signed long;
// Returns temperature in DegC, double precision. Output value of “51.23” equals 51.23 DegC.
// t_fine carries fine temperature as global value
BME280_S32_t t_fine;
double BME280_compensate_T_double(BME280_S32_t adc_T)
{
	double var1, var2, T;
	var1 = (((double)adc_T) / 16384.0 –((double)dig_T1) / 1024.0) * ((double)dig_T2);
	var2 = ((((double)adc_T) / 131072.0 –((double)dig_T1) / 8192.0) * (((double)adc_T) / 131072.0 –((double)dig_T1) / 8192.0)) * ((double)dig_T3);
	t_fine = (BME280_S32_t)(var1 + var2);
	T = t_fine / 5120.0;
	return T;
}

// Returns humidity in %rH as as double. Output value of “46.332” represents 46.332 %rH
double bme280_compensate_H_double(BME280_S32_t adc_H)
{
	double var_H;
	var_H = (((double)t_fine) – 76800.0);
	var_H = (adc_H –(((double)dig_H4) * 64.0 + ((double)dig_H5) / 16384.0 * var_H)) * (((double)dig_H2) / 65536.0 * (1.0 + ((double)dig_H6) / 67108864.0 * var_H * (1.0 + ((double)dig_H3) / 67108864.0 * var_H)));
	var_H = var_H * (1.0 –((double)dig_H1) * var_H / 524288.0);
	if (var_H > 100.0)
		var_H = 100.0;
	else if (var_H < 0.0)
		var_H = 0.0;
	return var_H;
	)