#ifndef AHT20_H
#define AHT20_H

#define AHT20_ADDR 0x38
#define AHT20_MAX_RETRIES 5
#define SENSOR_TEMP_MAX 85
#define SENSOR_TEMP_MIN -40
#define SENSOR_HUM_MAX 100
#define SENSOR_HUM_MIN 0

#include "stm32f1xx_hal.h"
#include "hal_i2c.h"

typedef enum {
	    SENSOR_STATUS_OK = 0,
	    SENSOR_STATUS_NOT_FOUND,
		SENSOR_COMMUNICATION_ERROR,
	    SENSOR_STATUS_INVALID_DATA,
	    SENSOR_STATUS_BUSY,
        SENSOR_STATUS_BUS_ERROR,
		SENSOR_NOT_CALIBRATED
}SensorStatus;


SensorStatus AHT20_Init(void);
SensorStatus AHT20_Read( uint8_t *temp_int, uint8_t *temp_frac,
		uint8_t *hum_int,  uint8_t *hum_frac);
SensorStatus sensor_convert_i2c_status (i2c_status_t i2c_status);




#endif
