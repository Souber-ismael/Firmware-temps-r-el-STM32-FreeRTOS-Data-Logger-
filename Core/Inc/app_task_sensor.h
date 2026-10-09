/*
 * app_task_sensor.h
 *
 *  Created on: Oct 6, 2026
 *      Author: admin
 */

#ifndef INC_APP_TASK_SENSOR_H_
#define INC_APP_TASK_SENSOR_H_

#include "uart_ll.h"
#include "ath20.h"
#include "FreeRTOS.h"
#include "task.h"
#include "main.h"
#include "app_task_supervisor.h"
#include "watchdog.h"



typedef struct {
	uint8_t temp_int;
	uint8_t temp_frac ;
	uint8_t hum_int;
	uint8_t hum_frac;
	SensorStatus st;
}syste;

void Task_Main(void *argument);


#endif /* INC_APP_TASK_SENSOR_H_ */
