/**
 * @file    app_task_sensor.c
 * @brief   Implementation de la tache capteur AHT20 et des helpers d affichage.
 *
 * @details Lit le capteur AHT20 toutes les 1 s (ou 2 s en mode degrade),
 *          formate les donnees pour l UART, publie dans queue_sample
 *          (vers Task_Storage) et health_queue (vers Task_Supervisor),
 *          et set WD_BIT_SENSOR pour signaler sa vivacite.
 */

#include "app_task_sensor.h"
#include "string.h"

/* Prototype de la fonction d aide interne */
static void funcprepar(syste s1, HealthReport_t *report, char *s, size_t size);

/**
 * @brief  Retourne une chaine decrivant l etat systeme courant.
 * @details Utilise current_state (variable globale du superviseur).
 * @retval Pointeur sur chaine litterale (ne pas liberer).
 */
char *get_sd_status_name(void)
{
    switch (current_state) {
    case STATE_RUNNING:          return "SYS_OK";
    case STATE_SENSOR_DEGRADED:  return "SYS_degraded sensor";
    case STATE_STORAGE_DEGRADED: return "SYS_degraded storage";
    case STATE_FATAL_ERROR:      return "SYS_fatal";
    default:                     return "SYS_unknown";
    }
}

/**
 * @brief  Retourne une chaine decrivant un code d erreur AHT20.
 * @param[in] r1  Code SensorStatus a traduire.
 * @retval Pointeur sur chaine litterale (ne pas liberer).
 */
char *get_sensor_status_name(SensorStatus r1)
{
    switch (r1) {
    case SENSOR_STATUS_OK:            return "Sensor_OK";
    case SENSOR_STATUS_NOT_FOUND:     return "Sensor_not_found";
    case SENSOR_COMMUNICATION_ERROR:  return "Sensor_i2c_error";
    case SENSOR_STATUS_INVALID_DATA:  return "Sensor_invalide data";
    case SENSOR_STATUS_BUSY:          return "Sensor_busy";
    case SENSOR_NOT_CALIBRATED:       return "Sensor_notcalibrated";
    default:                          return "Sensor_unknown";
    }
}

/**
 * @brief  Tache FreeRTOS de lecture du capteur AHT20.
 * @details Initialise le capteur au demarrage, puis boucle en lisant
 *          les donnees et en adaptant le comportement a current_state.
 *          Set WD_BIT_SENSOR dans wdEventGroup a chaque iteration active.
 * @param[in] argument  Non utilise.
 */
void Task_Main(void *argument)
{
    char msg[64];
    syste s1;
    HealthReport_t report;

    /* Initialisation du capteur au demarrage de la tache */
    s1.st = AHT20_Init();
    if (s1.st != SENSOR_STATUS_OK) {
        snprintf(msg, sizeof(msg), "[ERR] AHT20_Init = %d\r\n", (uint8_t)s1.st);
        UART_TX_send_string(&huart1, msg);
        current_state = STATE_FATAL_ERROR;
    }

    report.source = HEALTH_SRC_SENSOR;

    for (;;) {
        switch (current_state) {

        case STATE_RUNNING:
            /* Lecture + affichage + publication dans les deux files */
            s1.st = AHT20_Read(&s1.temp_int, &s1.temp_frac,
                                &s1.hum_int,  &s1.hum_frac);
            funcprepar(s1, &report, msg, sizeof(msg));
            UART_TX_send_string(&huart1, msg);
            xEventGroupSetBits(wdEventGroup, WD_BIT_SENSOR);
            xQueueSend(health_queue, &report, 0);
            xQueueSend(queue_sample, &s1, 0);
            vTaskDelay(pdMS_TO_TICKS(1000));
            break;

        case STATE_SENSOR_DEGRADED:
            /* Lecture en mode degrade : ralentit a 2 s, n ecrit pas sur SD */
            s1.st = AHT20_Read(&s1.temp_int, &s1.temp_frac,
                                &s1.hum_int,  &s1.hum_frac);
            funcprepar(s1, &report, msg, sizeof(msg));
            UART_TX_send_string(&huart1, msg);
            xEventGroupSetBits(wdEventGroup, WD_BIT_SENSOR);
            xQueueSend(health_queue, &report, 0);
            xQueueSend(queue_sample, &s1, 0);
            vTaskDelay(pdMS_TO_TICKS(2000));
            break;

        case STATE_STORAGE_DEGRADED:
            /* SD en panne : le capteur se signale vivant mais n envoie pas de sample */
            xEventGroupSetBits(wdEventGroup, WD_BIT_SENSOR);
            vTaskDelay(pdMS_TO_TICKS(2000));
            break;

        case STATE_FATAL_ERROR:
            /* Etat fatal : log UART + signal watchdog, attente longue */
            UART_TX_send_string(&huart1, "state :FATAL \r\n");
            xEventGroupSetBits(wdEventGroup, WD_BIT_SENSOR);
            vTaskDelay(pdMS_TO_TICKS(2000));
            break;

        default:
            UART_TX_send_string(&huart1, "state :default \r\n");
            vTaskDelay(pdMS_TO_TICKS(3000));
            break;
        }
    }
}

/**
 * @brief  Prepare le rapport de sante et le message UART pour une mesure.
 * @details Si la mesure est valide, formate "SYS_XX T: xx.xx C  H: xx.xx %".
 *          Sinon, formate "SYS_XX erreur = <nom_erreur>".
 *          Met a jour report->event et report->last_error en consequence.
 * @param[in]  s1      Mesure AHT20 avec son statut.
 * @param[out] report  Rapport de sante a remplir (source deja definie par l appelant).
 * @param[out] s       Buffer de sortie pour le message UART.
 * @param[in]  size    Taille du buffer s.
 */
static void funcprepar(syste s1, HealthReport_t *report, char *s, size_t size)
{
    char *sys_name    = get_sd_status_name();
    char *sensor_name = get_sensor_status_name(s1.st);

    if (s1.st == SENSOR_STATUS_OK) {
        report->event      = SYS_OK;
        report->last_error = 0;
        snprintf(s, size, "%s T: %d.%02d C  H: %d.%02d %%\r\n",
                 sys_name, s1.temp_int, s1.temp_frac,
                 s1.hum_int, s1.hum_frac);
    } else {
        report->event      = SYS_SENSOR_DEGRADED;
        report->last_error = (uint8_t)s1.st;
        snprintf(s, size, "%s erreur = %s\r\n", sys_name, sensor_name);
    }
}