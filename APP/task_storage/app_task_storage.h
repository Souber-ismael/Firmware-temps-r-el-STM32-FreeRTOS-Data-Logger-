/**
 * @file    app_task_storage.h
 * @brief   Interface de la tache de stockage SD (Task_Storage).
 *
 * @details Cette tache recoit les mesures depuis queue_sample,
 *          les accumule dans un buffer de 512 octets, puis ecrit
 *          un bloc complet sur la microSD via la couche storage.
 *          Elle rapporte son etat via health_queue vers Task_Supervisor.
 */

#ifndef INC_APP_TASK_STORAGE_H_
#define INC_APP_TASK_STORAGE_H_

#include "main.h"
#include "FreeRTOS.h"
#include "task.h"
#include "storage.h"
#include "app_task_sensor.h"
#include "app_task_supervisor.h"
#include "string.h"
#include "watchdog.h"

/**
 * @brief  Tache FreeRTOS d'ecriture des mesures sur la microSD.
 *
 * @details Sequence nominale (STATE_RUNNING) :
 *          1. Attend une mesure dans queue_sample (timeout 100 ticks).
 *          2. Si la mesure est valide (st == SENSOR_STATUS_OK),
 *             la copie dans write_buffer a buffer_offset.
 *          3. Quand write_buffer est plein (512 octets),
 *             appelle Storage_FlushBlock() pour ecrire sur SD.
 *          4. Set WD_BIT_STORAGE dans wdEventGroup.
 *
 *          En etats degrade/fatal, set WD_BIT_STORAGE et attend
 *          pour laisser les autres taches progresser.
 *
 * @param[in] pvParameters  Non utilise (NULL).
 */
void Task_Storage(void *argument);

#endif /* INC_APP_TASK_STORAGE_H_ */