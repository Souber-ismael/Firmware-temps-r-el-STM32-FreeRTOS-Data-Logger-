/*
 * app_task.c
 *
 *  Created on: Oct 6, 2026
 *      Author: admin
 */

#include "app_task_sensor.h"
#include "string.h"

const char* get_sd_status_name(void)
{
    switch (current_state)
    {
        case STATE_RUNNING:        return "SYS_OK";
        case STATE_SENSOR_DEGRADED:  return "SYS_degraded";
        case STATE_BOTH_DEGRADED:     return "SYS_error";
        case STATE_FATAL_ERROR:      return "SYS_fatal";
        default:              return "SYS_unknown";
    }
}
void Task_Main(void *argument)
{
    char msg[64];
    syste s1;
    HealthReport_t report;



    s1.st = AHT20_Init();
    if (s1.st != SENSOR_STATUS_OK)
    {
        snprintf(msg, sizeof(msg), "[ERR] AHT20_Init = %d\r\n", (uint8_t)s1.st);
        UART_TX_send_string(&huart1, msg);
        vTaskSuspend(NULL);   /* bloque sans boucle d'erreur */
    }

       report.source =HEALTH_SRC_SENSOR;

    for (;;)
    {
    	const char *a =get_sd_status_name();

    	s1.st = AHT20_Read(&s1.temp_int, &s1.temp_frac, &s1.hum_int, &s1.hum_frac);

        if (s1.st == SENSOR_STATUS_OK)
        {
            report.event=SYS_OK;
            report.last_error=0;
            snprintf(msg, sizeof(msg),
                     "T: %d.%02d C  H: %d.%02d %% State:%s\r\n",
                     s1.temp_int, s1.temp_frac,
					 s1.hum_int,  s1.hum_frac , a);
        }
        else
        {
        	report.event=SYS_SENSOR_DEGRADED;
        	report.last_error=(uint8_t)s1.st;
            snprintf(msg, sizeof(msg), "[ERR] AHT20_Read = %d state:%s\r\n", (uint8_t)s1.st ,  a);
        }

        UART_TX_send_string(&huart1, msg);

        xQueueSend(health_queue, &report, 0);

        xQueueSend(queue_sample, &s1, 0);

        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
