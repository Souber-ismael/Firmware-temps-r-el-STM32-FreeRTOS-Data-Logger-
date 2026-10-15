/**
 * @file    app_task_supervisor.c
 * @brief   Tache FreeRTOS superviseur : files, watchdog, appel de la logique.
 */

#include "app_task_supervisor.h"

void Task_Supervisor(void *pvParameters)
{
    HealthReport_t report;

    (void)pvParameters;
    supervisor_reset();


    for (;;) {
        switch (current_state) {

        case STATE_RUNNING:
        case STATE_SENSOR_DEGRADED:
        case STATE_STORAGE_DEGRADED:
            if (xQueueReceive(health_queue, &report, pdMS_TO_TICKS(100)) == pdTRUE) {
                supervisor_process_event(&report, xTaskGetTickCount());
            }
            watchdog_refresh();
            supervisor_update(xTaskGetTickCount());
            break;

        case STATE_FATAL_ERROR:
            watchdog_refresh();
            vTaskDelay(pdMS_TO_TICKS(2000));
            break;

        default:
            vTaskDelay(pdMS_TO_TICKS(3000));
            break;
        }
    }
}

void watchdog_refresh(void)
{
    EventBits_t bits = xEventGroupWaitBits(
        wdEventGroup,
        WD_ALL_BITS,
        pdTRUE,
        pdTRUE,
        pdMS_TO_TICKS(2000)
    );

    if ((bits & WD_ALL_BITS) == WD_ALL_BITS) {
        HAL_IWDG_Refresh(&hiwdg);
    }
}
