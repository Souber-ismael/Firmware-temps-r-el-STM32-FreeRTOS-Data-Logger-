/*
 * watchdog.c
 *
 *  Created on: Oct 9, 2026
 *      Author: admin
 */

#include "watchdog.h"

IWDG_HandleTypeDef hiwdg;

EventGroupHandle_t wdEventGroup;



void IWDG_Start(void)
{
    hiwdg.Instance       = IWDG;
    hiwdg.Init.Prescaler = IWDG_PRESCALER_128;
    hiwdg.Init.Reload    = 3242;
    HAL_IWDG_Init(&hiwdg);
}

void IWDG_INIT(void){
	IWDG_Start();

	wdEventGroup = xEventGroupCreate();
}
