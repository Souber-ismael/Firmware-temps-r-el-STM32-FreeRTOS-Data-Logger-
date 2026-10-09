/*
 * hal_i2c.h
 *
 *  Created on: Sep 24, 2026
 *      Author: admin
 */

#ifndef INC_HAL_I2C_H_
#define INC_HAL_I2C_H_

#include "stm32f1xx_hal.h"

typedef enum {
	I2C_OK = 0x00,
	I2C_ERR_HAL = 0x01,
	I2C_ERR_TIMEOUT,
	I2C_ERR_BUSY,
	I2C_ERR_NACK,
	I2C_ERR_PRAM,
	I2C_ERR_NOTREADY,
} i2c_status_t;

extern I2C_HandleTypeDef hi2c1;

i2c_status_t MX_I2C1_INIT(void);
void MX_I2CGPIO_INIT(void);
i2c_status_t I2C_check(uint32_t h);
i2c_status_t i2c_transmit(uint8_t address, uint8_t *data, uint16_t size,
		uint32_t timeout);
i2c_status_t i2c_receive(uint8_t address, uint8_t *data, uint16_t size,
		uint32_t timeout);
i2c_status_t hal_i2c_probe(uint8_t address);

#endif /* INC_HAL_I2C_H_ */
