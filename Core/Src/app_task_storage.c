/*
 * app_task_storage.c
 *
 *  Created on: Oct 6, 2026
 *      Author: admin
 */

#include "app_task_storage.h"


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

