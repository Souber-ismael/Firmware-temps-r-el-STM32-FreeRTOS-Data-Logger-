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


char* get_storage_status_name(uint8_t r1) {
	switch (r1) {
	case SD_OK:
		return "Storage_ok";
	case SD_ERROR:
		return "Storage_spi_error";
	case SD_TIMEOUT:
		return "Storage_spi_timeout";
	case SD_BUSY:
		return "Storage_spi_busy";
	case SD_NOT_READY :
		return "Storage_not_ready";
	case SD_ERROR_TIMEOUT:
		return "Storage_sd_timeout";
	case SD_ERR_DATA_TOKEN:
			return "Storage_err_data";
	case SD_ERROR_CDM:
			return "Storage_err_cmd";
	case SD_ERR_RESPONSE:
			return "Storage_err_response";
	case SD_ERR_WRITE_TIMEOUT:
			return "Storage_err_write";
	case SD_NOT_FOUND :
			return "Storage_not_found";

	default:
		return "Sensor_unknown";

	}
}


void Task_Storage(void *pvParameters) {

	SD_Init();
	Storage_Init();

	syste sample;
	HealthReport_t report;
	char *c;

	report.source = HEALTH_SRC_STORAGE;

	for (;;) {
		switch (current_state) {

		case STATE_RUNNING:
			if (xQueueReceive(queue_sample, &sample, 100) == pdTRUE) {
				if (sample.st == SENSOR_STATUS_OK) {
					memcpy(&write_buffer[buffer_offset], &sample, sizeof(syste));
					buffer_offset += sizeof(syste);

					if (buffer_offset + sizeof(syste) > 512) {
						function(&report);
						xQueueSend(health_queue, &report, 0);
					}
				}
			}
			// ← Toujours set le bit, même si pas de sample
			xEventGroupSetBits(wdEventGroup, WD_BIT_STORAGE);
			break;

		case STATE_SENSOR_DEGRADED:
			xEventGroupSetBits(wdEventGroup, WD_BIT_STORAGE);
			HAL_GPIO_TogglePin(GPIOA, GPIO_PIN_6);
			vTaskDelay(1000);
			break;

		case STATE_STORAGE_DEGRADED:
			function(&report);
			xQueueSend(health_queue, &report, 0);

			if (report.event != SYS_OK) {
				c = get_storage_status_name(report.last_error);
				UART_TX_send_string(&huart1, c);
			}

			xEventGroupSetBits(wdEventGroup, WD_BIT_STORAGE);
			vTaskDelay(5000);
			break;

		case STATE_FATAL_ERROR:
			xEventGroupSetBits(wdEventGroup, WD_BIT_STORAGE);
			HAL_GPIO_TogglePin(GPIOA, GPIO_PIN_6);
			vTaskDelay(2000);
			break;

		default:
			xEventGroupSetBits(wdEventGroup, WD_BIT_STORAGE);  // ← important
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

