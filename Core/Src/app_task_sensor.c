/*
 * app_task.c
 *
 *  Created on: Oct 6, 2026
 *      Author: admin
 */

#include "app_task_sensor.h"
#include "string.h"


void Task_Main(void *argument)
{
    char msg[64];
    syste s1;



    s1.st = AHT20_Init();
    if (s1.st != SENSOR_STATUS_OK)
    {
        snprintf(msg, sizeof(msg), "[ERR] AHT20_Init = %d\r\n", (uint8_t)s1.st);
        UART_TX_send_string(&huart1, msg);
        vTaskSuspend(NULL);   /* bloque sans boucle d'erreur */
    }


    for (;;)
    {
    	s1.st = AHT20_Read(&s1.temp_int, &s1.temp_frac, &s1.hum_int, &s1.hum_frac);

        if (s1.st == SENSOR_STATUS_OK)
        {
            snprintf(msg, sizeof(msg),
                     "T: %d.%02d C  H: %d.%02d %%\r\n",
                     s1.temp_int, s1.temp_frac,
					 s1.hum_int,  s1.hum_frac);
        }
        else
        {
            snprintf(msg, sizeof(msg), "[ERR] AHT20_Read = %d\r\n", (uint8_t)s1.st);
        }

        UART_TX_send_string(&huart1, msg);

        xQueueSend(queue_sample, &s1, 0);

        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
