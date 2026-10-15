/**
 * @file    supervisor_logic.c
 * @brief   Implementation portable de la machine d'etat superviseur.
 */

#include "supervisor_logic.h"
#include <stdbool.h>

volatile SystemState_t current_state = STATE_RUNNING;

static uint32_t sensor_ok_count = 0;
static uint32_t storage_ok_count = 0;
static uint32_t sensor_degraded_since = 0;
static uint32_t storage_degraded_since = 0;
static bool sensor_timer_active = false;
static bool storage_timer_active = false;

static void supervisor_ok_sensor(void);
static void supervisor_ok_storage(void);
static void supervisor_check_timeouts(uint32_t now_ms);

void supervisor_reset(void)
{
    current_state = STATE_RUNNING;
    sensor_ok_count = 0;
    storage_ok_count = 0;
    sensor_degraded_since = 0;
    storage_degraded_since = 0;
    sensor_timer_active = false;
    storage_timer_active = false;
}

void supervisor_process_event(const HealthReport_t *report, uint32_t now_ms)
{
    if (report == NULL) {
        return;
    }

    switch (report->source) {

    case HEALTH_SRC_SENSOR:
        if (report->event == SYS_OK) {
            sensor_ok_count++;
        } else {
            sensor_ok_count = 0;

            if (current_state != STATE_STORAGE_DEGRADED) {
                current_state = STATE_SENSOR_DEGRADED;
            }

            if (!sensor_timer_active) {
                sensor_degraded_since = now_ms;
                sensor_timer_active = true;
            }
        }
        break;

    case HEALTH_SRC_STORAGE:

        if (report->event == SYS_OK) {
            storage_ok_count++;
        } else {
            storage_ok_count = 0;

            if (current_state != STATE_SENSOR_DEGRADED) {
                current_state = STATE_STORAGE_DEGRADED;

            }
            if (!storage_timer_active) {
                storage_degraded_since = now_ms;
                storage_timer_active = true;
            }
        }
        break;

    default:
        break;
    }
}

void supervisor_update(uint32_t now_ms)
{
    switch (current_state) {

    case STATE_SENSOR_DEGRADED:
        supervisor_ok_sensor();
        supervisor_check_timeouts(now_ms);
        break;

    case STATE_STORAGE_DEGRADED:
        supervisor_ok_storage();
        supervisor_check_timeouts(now_ms);
        break;

    default:
        break;
    }
}

static void supervisor_ok_sensor(void)
{
    if (sensor_ok_count >= STREAK_TO_RECOVER_Sensor) {
        current_state         = STATE_RUNNING;
        sensor_degraded_since = 0;
        sensor_timer_active   = false;
    }
}

static void supervisor_ok_storage(void)
{
    if (storage_ok_count >= STREAK_TO_RECOVER_storage) {
        current_state          = STATE_RUNNING;
        storage_degraded_since = 0;
        storage_timer_active   = false;
    }
}

static void supervisor_check_timeouts(uint32_t now_ms)
{
    if (sensor_timer_active &&
        (now_ms - sensor_degraded_since) >= DEGRADED_TO_FATAL_MS)
    {
        current_state = STATE_FATAL_ERROR;
    }

    if (storage_timer_active &&
        (now_ms - storage_degraded_since) >= DEGRADED_TO_FATAL_MS)
    {
        current_state = STATE_FATAL_ERROR;
    }
}
