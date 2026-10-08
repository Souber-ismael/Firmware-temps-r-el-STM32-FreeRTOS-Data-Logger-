/*
 * app_task.c
 *
 *  Created on: Oct 6, 2026
 *      Author: admin
 */

#include "app_task_sensor.h"
#include "string.h"

void funcprepar(syste s1, HealthReport_t *report, char *s, size_t size);

char* get_sd_status_name(void) {
	switch (current_state) {
	case STATE_RUNNING:
		return "SYS_OK";
	case STATE_SENSOR_DEGRADED:
		return "SYS_degraded sensor";
	case STATE_STORAGE_DEGRADED:
		return "SYS_degraded storage";
	case STATE_FATAL_ERROR:
		return "SYS_fatal";
	default:
		return "SYS_unknown";
	}
}
void Task_Main(void *argument) {
	char msg[64];
	syste s1;
	HealthReport_t report;

	s1.st = AHT20_Init();
	if (s1.st != SENSOR_STATUS_OK) {
		snprintf(msg, sizeof(msg), "[ERR] AHT20_Init = %d\r\n",
				(uint8_t) s1.st);
		UART_TX_send_string(&huart1, msg);
		vTaskSuspend(NULL); /* bloque sans boucle d'erreur */
	}

	report.source = HEALTH_SRC_SENSOR;

	for (;;) {

		switch (current_state) {
		case STATE_RUNNING:
			s1.st = AHT20_Read(&s1.temp_int, &s1.temp_frac, &s1.hum_int,
					&s1.hum_frac);

			funcprepar(s1, &report, msg, sizeof(msg));

			UART_TX_send_string(&huart1, msg);

			xQueueSend(health_queue, &report, 0);

			xQueueSend(queue_sample, &s1, 0);

			vTaskDelay(pdMS_TO_TICKS(1000));
			break;

		case STATE_SENSOR_DEGRADED:
			s1.st = AHT20_Read(&s1.temp_int, &s1.temp_frac, &s1.hum_int,
					&s1.hum_frac);

			funcprepar(s1, &report, msg, sizeof(msg));

			UART_TX_send_string(&huart1, msg);

			xQueueSend(health_queue, &report, 0);

			xQueueSend(queue_sample, &s1, 0);

			vTaskDelay(pdMS_TO_TICKS(2000));
			break;

		case STATE_STORAGE_DEGRADED:

			UART_TX_send_string(&huart1, "state storage degraded \r\n");
			vTaskDelay(pdMS_TO_TICKS(3000));
			break;
		case STATE_FATAL_ERROR:
			UART_TX_send_string(&huart1, "state :FATAL \r\n");

			vTaskDelay(pdMS_TO_TICKS(3000));
			break;
		default:
			UART_TX_send_string(&huart1, "state :default \r\n");
			vTaskDelay(pdMS_TO_TICKS(3000));
			break;
		}

	}
}

void funcprepar(syste s1, HealthReport_t *report, char *s, size_t size) {
	char *a = get_sd_status_name();
	if (s1.st == SENSOR_STATUS_OK) {
		report->event = SYS_OK;
		report->last_error = 0;

		snprintf(s, size, "%s T: %d.%02d C  H: %d.%02d %%\r\n", a, s1.temp_int,
				s1.temp_frac, s1.hum_int, s1.hum_frac);
	} else {
		report->event = SYS_SENSOR_DEGRADED;
		report->last_error = (uint8_t) s1.st;

		snprintf(s, size, "%s AHT20_Read = %d\r\n", a, (uint8_t) s1.st);
	}
}
