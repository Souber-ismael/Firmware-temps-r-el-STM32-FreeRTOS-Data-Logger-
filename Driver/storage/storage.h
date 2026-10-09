/*
 * storage.h
 *
 *  Created on: Oct 4, 2026
 *      Author: admin
 */

#ifndef INC_STORAGE_H_
#define INC_STORAGE_H_

#include <stdint.h>
#include "SD_DRIVER.h"

#define SD_META_BLOCK        999
#define SD_LOG_START_BLOCK   1000
#define SD_LOG_BLOCK_COUNT   50000

typedef struct {
    uint32_t magic;          // signature pour valider que le bloc contient bien nos métadonnées
    uint32_t current_block;  // prochain bloc à écrire
    uint32_t write_count;    // nombre total d'écritures (utile pour wear-leveling manuel, optionnel)
    uint8_t  padding[500];
} SD_Metadata_t;

#define SD_META_MAGIC  0x53415345   // "SASE" en hexa, arbitraire mais reconnaissable



SD_Status Storage_Init(void);
SD_Status Storage_SaveMetadata(void);
SD_Status Storage_WriteSampleBlock(uint8_t *data_block);
#endif /* INC_STORAGE_H_ */
