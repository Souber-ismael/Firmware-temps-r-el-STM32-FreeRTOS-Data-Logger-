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

static TickType_t degraded_since = 0;

void Task_Supervisor(void *pvParameters)
{
    HealthReport_t report;


    for (;;)
    {
        if (xQueueReceive(health_queue,
                          &report,
                          pdMS_TO_TICKS(100)) == pdTRUE)
        {
            Supervisor_ProcessEvent(&report);
        }

        Supervisor_UpdateState();
    }
}


static void Supervisor_ProcessEvent(const HealthReport_t *report)
{
    switch (report->source)
    {
        case HEALTH_SRC_SENSOR:

            if (report->event == SYS_OK)
            {
                sensor_ok = true;
                sensor_ok_count++;
            }
            else
            {
                sensor_ok = false;
                sensor_ok_count = 0;
                sensor_last_error = report->last_error;
            }

            break;


        case HEALTH_SRC_STORAGE:

            if (report->event == SYS_OK)
            {
                storage_ok = true;
                storage_ok_count++;
            }
            else
            {
                storage_ok = false;
                storage_ok_count = 0;
                storage_last_error = report->last_error;
            }

            break;


        default:
            break;
    }
}

static void Supervisor_UpdateState(void)
{

	 if (current_state == STATE_RUNNING)
	    {
	        if (!sensor_ok || !storage_ok)
	        {
	        	current_state = STATE_SENSOR_DEGRADED;

	            degraded_since = xTaskGetTickCount();

	            sensor_ok_count = 0;
	            storage_ok_count = 0;
	        }

	        return;
	    }

	 if (current_state == STATE_SENSOR_DEGRADED)
	     {
	         if ((sensor_ok_count >= OK_STREAK_TO_RECOVER))
	         {
	        	 current_state = STATE_RUNNING;
	             degraded_since = 0;

	             return;
	         }

	         if ((xTaskGetTickCount() - degraded_since) >=
	                     pdMS_TO_TICKS(DEGRADED_TO_FATAL_MS))
	                 {
	        	     current_state  = STATE_FATAL_ERROR;
	                 }
	     }
}





