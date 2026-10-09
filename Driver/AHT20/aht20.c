#include "aht20.h"
#include "hal_i2c.h"
#include "uart_ll.h"

SensorStatus sensor_status;

SensorStatus AHT20_Init(void) {
	i2c_status_t st;

	st = hal_i2c_probe( AHT20_ADDR);

	if (st != I2C_OK) {
		return sensor_convert_i2c_status(st);
	}

	uint8_t reset_cmd[1] = { 0xBA };

	st = i2c_transmit(AHT20_ADDR, reset_cmd, 1, 200);
	if (st != I2C_OK) {
		return sensor_convert_i2c_status(st);
	}

	HAL_Delay(80);

	uint8_t status = 0;
	st = i2c_receive(AHT20_ADDR, &status, 1, 200);
	if (st != I2C_OK) {
		return sensor_convert_i2c_status(st);
	}

	if (!(status & 0x08)) {
		uint8_t cmd[3] = { 0xBE, 0x08, 0x00 };
		st = i2c_transmit(AHT20_ADDR, cmd, 3, 200);
		if (st != I2C_OK) {
			return sensor_convert_i2c_status(st);
		}

		HAL_Delay(40);

		st = i2c_receive(AHT20_ADDR, &status, 1, 200);

		if (st != I2C_OK) {
			return sensor_convert_i2c_status(st);
		}

		if (!(status & 0x08)) {
			return SENSOR_NOT_CALIBRATED;
		}

		return SENSOR_STATUS_OK;
	}

	return SENSOR_STATUS_OK;
}

SensorStatus AHT20_Read(uint8_t *temp_int, uint8_t *temp_frac, uint8_t *hum_int,
		uint8_t *hum_frac) {

	uint8_t cmd[3] = { 0xAC, 0x33, 0x00 };
	i2c_status_t st;
	uint8_t data[6];
	uint8_t retries = 0;

	st = i2c_transmit(AHT20_ADDR, cmd, 3, 200);
	if (st != I2C_OK) {
		return sensor_convert_i2c_status(st);
	}

	do {
		st = i2c_receive(AHT20_ADDR, data, 6, 200);
		if (st == I2C_OK)
			break;
		retries++;
		HAL_Delay(20);
	} while (retries < 5);

	if (st != I2C_OK)
		return sensor_convert_i2c_status(st);

	HAL_Delay(80);

	if (data[0] & 0x80) {
		HAL_Delay(20);
		st = i2c_receive(AHT20_ADDR, data, 6, 200);
		if (st != I2C_OK) {
			return sensor_convert_i2c_status(st);
		}
		if (data[0] & 0x80) {
			return SENSOR_STATUS_BUSY;
		}
	}

	// 5. Extraire les données
	uint32_t rawHum = ((uint32_t) data[1] << 12) | ((uint32_t) data[2] << 4)
			| (data[3] >> 4);

	uint32_t rawTemp = ((uint32_t) (data[3] & 0x0F) << 16)
			| ((uint32_t) data[4] << 8) | data[5];

	// 6. Convertir
	*hum_int = (uint8_t) ((rawHum * 100UL) / 1048576);
	*hum_frac = (uint8_t) ((rawHum * 10000UL / 1048576) % 100);
	*temp_int = (uint8_t) ((rawTemp * 200UL) / 1048576) - 50;
	*temp_frac = (uint8_t) ((rawTemp * 20000UL / 1048576) % 100);

	// Corriger fractions négatives
	if (*temp_frac < 0)
		*temp_frac = -(*temp_frac);
	if (*hum_frac < 0)
		*hum_frac = -(*hum_frac);

	if ((*temp_int > SENSOR_TEMP_MAX) || (*temp_int < SENSOR_TEMP_MIN))
		return SENSOR_STATUS_INVALID_DATA;

	if ((*hum_int > SENSOR_HUM_MAX) || (*hum_int < SENSOR_HUM_MIN))
		return SENSOR_STATUS_INVALID_DATA;

	return SENSOR_STATUS_OK;

}

SensorStatus sensor_convert_i2c_status(i2c_status_t i2c_status) {

	switch (i2c_status) {
	case I2C_OK:
		return SENSOR_STATUS_OK;

	case I2C_ERR_NACK:
		return SENSOR_STATUS_NOT_FOUND;

	case I2C_ERR_TIMEOUT:
		return SENSOR_COMMUNICATION_ERROR;

	case I2C_ERR_BUSY:
		return SENSOR_COMMUNICATION_ERROR;

	case I2C_ERR_HAL:
		return SENSOR_COMMUNICATION_ERROR;

	default:
		return SENSOR_STATUS_BUS_ERROR;
	}
}

