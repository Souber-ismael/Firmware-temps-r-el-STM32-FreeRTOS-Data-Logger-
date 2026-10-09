/**
 * @file    watchdog.c
 * @brief   Implementation du module watchdog (IWDG + event group FreeRTOS).
 *
 * @details Voir watchdog.h pour la description complete du mecanisme.
 */

#include "watchdog.h"

/** @brief Handle global du watchdog materiel IWDG. */
IWDG_HandleTypeDef hiwdg;

/** @brief Groupe d eventements FreeRTOS pour la supervision des taches. */
EventGroupHandle_t wdEventGroup;

/**
 * @brief  Configure et demarre le watchdog materiel IWDG.
 * @details Prescaler=128, Reload=3242 -> timeout ~10.37 s sur LSI 40 kHz.
 */
void IWDG_Start(void)
{
    hiwdg.Instance       = IWDG;
    hiwdg.Init.Prescaler = IWDG_PRESCALER_128;
    hiwdg.Init.Reload    = 3242;
    HAL_IWDG_Init(&hiwdg);
}

/**
 * @brief  Initialise l IWDG et cree le groupe d evenements de supervision.
 * @details Appele depuis main() avant vTaskStartScheduler().
 *          Une fois cet appel effectue, le watchdog est actif et doit
 *          etre nourri dans les 10 s.
 */
void IWDG_INIT(void)
{
    IWDG_Start();
    wdEventGroup = xEventGroupCreate();
}