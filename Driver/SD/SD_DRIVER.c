/*
 * SD_DRIVER.c
 *
 *  Created on: Sep 29, 2026
 *      Author: admin
 */

#include "SD_DRIVER.h"
#include "uart_ll.h"
#include "string.h"
#include "hal_spi.h"

static uint8_t sd_is_sdhc = 0;

SD_Status SD_ConvertSPIStatus(SPI_STATUS status) {
	switch (status) {
	case SPI_OK:
		return SD_OK;

	case SPI_BUSY:
		return SD_BUSY;

	case SPI_TIMEOUT:
		return SD_TIMEOUT;

	case SPI_ERROR:
	default:
		return SD_ERROR;
	}
}

SD_Status SD_SPI_TxRx(uint8_t *tx, uint8_t *rx) {
	SPI_STATUS status;

	status = SPI_TxRx(tx, rx);

	if (status != SPI_OK) {
		return SD_ConvertSPIStatus(status);
	}

	return SD_OK;
}

SD_Status SD_SPI_Tx(uint8_t *tx, uint16_t size) {
	SPI_STATUS status;

	status = SPI_Tx(tx, size);

	if (status != SPI_OK) {
		return SD_ConvertSPIStatus(status);
	}

	return SD_OK;
}

SD_Status SD_PowerUpSequence(void) {
	uint8_t dummy[10] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
			0xFF };
	SD_Status status;
	// 10 octets = 80 clocks, >= 74 requis
	SD_CS_HIGH();   // CS HAUT
	status = SD_SPI_Tx(dummy, 10);
	if (status != SD_OK)
		return status;

	return SD_OK;
}

SD_Status SD_SendCmd(uint8_t cmd, uint32_t arg, uint8_t crc) {
	uint8_t buf[6];
	uint8_t r1, dummy = 0xFF;
	SD_Status ret = SD_OK;

	buf[0] = 0x40 | cmd;
	buf[1] = (arg >> 24) & 0xFF;
	buf[2] = (arg >> 16) & 0xFF;
	buf[3] = (arg >> 8) & 0xFF;
	buf[4] = arg & 0xFF;
	buf[5] = crc;

	SD_CS_LOW();
	SD_SPI_Tx(&dummy, 1);   // 0xFF, protocole SD SPI obligatoire après CS LOW
	ret = SD_SPI_Tx(buf, 6);
	if (ret != SD_OK)
		goto Fin;

	for (int i = 0; i < 20; i++) {
		ret = SD_SPI_TxRx(&dummy, &r1);
		if (ret != SD_OK)
			goto Fin;
		if (r1 != 0xFF)
			break;
	}

	if (cmd == 0 || cmd == 8) {
		if (r1 != 0x01) {
			ret = SD_ERROR_CDM;
			goto Fin;
		}
	}
	if (cmd == 8) {
		uint8_t r7[4];
		for (int i = 0; i < 4; i++) {
			ret = SD_SPI_TxRx(&dummy, &r7[i]);
			if (ret != SD_OK)
				goto Fin;
		}
		if (r7[2] != 0x01 || r7[3] != 0xAA) {
			ret = SD_ERROR_CDM;
			goto Fin;
		}
	}
	if (cmd == 41 || cmd == 58) {
		if (r1 != 0x00) {
			ret = SD_NOT_READY;
			goto Fin;
		}
	}
	if (cmd == 17 || cmd == 24) {
		if (r1 != 0x00) {
			ret = SD_NOT_FOUND;
			goto Fin;
		}
	}

	Fin:
	// CMD17 : CS reste bas — le bloc de données doit être lu dans la même transaction
	if (cmd != 17) {
		SD_CS_HIGH();
		SD_SPI_Tx(&dummy, 1);  // 8 clocks de libération
	}
	return ret;
}

SD_Status SD_ReadOCR(uint8_t *is_sdhc) {
	SD_Status ret;
	uint8_t ocr_bytes[4];
	uint8_t dummy = 0xFF;
	uint32_t ocr;

	// SD_SendCmd(58) descend CS et le laisse bas pour cmd==58 ?
	// Non — cmd 58 != 17, donc Fin: remonte CS.
	// Il faut descendre CS manuellement et envoyer CMD58 sans le remonter.

	uint8_t buf[6] = { 0x40 | 58, 0, 0, 0, 0, 0x01 };
	uint8_t r1;

	SD_CS_LOW();
	ret = SD_SPI_Tx(buf, 6);
	if (ret != SD_OK)
		goto Fin;

	for (int i = 0; i < 10; i++) {
		ret = SD_SPI_TxRx(&dummy, &r1);
		if (ret != SD_OK)
			goto Fin;
		if (r1 != 0xFF)
			break;
	}
	if (r1 != 0x00) {
		ret = SD_NOT_READY;
		goto Fin;
	}

	for (int i = 0; i < 4; i++) {
		ret = SD_SPI_TxRx(&dummy, &ocr_bytes[i]);
		if (ret != SD_OK)
			goto Fin;
	}

	ocr = ((uint32_t) ocr_bytes[0] << 24) | ((uint32_t) ocr_bytes[1] << 16)
			| ((uint32_t) ocr_bytes[2] << 8) | ocr_bytes[3];

	// Bit 30 = CCS : 1 → SDHC/SDXC (adressage par bloc), 0 → SDSC (par octet)
	if (is_sdhc)
		*is_sdhc = (ocr >> 30) & 0x01;

	Fin: SD_CS_HIGH();
	SD_SPI_Tx(&dummy, 1);
	return ret;
}

SD_Status SD_Init(void) {
	SD_Status status;
	uint8_t dummy = 0xFF;

	status = SD_PowerUpSequence();
	if (status != SD_OK)
		return status;

	// SD_SendCmd gère CS — plus de SD_CS_LOW() ici
	status = SD_SendCmd(0, 0, 0x95);
	if (status != SD_OK)
		return status;

	status = SD_SendCmd(8, 0x000001AA, 0x87);
	if (status != SD_OK)
		return status;

	for (int i = 0; i < 20; i++) {
		status = SD_SendCmd(55, 0, 0x00);
		if (status != SD_OK)
			return status;

		status = SD_SendCmd(41, 0x40000000, 0x00);
		if (status == SD_OK)
			break;
		if (i == 19)
			return SD_NOT_READY;
		HAL_Delay(50);
	}

	status = SD_ReadOCR(&sd_is_sdhc);
	if (status != SD_OK)
		return status;

	SD_CS_HIGH();
	SD_SPI_Tx(&dummy, 1);
	return SD_OK;
}

SD_Status SD_READBLOC(uint32_t addr, uint8_t *buf) {
	SD_Status r1;
	uint8_t dummy = 0xFF;
	uint32_t send_addr = sd_is_sdhc ? addr : (addr * 512);

	// SD_SendCmd gère CS — pas de SD_CS_LOW() ici
	// Pour CMD17, CS reste bas après SD_SendCmd (voir Fin: ci-dessus)
	r1 = SD_SendCmd(17, send_addr, 0xFF);
	if (r1 != SD_OK) {
		SD_CS_HIGH();   // forcer CS haut en cas d'erreur CMD17
		return r1;
	}

	uint8_t token = 0xFF;
	uint32_t start = HAL_GetTick();

	while (token == 0xFF) {
		r1 = SD_SPI_TxRx(&dummy, &token);
		if (r1 != SD_OK) {
			SD_CS_HIGH();
			return r1;
		}
		if (HAL_GetTick() - start > 2000) {
			SD_CS_HIGH();
			return SD_ERROR_TIMEOUT;
		}
	}

	if (token != 0xFE) {
		SD_CS_HIGH();
		return SD_ERR_DATA_TOKEN;   // log token ici si tu as un UART
	}

	// Lire 512 octets un par un (SPI_TxRx est câblé sur 1 octet)
	for (int i = 0; i < 512; i++) {
		r1 = SD_SPI_TxRx(&dummy, &buf[i]);
		if (r1 != SD_OK) {
			SD_CS_HIGH();
			return r1;
		}
	}

	uint8_t crc1[2];
	SD_SPI_TxRx(&dummy, &crc1[0]);
	SD_SPI_TxRx(&dummy, &crc1[1]);

	SD_CS_HIGH();
	SD_SPI_Tx(&dummy, 1);
	return SD_OK;
}

// wbuf[512] en GLOBAL, pas sur la stack
SD_Status SD_WriteBlock(uint32_t addr, uint8_t *buffer) {
	uint8_t dummy = 0xFF, r1 = 0xFF;
	uint32_t arg = sd_is_sdhc ? addr : (addr * 512);
	SD_Status ret;

	// --- CS HIGH reset ---
	SD_CS_HIGH();
	ret = SD_SPI_Tx(&dummy, 1);
	if (ret != SD_OK) {
		SD_CS_HIGH();
		return ret;
	}
	ret = SD_SPI_Tx(&dummy, 1);
	if (ret != SD_OK) {
		SD_CS_HIGH();   // forcer CS haut en cas d'erreur CMD24
		return ret;
	}

	// --- CS LOW ---
	SD_CS_LOW();
	HAL_Delay(1);

	// --- Attend READY (FF) ---
	for (int i = 0; i < 100; i++) {
		ret = SD_SPI_TxRx(&dummy, &r1);
		if (ret != SD_OK) {
			SD_CS_HIGH();
			return ret;
		}
		if (r1 == 0xFF)
			break;
	}

	// --- Envoie CMD24 ---
	uint8_t cmd[6] = { 0x40 | 24, 0, 0, (arg >> 8) & 0xFF, arg & 0xFF, 0x01 }; // bloc 10 pour SDHC
	if (arg > 512) {
		cmd[1] = arg >> 24;
		cmd[2] = arg >> 16;
		cmd[3] = arg >> 8;
		cmd[4] = arg;
	}
	ret = SD_SPI_Tx(cmd, 6);
	if (ret != SD_OK) {
		SD_CS_HIGH();
		return ret;
	}

	// --- Attend R1, 17x FF c'est normal chez toi ---
	r1 = 0xFF;
	for (int i = 0; i < 100; i++) {
		SD_SPI_TxRx(&dummy, &r1);
		if (r1 == 0x00)
			break;
	}

	if (r1 != 0x00) {
		ret = SD_NOT_READY;
		goto end;
	}

	// --- Token + DATA ---
	uint8_t fe = 0xFE;
	ret = SD_SPI_Tx(&fe, 1);
	if (ret != SD_OK) {
		SD_CS_HIGH();
		return ret;
	}
	ret = SD_SPI_Tx(buffer, 512);
	if (ret != SD_OK) {
		SD_CS_HIGH();
		return ret;
	}
	uint8_t crc[2] = { 0xFF, 0xFF };
	ret = SD_SPI_Tx(crc, 2);
	if (ret != SD_OK) {
		SD_CS_HIGH();
		return ret;
	}

	// --- Data response ---
	uint8_t dr = 0xFF;
	for (int i = 0; i < 20; i++) {
		ret = SD_SPI_TxRx(&dummy, &dr);
		if (ret != SD_OK) {
			SD_CS_HIGH();
			return ret;
		}
		if (dr != 0xFF)
			break;
	}

	// --- Attend fin d'écriture ---
	uint32_t t = HAL_GetTick();
	do {
		ret = SD_SPI_TxRx(&dummy, &r1);
		if (ret != SD_OK) {
			SD_CS_HIGH();
			return ret;
		}
	} while (r1 != 0xFF && HAL_GetTick() - t < 2000);

	end: SD_CS_HIGH();
	SD_SPI_Tx(&dummy, 1);
	SD_SPI_Tx(&dummy, 1);
	return ret;

}
