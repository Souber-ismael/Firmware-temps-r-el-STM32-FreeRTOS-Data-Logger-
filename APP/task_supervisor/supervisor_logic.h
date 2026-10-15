/**
 * @file    supervisor_logic.h
 * @brief   Machine d'etat du superviseur, sans dependance FreeRTOS/HAL.
 *
 * @details Ce module est volontairement portable (stdint uniquement) pour
 *          pouvoir etre teste sur PC avec Unity + GCC.  Les temps sont
 *          exprimes en millisecondes (configTICK_RATE_HZ = 1000 sur la cible).
 */

#ifndef TASK_SUPERVISOR_SUPERVISOR_LOGIC_H_
#define TASK_SUPERVISOR_SUPERVISOR_LOGIC_H_

#include <stdint.h>
#include <stdio.h>

/**
 * @brief  Etats globaux du systeme surveilles par Task_Supervisor.
 *
 * @details Transitions possibles :
 *          STATE_RUNNING → STATE_SENSOR_DEGRADED  → STATE_FATAL_ERROR
 *          STATE_RUNNING → STATE_STORAGE_DEGRADED → STATE_FATAL_ERROR
 *          STATE_*_DEGRADED → STATE_RUNNING (si suffisamment de succes consecutifs)
 */
typedef enum {
    STATE_RUNNING,           /**< Systeme nominal, toutes les taches fonctionnent     */
    STATE_SENSOR_DEGRADED,   /**< Erreurs capteur detectees, mesures en mode degrade  */
    STATE_STORAGE_DEGRADED,  /**< Erreurs SD detectees, ecriture en mode degrade      */
    STATE_FATAL_ERROR        /**< Etat degrade depasse le timeout, systeme bloque      */
} SystemState_t;

/**
 * @brief  Evenements de sante envoyes par les taches via health_queue.
 */
typedef enum {
    SYS_OK = 0,          /**< Operation reussie (capteur ou stockage)                 */
    SYS_SENSOR_DEGRADED, /**< Erreur capteur signalee par Task_Main                   */
    SYS_STORAGE_DEGRADED /**< Erreur SD signalee par Task_Storage                     */
} SystemHealthEvent_t;

/**
 * @brief  Sources d'un rapport de sante.
 */
typedef enum {
    HEALTH_SRC_SENSOR,  /**< Rapport emis par Task_Main (capteur AHT20) */
    HEALTH_SRC_STORAGE  /**< Rapport emis par Task_Storage (microSD)    */
} HealthSource_t;

/**
 * @brief  Structure d'un rapport de sante envoye dans health_queue.
 */
typedef struct {
    HealthSource_t      source;      /**< Tache emettrice du rapport              */
    SystemHealthEvent_t event;       /**< Resultat de l'operation (OK ou erreur)  */
    uint8_t             last_error;  /**< Code d'erreur brut                      */
} HealthReport_t;

/** Duree en ms avant de passer d'un etat degrade a STATE_FATAL_ERROR (40 s). */
#define DEGRADED_TO_FATAL_MS        40000u

/** Nombre de mesures capteur OK consecutives pour revenir a STATE_RUNNING. */
#define STREAK_TO_RECOVER_Sensor    3u

/** Nombre d'ecritures SD OK consecutives pour revenir a STATE_RUNNING. */
#define STREAK_TO_RECOVER_storage   1u

/** @brief Etat global du systeme, mis a jour par le superviseur. */
extern volatile SystemState_t current_state;

/**
 * @brief  Remet la machine d'etat dans son etat initial (STATE_RUNNING).
 */
void supervisor_reset(void);

/**
 * @brief  Applique un rapport de sante a la machine d'etat.
 *
 * @param[in] report  Rapport recu (ignore si NULL).
 * @param[in] now_ms  Horloge monotone en millisecondes.
 */
void supervisor_process_event(const HealthReport_t *report, uint32_t now_ms);

/**
 * @brief  Evalue recuperation (streak OK) et timeout degrade → fatal.
 *
 * @param[in] now_ms  Horloge monotone en millisecondes.
 */
void supervisor_update(uint32_t now_ms);

#endif /* TASK_SUPERVISOR_SUPERVISOR_LOGIC_H_ */
