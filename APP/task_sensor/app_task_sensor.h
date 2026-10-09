/**
 * @file    app_task_sensor.h
 * @brief   Interface de la tache capteur (Task_Main) et definition du type syste.
 *
 * @details Ce fichier expose :
 *          - La structure @ref syste qui transporte une mesure AHT20 complete.
 *          - Le prototype de @ref Task_Main.
 *
 *          La tache lit le capteur, affiche les valeurs sur UART, et publie
 *          les donnees dans queue_sample (vers Task_Storage) et health_queue
 *          (vers Task_Supervisor).
 */

#ifndef INC_APP_TASK_SENSOR_H_
#define INC_APP_TASK_SENSOR_H_

#include "aht20.h"
#include "uart_ll.h"
#include "FreeRTOS.h"
#include "task.h"
#include "main.h"
#include "app_task_supervisor.h"
#include "watchdog.h"

/**
 * @brief  Donnees d'une mesure AHT20 transmises entre les taches.
 *
 * @details Les valeurs de temperature et d'humidite sont stockees sans
 *          virgule flottante : partie entiere + partie fractionnaire sur
 *          2 chiffres.  Le champ st indique si la mesure est valide.
 *
 * @note    Taille : 5 octets. Aligne naturellement sur 1 octet.
 */
typedef struct {
    uint8_t      temp_int;   /**< Partie entiere de la temperature  (ex : 23 pour 23.45 C) */
    uint8_t      temp_frac;  /**< Fraction temperature sur 2 chiffres (ex : 45 pour 23.45) */
    uint8_t      hum_int;    /**< Partie entiere de l'humidite      (ex : 58 pour 58.12 %) */
    uint8_t      hum_frac;   /**< Fraction humidite sur 2 chiffres  (ex : 12 pour 58.12)   */
    SensorStatus st;         /**< Code de retour AHT20_Read pour cette mesure              */
} syste;

/**
 * @brief  Tache FreeRTOS de lecture du capteur AHT20.
 *
 * @details Comportement selon l'etat systeme current_state :
 *          - STATE_RUNNING          : lit, affiche UART, envoie queue, delai 1 s.
 *          - STATE_SENSOR_DEGRADED  : lit, affiche l'erreur UART, delai 2 s.
 *          - STATE_STORAGE_DEGRADED : set le bit WD seulement, delai 2 s.
 *          - STATE_FATAL_ERROR      : affiche "FATAL", set WD, delai 2 s.
 *
 *          Dans tous les etats actifs, set WD_BIT_SENSOR dans wdEventGroup
 *          pour signaler sa vivacite au superviseur watchdog.
 *
 * @param[in] argument  Non utilise (NULL).
 */
void Task_Main(void *argument);

#endif /* INC_APP_TASK_SENSOR_H_ */