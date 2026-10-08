/*
 * app_task_storage.c
 *
 *  Created on: Oct 6, 2026
 *      Author: admin
 */

#include "app_task_storage.h"

static uint8_t write_buffer[512];
static uint16_t buffer_offset = 0;
void function(HealthReport_t *report);

void Task_Storage(void *pvParameters) {

	SD_Init();
	Storage_Init();

	syste sample;
	HealthReport_t report;

	report.source = HEALTH_SRC_STORAGE;
	for (;;) {
		switch (current_state) {
		case STATE_RUNNING:
			if (xQueueReceive(queue_sample, &sample, 100) == pdTRUE) {
				if (sample.st != SENSOR_STATUS_OK)
					continue;
				memcpy(&write_buffer[buffer_offset], &sample, sizeof(syste));
				buffer_offset += sizeof(syste);
				if (buffer_offset + sizeof(syste) > 512) {
					function(&report);
					xQueueSend(health_queue, &report, 0);
				}
			}
			break;
		case STATE_SENSOR_DEGRADED:
			HAL_GPIO_TogglePin(GPIOA, GPIO_PIN_6);
			vTaskDelay(1000);
			break;

		case STATE_STORAGE_DEGRADED:
			function(&report);
			if(report.event = SYS_OK){
			xQueueSend(health_queue, &report, 0);
			     break;
			}
			xQueueSend(health_queue, &report, 0);
			vTaskDelay(5000);
			break;
		case STATE_FATAL_ERROR:
			HAL_GPIO_TogglePin(GPIOA, GPIO_PIN_6);
			vTaskDelay(10000);
			break;

		default:
			HAL_GPIO_TogglePin(GPIOA, GPIO_PIN_6);
			vTaskDelay(2000);
			break;
		}
	}
}

	void function(HealthReport_t *report) {

		SD_Status ret;

		ret = Storage_WriteSampleBlock(write_buffer);
		if (ret == SD_OK) {
			report->event = SYS_OK;
			report->last_error = (uint8_t) ret;
			buffer_offset = 0;
			memset(write_buffer, 0xFF, 512);
			HAL_GPIO_TogglePin(GPIOA, GPIO_PIN_5);
		} else {
			report->event = SYS_STORAGE_DEGRADED;
			report->last_error = (uint8_t) ret;
		}
	}

