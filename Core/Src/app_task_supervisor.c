/*
 * app_task_supervisor.c
 *
 *  Created on: Oct 6, 2026
 *      Author: admin
 */

#include "app_task_supervisor.h"

SystemState_t current_state = STATE_INIT;

static uint8_t sensor_last_error;
static uint8_t storage_last_error;

static bool sensor_ok;
static bool storage_ok;

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
            }
            else
            {
                sensor_ok = false;
                sensor_last_error = report->last_error;
            }

            break;


        case HEALTH_SRC_STORAGE:

            if (report->event == SYS_OK)
            {
                storage_ok = true;
            }
            else
            {
                storage_ok = false;
                storage_last_error = report->last_error;
            }

            break;


        default:
            break;
    }
}

static void Supervisor_UpdateState(void)
{
    if (sensor_ok && storage_ok)
    {
    	current_state = STATE_RUNNING;
    }
    else
    {
    	current_state = STATE_BOTH_DEGRADED;
    }
}





