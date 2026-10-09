/*
 * SD_DRIVER.h
 *
 *  Created on: Sep 29, 2026
 *      Author: admin
 */

#ifndef INC_SD_DRIVER_H_
#define INC_SD_DRIVER_H_
#include "stm32f1xx_hal.h"
#include "hal_spi.h"

typedef enum
{
    SD_OK,
    SD_ERROR,
    SD_TIMEOUT,
    SD_BUSY,
    SD_NOT_READY,
	SD_ERROR_TIMEOUT,
	SD_ERR_DATA_TOKEN,
	SD_ERROR_CDM,
	SD_ERR_RESPONSE,
	SD_ERR_WRITE_TIMEOUT,
    SD_NOT_FOUND
} SD_Status;



SD_Status SD_SendCmd(uint8_t cmd, uint32_t arg, uint8_t crc);
SD_Status SD_PowerUpSequence(void);
SD_Status SD_Init(void);
SD_Status SD_READBLOC(uint32_t  addr , uint8_t *buf);
SD_Status SD_WriteBlock(uint32_t addr ,uint8_t *buffer);

#endif /* INC_SD_DRIVER_H_ */
