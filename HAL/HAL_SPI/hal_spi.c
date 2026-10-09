/*
 * hal_spi.c
 *
 *  Created on: Sep 29, 2026
 *      Author: admin
 */
#include "hal_spi.h"

SPI_HandleTypeDef hspi2;

SPI_STATUS MX_SPI_INIT(void) {

	GPIO_InitTypeDef GPIO = { 0 };
	// hal_spi.c — séparer MISO des autres pins
	GPIO.Mode = GPIO_MODE_AF_PP;
	GPIO.Pin = GPIO_PIN_13 | GPIO_PIN_15;  // SCK + MOSI seulement
	GPIO.Speed = GPIO_SPEED_HIGH;
	HAL_GPIO_Init(GPIOB, &GPIO);

	GPIO.Mode = GPIO_MODE_INPUT;
	GPIO.Pull = GPIO_PULLUP;
	GPIO.Speed = GPIO_SPEED_HIGH;
	GPIO.Pin = GPIO_PIN_14;                // MISO = entrée
	HAL_GPIO_Init(GPIOB, &GPIO);

	GPIO.Pin = GPIO_PIN_4;
	GPIO.Mode = GPIO_MODE_OUTPUT_PP;
	HAL_GPIO_Init(GPIOA, &GPIO);
	SD_CS_HIGH();

	hspi2.Instance = SPI2;
	hspi2.Init.Mode = SPI_MODE_MASTER;
	hspi2.Init.Direction = SPI_DIRECTION_2LINES;
	hspi2.Init.DataSize = SPI_DATASIZE_8BIT;
	hspi2.Init.CLKPolarity = SPI_POLARITY_LOW; // CPOL=0
	hspi2.Init.CLKPhase = SPI_PHASE_1EDGE; // CPHA=0 Mode 0 obligatoire pour SD
	hspi2.Init.NSS = SPI_NSS_SOFT;
	hspi2.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_128; // ~ 400kHz si APB2=50MHz
	hspi2.Init.FirstBit = SPI_FIRSTBIT_MSB;

	if (HAL_SPI_Init(&hspi2) != HAL_OK)
		return SPI_NOINIT;

	return SPI_OK;

}

SPI_STATUS SD_SetHighSpeed(void) {
	hspi2.Instance = SPI2;
	hspi2.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_8; // ~18 MHz, démarre prudent

	if (HAL_SPI_Init(&hspi2) != HAL_OK) {
		return SPI_NOINIT;  // erreur de réinitialisation du périphérique
	}

	return SPI_OK;
}

void SD_CS_LOW(void) {
	HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_RESET);
}

void SD_CS_HIGH(void) {
	HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_SET);
}

SPI_STATUS SPI_TxRx(uint8_t *tx, uint8_t *rx) {
	HAL_StatusTypeDef hal_status;

	hal_status = HAL_SPI_TransmitReceive(&hspi2, tx, rx, 1, 100);

	switch (hal_status) {
	case HAL_OK:
		return SPI_OK;

	case HAL_BUSY:
		return SPI_BUSY;

	case HAL_TIMEOUT:
		return SPI_TIMEOUT;

	case HAL_ERROR:
	default:
		return SPI_ERROR;
	}
}

SPI_STATUS SPI_Tx(uint8_t *tx, uint16_t size) {
	HAL_StatusTypeDef hal_status;
	uint16_t tim = (size == 512) ? 1000 : 100;
	hal_status = HAL_SPI_Transmit(&hspi2, tx, size, tim);

	switch (hal_status) {
	case HAL_OK:
		return SPI_OK;

	case HAL_BUSY:
		return SPI_BUSY;

	case HAL_TIMEOUT:
		return SPI_TIMEOUT;

	case HAL_ERROR:
	default:
		return SPI_ERROR;
	}
}
