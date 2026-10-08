#include "uart_ll.h"
#include <stdio.h>
#include "string.h"
#include "FreeRTOS.h"
#include "cmsis_os.h"

UART_HandleTypeDef huart1;
DMA_HandleTypeDef hdma1;
volatile  uint8_t uart_tx_done =1;
TaskHandle_t uartTxTaskHandle = NULL;

UartDmaStatus MX_USART1_UART_Init(void) {

	huart1.Instance = USART1;
	huart1.Init.BaudRate =9600;
	huart1.Init.HwFlowCtl =UART_HWCONTROL_NONE;
	huart1.Init.Mode= UART_MODE_TX_RX;
	huart1.Init.OverSampling = UART_OVERSAMPLING_16;
	huart1.Init.Parity = UART_PARITY_NONE;
	huart1.Init.StopBits = UART_STOPBITS_1;
	huart1.Init.WordLength = UART_WORDLENGTH_8B;

	 if (HAL_UART_Init(&huart1) != HAL_OK)
	  {
		 return  UART_ERROR;;
	  }


	 HAL_NVIC_SetPriority(USART1_IRQn, 6, 0);
	 HAL_NVIC_EnableIRQ(USART1_IRQn);

	 return UART_DMA_OK;
}

UartDmaStatus MX_DMA1_UART_INIT(void){

	__HAL_RCC_DMA1_CLK_ENABLE();

	hdma1.Instance                 = DMA1_Channel4; /* USART1 TX — fixed by hardware */
	hdma1.Init.Direction           = DMA_MEMORY_TO_PERIPH;
	hdma1.Init.PeriphInc           = DMA_PINC_DISABLE;  /* DR is a single fixed register */
	hdma1.Init.MemInc              = DMA_MINC_ENABLE;   /* advance through source buffer */
	hdma1.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
	hdma1.Init.MemDataAlignment    = DMA_MDATAALIGN_BYTE;
	hdma1.Init.Mode                = DMA_NORMAL;        /* stops after the transfer      */
	hdma1.Init.Priority            = DMA_PRIORITY_LOW;

	if(HAL_DMA_Init(&hdma1)!= HAL_OK)
	return DMA_ERROR;


	__HAL_LINKDMA(&huart1, hdmatx, hdma1);


	HAL_NVIC_SetPriority(DMA1_Channel4_IRQn, 6, 0);
	HAL_NVIC_EnableIRQ(DMA1_Channel4_IRQn);

	return UART_DMA_OK;
}





void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart){
    if(huart == &huart1){ // vérifie ton uart
        BaseType_t xHigherPriorityTaskWoken = pdFALSE;
        vTaskNotifyGiveFromISR(uartTxTaskHandle, &xHigherPriorityTaskWoken);
        portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
    }
}

UartDmaStatus UART_TX_send_string(UART_HandleTypeDef *huart, const char *str)
{
    static uint8_t tx_buf[UART_TX_BUF_SIZE];
    uint16_t len = (uint16_t)strlen(str);

    if (len == 0 || len >= sizeof(tx_buf))
        return UART_DMA_INVALID_PARAMETER;

    if(huart->gState != HAL_UART_STATE_READY) // check avant
        return UART_DMA_BUSY;

    memcpy(tx_buf, str, len);
    uartTxTaskHandle = xTaskGetCurrentTaskHandle(); // set AVANT de lancer

    if(HAL_UART_Transmit_DMA(huart, tx_buf, len) != HAL_OK){
        return UART_DMA_ERROR;
    }

    // on attend QUE si DMA bien lancé
    if (ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(2000)) == 0)
    {
        return UART_DMA_BUSY; // timeout, callback jamais venu
    }

    return UART_DMA_OK;
}
