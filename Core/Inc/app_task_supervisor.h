/*
 * app_task_supervisor.h
 *
 *  Created on: Oct 6, 2026
 *      Author: admin
 */

#ifndef INC_APP_TASK_SUPERVISOR_H_
#define INC_APP_TASK_SUPERVISOR_H_

#include "main.h"
#include "main.h"
#include "FreeRTOS.h"
#include "task.h"
#include "stdbool.h"

typedef enum {
    STATE_INIT,
    STATE_RUNNING,
    STATE_SENSOR_DEGRADED,
    STATE_STORAGE_DEGRADED,
    STATE_FATAL_ERROR
} SystemState_t;


typedef enum {
    SYS_OK = 0,
    SYS_SENSOR_DEGRADED,
    SYS_SENSOR_FATAL,     // décidé par le Supervisor, pas envoyé directement
    SYS_STORAGE_DEGRADED,
    SYS_STORAGE_FATAL,    // idem
} SystemHealthEvent_t;

typedef enum {
    HEALTH_SRC_SENSOR,
    HEALTH_SRC_STORAGE
} HealthSource_t;

typedef struct {
	HealthSource_t source;
	SystemHealthEvent_t event;
    uint8_t last_error;    // ou SD_Status_t selon la source — point à trancher, voir plus bas
} HealthReport_t;



#define DEGRADED_TO_FATAL_MS   30000
#define STREAK_TO_RECOVER_Sensor   3
#define STREAK_TO_RECOVER_storage   1

extern SystemState_t current_state;

void Task_Supervisor(void *pvParameters);


#endif /* INC_APP_TASK_SUPERVISOR_H_ */
