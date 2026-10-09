/**
 * @file    app_task_supervisor.h
 * @brief   Interface de la tache superviseur et des types partages entre taches.
 *
 * @details Ce fichier definit :
 *          - La machine d'etat systeme (@ref SystemState_t).
 *          - Les evenements de sante rapportes par les taches (@ref SystemHealthEvent_t).
 *          - La structure de rapport de sante (@ref HealthReport_t).
 *          - Les seuils de recuperation et de declenchement fatal.
 *          - Le prototype de la tache superviseur (@ref Task_Supervisor).
 *
 *          La variable globale @ref current_state est lue par toutes les taches
 *          pour adapter leur comportement a l'etat du systeme.
 */

#ifndef INC_APP_TASK_SUPERVISOR_H_
#define INC_APP_TASK_SUPERVISOR_H_

#include "main.h"
#include "FreeRTOS.h"
#include "task.h"
#include "stdbool.h"
#include "watchdog.h"
#include "uart_ll.h"

/**
 * @brief  Etats globaux du systeme surveilles par Task_Supervisor.
 *
 * @details La machine d'etat evolue selon les rapports recus via health_queue.
 *          Transitions possibles :
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
 *
 * @details Chaque tache envoie un rapport apres chaque operation.
 *          Le superviseur consomme ces rapports pour mettre a jour
 *          l'etat systeme et decider de nourrir ou non l'IWDG.
 */
typedef struct {
    HealthSource_t      source;      /**< Tache emettrice du rapport              */
    SystemHealthEvent_t event;       /**< Resultat de l'operation (OK ou erreur)  */
    uint8_t             last_error;  /**< Code d'erreur brut (SD_Status ou SensorStatus cast en uint8_t) */
} HealthReport_t;

/** @defgroup Supervisor_Thresholds Seuils de la machine d'etat superviseur
 *  @{
 */
/** Duree en ms avant de passer d'un etat degrade a STATE_FATAL_ERROR (40 s). */
#define DEGRADED_TO_FATAL_MS        40000

/** Nombre de mesures capteur OK consecutives pour revenir a STATE_RUNNING. */
#define STREAK_TO_RECOVER_Sensor    3

/** Nombre d'ecritures SD OK consecutives pour revenir a STATE_RUNNING. */
#define STREAK_TO_RECOVER_storage   1
/** @} */

/** @brief Etat global du systeme, mis a jour par Task_Supervisor et lu par toutes les taches. */
extern SystemState_t current_state;

/**
 * @brief  Tache FreeRTOS de supervision et de gestion du watchdog.
 *
 * @details Cette tache :
 *          - Recoit les rapports de sante depuis health_queue.
 *          - Met a jour la machine d'etat systeme.
 *          - En STATE_RUNNING : raffraichit l'IWDG directement.
 *          - En etat degrade : utilise watchdog_refresh() qui verifie
 *            d'abord que toutes les taches ont bien set leurs bits watchdog.
 *          - Gere les transitions vers STATE_FATAL_ERROR apres timeout.
 *
 * @param[in] pvParameters  Non utilise (NULL).
 */
void Task_Supervisor(void *pvParameters);

/**
 * @brief  Raffraichit l'IWDG uniquement si toutes les taches sont vivantes.
 *
 * @details Attend (max 2 s) que WD_BIT_SENSOR ET WD_BIT_STORAGE soient
 *          simultanement leves dans wdEventGroup.  Si les deux bits sont
 *          presents, l'IWDG est nourri.  Sinon, l'IWDG n'est pas raffraichi
 *          et expirera apres ~10 s si aucune autre tache ne le nourrit.
 *
 * @note    Appelee depuis Task_Supervisor en etats degrade et fatal.
 */
void watchdog_refresh(void);

#endif /* INC_APP_TASK_SUPERVISOR_H_ */