/**
 * @file    app_task_storage.c
 * @brief   Implementation de la tache de stockage SD et du helper d ecriture.
 *
 * @details Recoit les mesures depuis queue_sample, les accumule dans un buffer
 *          statique de 512 octets, puis ecrit un bloc complet sur la microSD
 *          via Storage_WriteSampleBlock().  Rapporte le resultat dans health_queue.
 *
 *          En etat degrade SD (STATE_STORAGE_DEGRADED), tente periodiquement
 *          de vider le buffer et rapporte le resultat au superviseur.
 */

#include "app_task_storage.h"

/** @brief Buffer d accumulation des mesures avant ecriture SD (1 bloc = 512 octets). */
static uint8_t write_buffer[512];

/** @brief Position courante d ecriture dans write_buffer (en octets). */
static uint16_t buffer_offset = 0;

/* Prototype de la fonction interne d ecriture */
static void Storage_FlushBlock(HealthReport_t *report);

/**
 * @brief  Retourne une chaine decrivant un code d erreur SD.
 * @param[in] r1  Code SD_Status a traduire (passe en uint8_t).
 * @retval Pointeur sur chaine litterale (ne pas liberer).
 */
static char *get_storage_status_name(uint8_t r1)
{
    switch (r1) {
    case SD_OK:               return "Storage_ok";
    case SD_ERROR:            return "Storage_spi_error";
    case SD_TIMEOUT:          return "Storage_spi_timeout";
    case SD_BUSY:             return "Storage_spi_busy";
    case SD_NOT_READY:        return "Storage_not_ready";
    case SD_ERROR_TIMEOUT:    return "Storage_sd_timeout";
    case SD_ERR_DATA_TOKEN:   return "Storage_err_data";
    case SD_ERROR_CDM:        return "Storage_err_cmd";
    case SD_ERR_RESPONSE:     return "Storage_err_response";
    case SD_ERR_WRITE_TIMEOUT: return "Storage_err_write";
    case SD_NOT_FOUND:        return "Storage_not_found";
    default:                  return "Storage_unknown";
    }
}

/**
 * @brief  Tache FreeRTOS d ecriture des mesures sur la microSD.
 * @details Initialise SD et la couche de stockage au demarrage, puis boucle
 *          en consommant queue_sample et en ecrivant sur SD bloc par bloc.
 *          Set WD_BIT_STORAGE a chaque iteration pour signaler sa vivacite.
 * @param[in] pvParameters  Non utilise.
 */
void Task_Storage(void *pvParameters)
{
    SD_Init();       /* Initialisation SPI + carte SD */
    Storage_Init();  /* Lecture des metadonnees, reprise du pointeur de bloc */

    syste sample;
    HealthReport_t report;
    char *c;

    report.source = HEALTH_SRC_STORAGE;

    for (;;) {
        switch (current_state) {

        case STATE_RUNNING:
            /* Recoit une mesure (attente 100 ticks) et l ajoute au buffer */
            if (xQueueReceive(queue_sample, &sample, 100) == pdTRUE) {
                if (sample.st == SENSOR_STATUS_OK) {
                    memcpy(&write_buffer[buffer_offset], &sample, sizeof(syste));
                    buffer_offset += sizeof(syste);

                    /* Buffer plein : ecrit le bloc sur SD et rapporte le resultat */
                    if (buffer_offset + sizeof(syste) > 512) {
                        Storage_FlushBlock(&report);
                        xQueueSend(health_queue, &report, 0);
                    }
                }
            }
            /* Set le bit watchdog meme si aucun sample n etait disponible */
            xEventGroupSetBits(wdEventGroup, WD_BIT_STORAGE);
            break;

        case STATE_SENSOR_DEGRADED:
            /* Capteur degrade : pas de nouvelles donnees, attend + heartbeat */
            xEventGroupSetBits(wdEventGroup, WD_BIT_STORAGE);
            HAL_GPIO_TogglePin(GPIOA, GPIO_PIN_6);
            vTaskDelay(1000);
            break;

        case STATE_STORAGE_DEGRADED:
            /* SD degrade : tente l ecriture et logue l erreur sur UART */
            Storage_FlushBlock(&report);
            xQueueSend(health_queue, &report, 0);
            if (report.event != SYS_OK) {
                c = get_storage_status_name(report.last_error);
                UART_TX_send_string(&huart1, c);
            }
            xEventGroupSetBits(wdEventGroup, WD_BIT_STORAGE);
            vTaskDelay(5000);
            break;

        case STATE_FATAL_ERROR:
            /* Etat fatal : heartbeat LED + signal watchdog, longue attente */
            xEventGroupSetBits(wdEventGroup, WD_BIT_STORAGE);
            HAL_GPIO_TogglePin(GPIOA, GPIO_PIN_6);
            vTaskDelay(2000);
            break;

        default:
            xEventGroupSetBits(wdEventGroup, WD_BIT_STORAGE);
            HAL_GPIO_TogglePin(GPIOA, GPIO_PIN_6);
            vTaskDelay(2000);
            break;
        }
    }
}

/**
 * @brief  Ecrit write_buffer sur la microSD et met a jour le rapport de sante.
 * @details Appelle Storage_WriteSampleBlock(write_buffer).
 *          Si succes : remet buffer_offset a 0, reinitialise le buffer a 0xFF,
 *          et toggle PA5 (LED confirmation).
 *          Si echec : rapporte SYS_STORAGE_DEGRADED avec le code d erreur SD.
 * @param[out] report  Rapport de sante a remplir selon le resultat.
 */
static void Storage_FlushBlock(HealthReport_t *report)
{
    SD_Status ret;

    ret = Storage_WriteSampleBlock(write_buffer);

    if (ret == SD_OK) {
        report->event      = SYS_OK;
        report->last_error = (uint8_t)ret;
        buffer_offset      = 0;
        memset(write_buffer, 0xFF, 512);
        HAL_GPIO_TogglePin(GPIOA, GPIO_PIN_5);  /* LED : ecriture SD reussie */
    } else {
        report->event      = SYS_STORAGE_DEGRADED;
        report->last_error = (uint8_t)ret;
    }
}