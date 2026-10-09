/**
 * @file    app_task_supervisor.c
 * @brief   Implementation de la tache superviseur et de la machine d etat systeme.
 *
 * @details La tache Task_Supervisor tourne en permanence et :
 *          1. Consomme les rapports de sante depuis health_queue.
 *          2. Met a jour current_state selon les evenements recus.
 *          3. Raffraichit l IWDG directement (STATE_RUNNING) ou via
 *             watchdog_refresh() (etats degrade/fatal).
 *          4. Gere les transitions vers STATE_FATAL_ERROR apres DEGRADED_TO_FATAL_MS.
 *          5. Permet la recuperation si les taches reviennent a un etat stable.
 */

#include "app_task_supervisor.h"

/** @brief Etat courant du systeme. Initialise a STATE_RUNNING au demarrage. */
SystemState_t current_state = STATE_RUNNING;

/** @brief Compteur de mesures capteur OK consecutives (pour recuperation). */
static uint32_t sensor_ok_count = 0;

/** @brief Compteur d ecritures SD OK consecutives (pour recuperation). */
static uint32_t storage_ok_count = 0;

/** @brief Tick FreeRTOS de la premiere erreur capteur (0 = pas d erreur active). */
static TickType_t sensor_degraded_since = 0;

/** @brief Tick FreeRTOS de la premiere erreur SD (0 = pas d erreur active). */
static TickType_t storage_degraded_since = 0;

/* Prototypes des fonctions statiques internes */
static void Supervisor_ProcessEvent(const HealthReport_t *report);
static void Supervisor_UpdateState(void);
static void Supervisor_CheckTimeouts(void);
static void supervisor_ok_sensor(void);
static void supervisor_ok_storage(void);

/**
 * @brief  Tache FreeRTOS de supervision et de gestion du watchdog.
 * @details Boucle principale qui lit health_queue, raffraichit l IWDG
 *          et met a jour la machine d etat selon l etat courant.
 * @param[in] pvParameters  Non utilise.
 */
void Task_Supervisor(void *pvParameters)
{
    HealthReport_t report;

    for (;;) {
        switch (current_state) {

        case STATE_RUNNING:
            /* En nominal : lit un rapport si disponible, nourrit l IWDG directement */
            if (xQueueReceive(health_queue, &report, pdMS_TO_TICKS(100)) == pdTRUE) {
                Supervisor_ProcessEvent(&report);
            }
            HAL_IWDG_Refresh(&hiwdg);
            Supervisor_UpdateState();
            break;

        case STATE_SENSOR_DEGRADED:
            /* En degrade capteur : lit rapport, raffraichit via event group (conditionnel) */
            if (xQueueReceive(health_queue, &report, pdMS_TO_TICKS(100)) == pdTRUE) {
                Supervisor_ProcessEvent(&report);
            }
            watchdog_refresh();
            Supervisor_UpdateState();
            break;

        case STATE_STORAGE_DEGRADED:
            /* En degrade SD : lit rapport, raffraichit via event group (conditionnel) */
            if (xQueueReceive(health_queue, &report, pdMS_TO_TICKS(100)) == pdTRUE) {
                Supervisor_ProcessEvent(&report);
            }
            watchdog_refresh();
            Supervisor_UpdateState();
            break;

        case STATE_FATAL_ERROR:
            /* Etat fatal : seul le watchdog logiciel decide si l IWDG est nourri */
            watchdog_refresh();
            vTaskDelay(pdMS_TO_TICKS(2000));
            break;

        default:
            vTaskDelay(pdMS_TO_TICKS(3000));
            break;
        }
    }
}

/**
 * @brief  Traite un rapport de sante recu depuis health_queue.
 * @details Met a jour les compteurs de succes/echec et les timestamps
 *          de debut de degradation.  Modifie current_state si une erreur
 *          est detectee (sans ecraser un etat degrade deja en cours).
 * @param[in] report  Rapport de sante emis par Task_Main ou Task_Storage.
 */
static void Supervisor_ProcessEvent(const HealthReport_t *report)
{
    switch (report->source) {

    case HEALTH_SRC_SENSOR:
        if (report->event == SYS_OK) {
            sensor_ok_count++;  /* Incremente pour suivre la recuperation */
        } else {
            sensor_ok_count = 0;
            /* Ne pas ecraser STATE_STORAGE_DEGRADED deja actif */
            if (current_state != STATE_STORAGE_DEGRADED) {
                current_state = STATE_SENSOR_DEGRADED;
            }
            /* Enregistre le debut de la degradation pour le timeout fatal */
            if (sensor_degraded_since == 0) {
                sensor_degraded_since = xTaskGetTickCount();
            }
        }
        break;

    case HEALTH_SRC_STORAGE:
        if (report->event == SYS_OK) {
            storage_ok_count++;  /* Incremente pour suivre la recuperation */
        } else {
            storage_ok_count = 0;
            /* Ne pas ecraser STATE_SENSOR_DEGRADED deja actif */
            if (current_state != STATE_SENSOR_DEGRADED) {
                current_state = STATE_STORAGE_DEGRADED;
            }
            if (storage_degraded_since == 0) {
                storage_degraded_since = xTaskGetTickCount();
            }
        }
        break;

    default:
        break;
    }
}

/**
 * @brief  Evalue si le systeme peut revenir a STATE_RUNNING ou passer en FATAL.
 * @details Appele a chaque iteration de Task_Supervisor en etats degrade.
 *          Verifie les seuils de recuperation et les timeouts fatals.
 */
static void Supervisor_UpdateState(void)
{
    switch (current_state) {

    case STATE_SENSOR_DEGRADED:
        supervisor_ok_sensor();       /* Tente la recuperation */
        Supervisor_CheckTimeouts();   /* Verifie si le fatal doit etre declenche */
        break;

    case STATE_STORAGE_DEGRADED:
        supervisor_ok_storage();
        Supervisor_CheckTimeouts();
        break;

    case STATE_FATAL_ERROR:
        /* En fatal : aucune recuperation, la tache attend */
        vTaskDelay(pdMS_TO_TICKS(10000));
        break;

    default:
        break;
    }
}

/**
 * @brief  Raffraichit l IWDG si et seulement si toutes les taches sont vivantes.
 * @details Attend max 2 s que WD_BIT_SENSOR et WD_BIT_STORAGE soient simultanement
 *          leves dans wdEventGroup.  Les bits sont effacees apres lecture.
 *          Si les deux bits ne sont pas arrives dans le delai, l IWDG n est pas
 *          nourri et le timeout materiel (~10 s) declenchera un reset.
 */
void watchdog_refresh(void)
{
    EventBits_t bits = xEventGroupWaitBits(
        wdEventGroup,
        WD_ALL_BITS,
        pdTRUE,                  /* Efface les bits apres lecture */
        pdTRUE,                  /* Attend TOUS les bits */
        pdMS_TO_TICKS(2000)
    );

    if ((bits & WD_ALL_BITS) == WD_ALL_BITS) {
        /* Toutes les taches ont signale leur vivacite : nourrit l IWDG */
        HAL_IWDG_Refresh(&hiwdg);
    }
    /* Sinon : au moins une tache ne repond plus, l IWDG ne sera pas nourri */
}

/**
 * @brief  Verifie si le capteur est suffisamment stable pour revenir en RUNNING.
 * @details Si sensor_ok_count >= STREAK_TO_RECOVER_Sensor, revient a STATE_RUNNING
 *          et remet sensor_degraded_since a 0.
 */
static void supervisor_ok_sensor(void)
{
    if (sensor_ok_count >= STREAK_TO_RECOVER_Sensor) {
        current_state         = STATE_RUNNING;
        sensor_degraded_since = 0;
    }
}

/**
 * @brief  Verifie si le stockage est suffisamment stable pour revenir en RUNNING.
 * @details Si storage_ok_count >= STREAK_TO_RECOVER_storage, revient a STATE_RUNNING
 *          et remet storage_degraded_since a 0.
 */
static void supervisor_ok_storage(void)
{
    if (storage_ok_count >= STREAK_TO_RECOVER_storage) {
        current_state          = STATE_RUNNING;
        storage_degraded_since = 0;
    }
}

/**
 * @brief  Verifie si un etat degrade a depasse le seuil de timeout fatal.
 * @details Si le capteur ou le stockage est degrade depuis plus de
 *          DEGRADED_TO_FATAL_MS millisecondes, passe en STATE_FATAL_ERROR.
 */
static void Supervisor_CheckTimeouts(void)
{
    if (sensor_degraded_since != 0 &&
        (xTaskGetTickCount() - sensor_degraded_since) >= pdMS_TO_TICKS(DEGRADED_TO_FATAL_MS)) {
        current_state = STATE_FATAL_ERROR;
    }

    if (storage_degraded_since != 0 &&
        (xTaskGetTickCount() - storage_degraded_since) >= pdMS_TO_TICKS(DEGRADED_TO_FATAL_MS)) {
        current_state = STATE_FATAL_ERROR;
    }
}