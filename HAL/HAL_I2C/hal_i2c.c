/*
 * hal_i2c.c
 *
 *  Created on: Sep 24, 2026
 *      Author: admin
 */
#include "hal_i2c.h"

I2C_HandleTypeDef hi2c1;

i2c_status_t MX_I2C1_INIT(void) {

	hi2c1.Instance = I2C1;
	hi2c1.Init.ClockSpeed = 10000;     // Timing 100 kHz (CubeMX)
	hi2c1.Init.OwnAddress1 = 0;
	hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
	hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
	hi2c1.Init.OwnAddress2 = 0;
	hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
	hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;

	if (HAL_I2C_Init(&hi2c1) != HAL_OK) {
		return I2C_ERR_HAL;
	}

	return I2C_OK;
}

void MX_I2CGPIO_INIT(void) {

	__HAL_RCC_GPIOB_CLK_ENABLE();

	GPIO_InitTypeDef GPIO_initStruct = { 0 };

	GPIO_initStruct.Pin = GPIO_PIN_6 | GPIO_PIN_7;
	GPIO_initStruct.Mode = GPIO_MODE_AF_OD;
	GPIO_initStruct.Pull = GPIO_NOPULL;
	GPIO_initStruct.Speed = GPIO_SPEED_HIGH;

	HAL_GPIO_Init(GPIOB, &GPIO_initStruct);
}

i2c_status_t i2c_convert_hal(HAL_StatusTypeDef s) {

	uint32_t error;
	switch (s) {
	case HAL_OK:
		return I2C_OK;
	case HAL_BUSY:
		return I2C_ERR_BUSY;
	case HAL_TIMEOUT:
		return I2C_ERR_TIMEOUT;
	case HAL_ERROR:
		error = HAL_I2C_GetError(&hi2c1);
		if (error & HAL_I2C_ERROR_AF) {
			return I2C_ERR_NACK;
		}
	default:
		return I2C_ERR_HAL;
	}
}

i2c_status_t hal_i2c_probe(uint8_t address) {
	HAL_StatusTypeDef ret;

	ret = HAL_I2C_IsDeviceReady(&hi2c1, address << 1, 1, 100);

	return i2c_convert_hal(ret);

}

i2c_status_t i2c_transmit(uint8_t address, uint8_t *data, uint16_t size,
		uint32_t timeout) {

	HAL_StatusTypeDef ret;

	ret = HAL_I2C_Master_Transmit(&hi2c1, address << 1, data, size, timeout);

	return i2c_convert_hal(ret);

}

i2c_status_t i2c_receive(uint8_t address, uint8_t *data, uint16_t size,
		uint32_t timeout) {

	HAL_StatusTypeDef ret;

	ret = HAL_I2C_Master_Receive(&hi2c1, address << 1, data, size, timeout);

	return i2c_convert_hal(ret);

}
