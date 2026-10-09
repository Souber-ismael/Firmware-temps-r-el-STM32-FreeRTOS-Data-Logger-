/*
 * storage.c
 *
 *  Created on: Oct 4, 2026
 *      Author: admin
 */

#include "storage.h"


static uint32_t current_block;

SD_Status Storage_Init(void)
{
    SD_Metadata_t meta;

    SD_Status ret =
     SD_READBLOC(SD_META_BLOCK, (uint8_t *)&meta);

    if (ret == SD_OK && meta.magic == SD_META_MAGIC)
    {
        current_block = meta.current_block;
    }
    else
    {
        current_block = SD_LOG_START_BLOCK;

        Storage_SaveMetadata();
    }

    return SD_OK;
}


SD_Status Storage_SaveMetadata(void)
{
    SD_Metadata_t meta = {0};

    meta.magic = SD_META_MAGIC;
    meta.current_block = current_block;

    return SD_WriteBlock(
        SD_META_BLOCK,
        (uint8_t *)&meta
    );
}


SD_Status Storage_WriteSampleBlock(uint8_t *data_block)
{
    SD_Status ret;

    ret = SD_WriteBlock(current_block, data_block);

    if (ret != SD_OK)
    {
        return ret;
    }

    current_block++;

    if (current_block >=
        SD_LOG_START_BLOCK + SD_LOG_BLOCK_COUNT)
    {
        current_block = SD_LOG_START_BLOCK;
    }

    ret = Storage_SaveMetadata();

    return ret;
}
