/*
 * app_task_supervisor.c
 *
 *  Created on: Oct 6, 2026
 *      Author: admin
 */

#include "app_task_supervisor.h"

SystemState_t current_state = STATE_RUNNING;

static uint8_t sensor_last_error;
static uint8_t storage_last_error;

static bool sensor_ok;
static bool storage_ok = true;

static uint32_t sensor_ok_count = 0;
static uint32_t storage_ok_count = 0;

static TickType_t sensor_degraded_since = 0;
static TickType_t storage_degraded_since = 0;

static void Supervisor_ProcessEvent(const HealthReport_t *report);
static void Supervisor_UpdateState(void);
static void Supervisor_CheckTimeouts(void);
static void supervisor_ok_sensor(void);
static void supervisor_ok_storage(void);

void Task_Supervisor(void *pvParameters) {
	HealthReport_t report;

	for (;;) {
		switch (current_state) {
				case STATE_RUNNING:
					if (xQueueReceive(health_queue, &report, pdMS_TO_TICKS(100)) == pdTRUE) {
								Supervisor_ProcessEvent(&report);
							}

							Supervisor_UpdateState();
					break;

				case STATE_SENSOR_DEGRADED:
					if (xQueueReceive(health_queue, &report, pdMS_TO_TICKS(100)) == pdTRUE) {
								Supervisor_ProcessEvent(&report);
							}

							Supervisor_UpdateState();

					break;

				case STATE_STORAGE_DEGRADED:
					if (xQueueReceive(health_queue, &report, pdMS_TO_TICKS(100)) == pdTRUE) {
								Supervisor_ProcessEvent(&report);
							}

							Supervisor_UpdateState();

					break;
				case STATE_FATAL_ERROR:
                      vTaskDelay(pdMS_TO_TICKS(3000));
					break;
				default:
					vTaskDelay(pdMS_TO_TICKS(3000));
					break;
				}


	}
}

static void Supervisor_ProcessEvent(const HealthReport_t *report) {
	switch (report->source) {
	case HEALTH_SRC_SENSOR:

		if (report->event == SYS_OK) {
			sensor_ok = true;
			sensor_ok_count++;
		} else {
			sensor_ok = false;
			sensor_ok_count = 0;
			sensor_last_error = report->last_error;
			if(current_state != STATE_STORAGE_DEGRADED)
			current_state = STATE_SENSOR_DEGRADED;
			if(sensor_degraded_since == 0){
				sensor_degraded_since = xTaskGetTickCount();
			}
		}

		break;

	case HEALTH_SRC_STORAGE:

		if (report->event == SYS_OK) {
			storage_ok = true;
			storage_ok_count++;
		} else {
			storage_ok = false;
			storage_ok_count = 0;
			storage_last_error = report->last_error;
			if(current_state != STATE_SENSOR_DEGRADED)
			   current_state = STATE_STORAGE_DEGRADED;
			if(storage_degraded_since == 0)
			storage_degraded_since = xTaskGetTickCount();
		}

		break;

	default:
		break;
	}
}

static void Supervisor_UpdateState(void) {



     switch (current_state) {
		case STATE_SENSOR_DEGRADED:
			supervisor_ok_sensor();
			Supervisor_CheckTimeouts();
			break;

		case STATE_STORAGE_DEGRADED:
			supervisor_ok_storage();
			Supervisor_CheckTimeouts();
					break;
		case STATE_FATAL_ERROR:

			vTaskDelay(10000);
			break;
		default:
			break;
	}

}


static void supervisor_ok_sensor(void){
	if (sensor_ok_count >= STREAK_TO_RECOVER_Sensor) {
				current_state = STATE_RUNNING;
				sensor_degraded_since = 0;

				return;
			}
}

static void supervisor_ok_storage(void){
	if(storage_ok_count >= STREAK_TO_RECOVER_storage){
				current_state = STATE_RUNNING;
				storage_degraded_since = 0;
				return;
	}
}

static void Supervisor_CheckTimeouts(void)
{


    if (sensor_degraded_since != 0 &&
        (xTaskGetTickCount() - sensor_degraded_since) >= pdMS_TO_TICKS(DEGRADED_TO_FATAL_MS))
    {
        current_state = STATE_FATAL_ERROR;
    }

    if (storage_degraded_since != 0 &&
        (xTaskGetTickCount() - storage_degraded_since) >= pdMS_TO_TICKS(DEGRADED_TO_FATAL_MS))
    {
        current_state = STATE_FATAL_ERROR;
    }
}
