
#include "main.h"
#include "cmsis_os.h"
#include "FreeRTOS.h"
#include "task.h"
#include "ath20.h"
#include "hal_i2c.h"
#include "uart_ll.h"
#include <stdio.h>
#include "string.h"
#include "storage.h"

void SystemClock_Config(void);
static void MX_GPIO_Init(void);
void Task_Main(void *argument);
void  Task_Storage (void *argument);
QueueHandle_t queue_sample = NULL;

typedef struct {
	uint8_t temp_int;
	uint8_t temp_frac ;
	uint8_t hum_int;
	uint8_t hum_frac;
	SensorStatus st;
}syste;

SPI_STATUS a;

int main(void)
{
    HAL_Init();
    SystemClock_Config();
    MX_GPIO_Init();
    MX_USART1_UART_Init();
    MX_DMA1_UART_INIT();
    uart_tx_done = 1;

    MX_I2CGPIO_INIT();
    MX_I2C1_INIT();



    if(MX_SPI_INIT() == SPI_OK)



    queue_sample = xQueueCreate(10, sizeof(syste));



    if (xTaskCreate(Task_Main, "Main",384,  NULL, 3,NULL)!= pdPASS) {
    	 UART_TX_send_string(&huart1, "CREATE STORAGE FAIL 1\r\n");
    }

    if (xTaskCreate(Task_Storage, "STOR", 512, NULL, 2, NULL) != pdPASS) {
        UART_TX_send_string(&huart1, "CREATE STORAGE FAIL2\r\n");
    }

    vTaskStartScheduler();

    while (1) {}
}

/* ---------------------------------------------------------------
 * Task_Main : lit AHT20 toutes les 2 s et affiche via UART
 * --------------------------------------------------------------- */
void Task_Main(void *argument)
{
    char msg[64];
    syste s1;



    s1.st = AHT20_Init();
    if (s1.st != SENSOR_STATUS_OK)
    {
        snprintf(msg, sizeof(msg), "[ERR] AHT20_Init = %d\r\n", (uint8_t)s1.st);
        UART_TX_send_string(&huart1, msg);
        vTaskSuspend(NULL);   /* bloque sans boucle d'erreur */
    }


    for (;;)
    {
    	s1.st = AHT20_Read(&s1.temp_int, &s1.temp_frac, &s1.hum_int, &s1.hum_frac);

        if (s1.st == SENSOR_STATUS_OK)
        {
            snprintf(msg, sizeof(msg),
                     "T: %d.%02d C  H: %d.%02d %%\r\n",
                     s1.temp_int, s1.temp_frac,
					 s1.hum_int,  s1.hum_frac);
        }
        else
        {
            snprintf(msg, sizeof(msg), "[ERR] AHT20_Read = %d\r\n", (uint8_t)s1.st);
        }

        UART_TX_send_string(&huart1, msg);
        if (xQueueSend(queue_sample, &s1, 0) != pdTRUE ){
        	UART_TX_send_string(&huart1, msg);
        }

        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}



void Task_Storage(void *pvParameters)
{
    static uint8_t write_buffer[512];
    static uint16_t buffer_offset = 0;

    SD_Init();
    Storage_Init();

    syste sample;

    for (;;)
    {
        if (xQueueReceive(queue_sample, &sample, portMAX_DELAY) == pdTRUE)
        {

        	if(sample.st != SD_OK) continue;


            memcpy(&write_buffer[buffer_offset],
                   &sample,
                   sizeof(syste));

            buffer_offset += sizeof(syste);

            if (buffer_offset + sizeof(syste) > 512)
            {
                if(Storage_WriteSampleBlock(write_buffer)== SD_OK)
                {HAL_GPIO_TogglePin(GPIOA, GPIO_PIN_5);}
                buffer_offset = 0;
                memset(write_buffer, 0xFF, 512);
            }
        }


    }
}

/* ---------------------------------------------------------------
 * Mesure du stack utilise depuis la tache elle-meme
 * Ajoute dans la boucle for(;;) de Task_Main si tu veux surveiller
 *
 *   UBaseType_t hwm = uxTaskGetStackHighWaterMark(NULL);
 *   snprintf(msg, sizeof(msg), "[STACK] HWM: %lu words libres\r\n",
 *            (unsigned long)hwm);
 *   UART_TX_send_string(&huart1, msg);
 *
 * HWM = nombre de words de stack jamais utilises (minimum historique)
 * Stack total alloue = 384 words
 * Stack consomme = 384 - HWM
 * --------------------------------------------------------------- */

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


static void MX_GPIO_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    __HAL_RCC_GPIOC_CLK_ENABLE();
    __HAL_RCC_GPIOD_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();

    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, GPIO_PIN_RESET);

    GPIO_InitStruct.Pin  = B1_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(B1_GPIO_Port, &GPIO_InitStruct);

    GPIO_InitStruct.Pin   = GPIO_PIN_5;
    GPIO_InitStruct.Mode  = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull  = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    HAL_NVIC_SetPriority(EXTI15_10_IRQn, 5, 0);
    HAL_NVIC_EnableIRQ(EXTI15_10_IRQn);
}

void Error_Handler(void)
{
    __disable_irq();
    while (1) {}
}

#ifdef USE_FULL_ASSERT
void assert_failed(uint8_t *file, uint32_t line)
{
}
#endif
