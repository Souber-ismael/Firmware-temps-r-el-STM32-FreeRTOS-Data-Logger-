/*
 * watchdog.h
 *
 *  Created on: Oct 9, 2026
 *      Author: admin
 */

#ifndef INC_WATCHDOG_H_
#define INC_WATCHDOG_H_

#include "stm32f1xx_hal.h"
#include "FreeRTOS.h"
#include "cmsis_os.h"

#define WD_BIT_SENSOR     (1<<0)
#define WD_BIT_STORAGE  (1<<1)
#define WD_ALL_BITS     (WD_BIT_SENSOR | WD_BIT_STORAGE)


extern EventGroupHandle_t wdEventGroup;

extern IWDG_HandleTypeDef hiwdg;

void IWDG_Start(void);


#endif /* INC_WATCHDOG_H_ */
