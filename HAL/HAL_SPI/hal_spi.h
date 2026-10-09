/*
 * hal_spi.h
 *
 *  Created on: Sep 29, 2026
 *      Author: admin
 */

#ifndef INC_HAL_SPI_H_
#define INC_HAL_SPI_H_

#include "stm32f1xx.h"
#include "stm32f1xx_hal.h"
#include "stm32f1xx_hal_spi.h"

extern SPI_HandleTypeDef hspi2;

typedef enum
{
    SPI_OK,
	SPI_NOINIT,
    SPI_ERROR,
    SPI_TIMEOUT,
	SPI_BUSY
} SPI_STATUS;

SPI_STATUS MX_SPI_INIT(void);
void SD_CS_LOW(void);
void SD_CS_HIGH(void);
SPI_STATUS SPI_Tx(uint8_t *tx , uint16_t size);
SPI_STATUS SPI_TxRx(uint8_t *tx, uint8_t *rx);
SPI_STATUS SD_SetHighSpeed(void);

#endif /* INC_HAL_SPI_H_ */
