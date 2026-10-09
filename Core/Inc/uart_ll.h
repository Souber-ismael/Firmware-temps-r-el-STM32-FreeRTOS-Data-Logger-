#ifndef UART_BM_H
#define UART_BM_H

#include "stm32f1xx.h"
#include "stm32f1xx_hal.h"
#include <stdint.h>
#include "FreeRTOS.h"
#include "cmsis_os.h"

#define UART_TX_BUF_SIZE  128

extern UART_HandleTypeDef huart1;
extern DMA_HandleTypeDef hdma1;
extern TaskHandle_t uartTxTaskHandle;

typedef enum
{
    UART_DMA_OK = 0,
    UART_ERROR,
    DMA_ERROR,
    UART_DMA_ERROR,
    UART_DMA_BUSY,
	UART_DMA_MUTEX_TIMEOUT,
    UART_DMA_INVALID_PARAMETER
} UartDmaStatus;

UartDmaStatus MX_USART1_UART_Init(void);
UartDmaStatus MX_DMA1_UART_INIT(void);
UartDmaStatus UART_TX_send_string(UART_HandleTypeDef *huart, const char *str);

#endif /* UART_BM_H */
