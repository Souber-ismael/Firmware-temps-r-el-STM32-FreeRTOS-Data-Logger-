/**
 * @file    main.c
 * @brief   Point d entree de la station meteo embarquee sur STM32F103.
 *
 * @details Ce fichier initialise tous les peripheriques materiel, cree les
 *          files de messages FreeRTOS et lance les trois taches applicatives
 *          avant de ceder le controle au scheduler.
 *
 *          Architecture des taches :
 *          - Task_Main       (prio 3) : lecture AHT20, affichage UART.
 *          - Task_Storage    (prio 1) : accumulation et ecriture SD.
 *          - Task_Supervisor (prio 2) : supervision, machine d etat, IWDG.
 *
 *          Files de messages :
 *          - queue_sample  : mesures brutes AHT20 (syste) -> Task_Storage.
 *          - health_queue  : rapports de sante (HealthReport_t) -> Task_Supervisor.
 *
 *          Horloge : HSI/2 x16 = 64 MHz SYSCLK, APB1=32 MHz, APB2=64 MHz.
 */

#include "main.h"
#include <stdio.h>
#include "hal_spi.h"
#include "uart_ll.h"
#include "hal_i2c.h"
#include "app_task_sensor.h"
#include "app_task_storage.h"
#include "app_task_supervisor.h"
#include "watchdog.h"

/* Prototypes des fonctions locales */
void SystemClock_Config(void);
static void MX_GPIO_Init(void);

/** @brief File de mesures AHT20 entre Task_Main et Task_Storage (capacite 10). */
QueueHandle_t queue_sample = NULL;

/** @brief File de rapports de sante entre les taches et Task_Supervisor (capacite 10). */
QueueHandle_t health_queue = NULL;

/**
 * @brief  Point d entree du programme.
 * @details Initialise HAL, configure l horloge, les GPIO, l UART+DMA, l I2C,
 *          le SPI, le watchdog, les files FreeRTOS et les trois taches,
 *          puis demarre le scheduler.  Ne retourne jamais.
 * @retval int  Non utilise.
 */
int main(void)
{
    HAL_Init();
    SystemClock_Config();
    MX_GPIO_Init();

    /* UART1 + DMA1 Channel4 pour le debug serie */
    MX_USART1_UART_Init();
    MX_DMA1_UART_INIT();

    /* I2C1 pour le capteur AHT20 */
    MX_I2CGPIO_INIT();
    MX_I2C1_INIT();

    /* SPI2 pour la microSD ; IWDG demarre uniquement si SPI init reussie */
    if (MX_SPI_INIT() == SPI_OK) {
        IWDG_INIT();  /* Lance le watchdog materiel (~10 s) et cree wdEventGroup */
    }

    /* Creation des files de communication inter-taches */
    queue_sample = xQueueCreate(10, sizeof(syste));
    health_queue = xQueueCreate(10, sizeof(HealthReport_t));

    /* Creation des taches FreeRTOS */
    if (xTaskCreate(Task_Main, "Main", 384, NULL, 3, NULL) != pdPASS) {
        UART_TX_send_string(&huart1, "CREATE STORAGE FAIL 1\r\n");
    }

    if (xTaskCreate(Task_Storage, "STOR", 512, NULL, 1, NULL) != pdPASS) {
        UART_TX_send_string(&huart1, "CREATE STORAGE FAIL2\r\n");
    }

    if (xTaskCreate(Task_Supervisor, "supervisor", 314, NULL, 2, NULL) != pdPASS) {
        UART_TX_send_string(&huart1, "CREATE STORAGE FAIL3\r\n");
    }

    /* Demarre le scheduler FreeRTOS — ne retourne jamais si le heap est suffisant */
    vTaskStartScheduler();

    while (1) {}
}

/**
 * @brief  Configure le bus d horloge systeme sur HSI x16 = 64 MHz.
 * @details Oscillateur : HSI (8 MHz interne).
 *          PLL : source HSI/2 = 4 MHz, multipliee par 16 = 64 MHz SYSCLK.
 *          AHB = 64 MHz, APB1 = 32 MHz (/2), APB2 = 64 MHz (/1).
 *          Latence Flash : 2 cycles (obligatoire pour HCLK > 48 MHz).
 */
void SystemClock_Config(void)
{
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

    RCC_OscInitStruct.OscillatorType      = RCC_OSCILLATORTYPE_HSI;
    RCC_OscInitStruct.HSIState            = RCC_HSI_ON;
    RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
    RCC_OscInitStruct.PLL.PLLState        = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource       = RCC_PLLSOURCE_HSI_DIV2;
    RCC_OscInitStruct.PLL.PLLMUL          = RCC_PLL_MUL16;
    if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) { Error_Handler(); }

    RCC_ClkInitStruct.ClockType      = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK
                                     | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInitStruct.SYSCLKSource   = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider  = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;
    if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK) { Error_Handler(); }
}

/**
 * @brief  Initialise les GPIO de l application.
 * @details Broches configurees :
 *          - PC13 (B1_Pin) : entree EXTI front montant (bouton utilisateur).
 *          - PA5           : sortie push-pull (LED etat systeme / flash SD).
 *          - PA6           : sortie push-pull (LED heartbeat superviseur).
 *          Les GPIO des peripheriques (UART, I2C, SPI) sont configures
 *          dans leurs modules respectifs via HAL_xxx_MspInit().
 */
static void MX_GPIO_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    __HAL_RCC_GPIOC_CLK_ENABLE();
    __HAL_RCC_GPIOD_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();

    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, GPIO_PIN_RESET);

    /* PC13 : bouton utilisateur, interruption sur front montant */
    GPIO_InitStruct.Pin  = B1_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(B1_GPIO_Port, &GPIO_InitStruct);

    /* PA5 : LED etat systeme (toggle lors des ecritures SD reussies) */
    GPIO_InitStruct.Pin   = GPIO_PIN_5;
    GPIO_InitStruct.Mode  = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull  = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    /* PA6 : LED heartbeat superviseur (toggle a chaque iteration superviseur) */
    GPIO_InitStruct.Pin   = GPIO_PIN_6;
    GPIO_InitStruct.Mode  = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull  = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    /* EXTI15_10 : priorite 5 (dans la plage FreeRTOS SYSCALL) */
    HAL_NVIC_SetPriority(EXTI15_10_IRQn, 5, 0);
    HAL_NVIC_EnableIRQ(EXTI15_10_IRQn);
}

/**
 * @brief  Gestionnaire d erreur fatal.
 * @details Desactive les interruptions et boucle indefiniment.
 *          Atteint uniquement si HAL_RCC_OscConfig ou HAL_RCC_ClockConfig echoue.
 */
void Error_Handler(void)
{
    __disable_irq();
    while (1) {}
}

#ifdef USE_FULL_ASSERT
/**
 * @brief  Callback d assertion HAL (active uniquement si USE_FULL_ASSERT est defini).
 * @param[in] file  Nom du fichier source ou l assertion a echoue.
 * @param[in] line  Numero de ligne de l assertion.
 */
void assert_failed(uint8_t *file, uint32_t line)
{
    (void)file;
    (void)line;
}
#endif