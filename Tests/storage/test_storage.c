#include "unity.h"
#include <string.h>
#include "storage.h"
#include "mock_SD_DRIVER.h"
#include "storage.c"


void setUp(void)
{
}
void tearDown(void) {}

/* ---------- Les tests ---------- */

void test_write_sample_error_does_not_advance(void)
{
    uint8_t data[512] = {0};
    current_block = SD_LOG_START_BLOCK;

    SD_WriteBlock_ExpectAndReturn(SD_LOG_START_BLOCK, data, SD_ERROR);

    SD_Status ret = Storage_WriteSampleBlock(data);

    TEST_ASSERT_EQUAL(SD_ERROR, ret);
    TEST_ASSERT_EQUAL_UINT32(SD_LOG_START_BLOCK, current_block);
}

void test_write_sample_success_advances_and_saves_meta(void)
{
    uint8_t data[512] = {0};
    current_block = SD_LOG_START_BLOCK;

    SD_WriteBlock_ExpectAndReturn(SD_LOG_START_BLOCK, data, SD_OK);  /* la donnée */
    SD_WriteBlock_IgnoreAndReturn(SD_OK);

    SD_Status ret = Storage_WriteSampleBlock(data);

    TEST_ASSERT_EQUAL(SD_OK, ret);
    TEST_ASSERT_EQUAL_UINT32(SD_LOG_START_BLOCK + 1, current_block);
}


void test_write_sample_wraps_at_end_of_log(void)
{
    uint8_t data[512] = {0};
    SD_Metadata_t meta = {0};
    current_block = SD_LOG_START_BLOCK + SD_LOG_BLOCK_COUNT - 1;

    SD_WriteBlock_ExpectAndReturn(SD_LOG_START_BLOCK + SD_LOG_BLOCK_COUNT - 1, data, SD_OK);
    SD_WriteBlock_IgnoreAndReturn(SD_OK);


    Storage_WriteSampleBlock(data);

    TEST_ASSERT_EQUAL_UINT32(SD_LOG_START_BLOCK, current_block);
}

void test_init_read_error_resets(void)
{
    SD_READBLOC_ExpectAndReturn(SD_META_BLOCK, NULL, SD_ERROR);
    SD_READBLOC_IgnoreArg_buf();
    SD_WriteBlock_ExpectAndReturn(SD_META_BLOCK, NULL, SD_OK);
    SD_WriteBlock_IgnoreArg_buffer();

    SD_Status ret = Storage_Init();

    TEST_ASSERT_EQUAL(SD_OK, ret);
    TEST_ASSERT_EQUAL_UINT32(SD_LOG_START_BLOCK, current_block);
}


void test_init_bad_magic_resets_and_saves(void)
{
    SD_Metadata_t meta = {0};
    meta.magic         = 0x12345678u;                 /* faux magic */
    meta.current_block = SD_LOG_START_BLOCK + 3;

    SD_READBLOC_ExpectAndReturn(SD_META_BLOCK, NULL, SD_OK);
    SD_READBLOC_IgnoreArg_buf();
    SD_READBLOC_ReturnMemThruPtr_buf((uint8_t *)&meta, sizeof(meta));

    SD_WriteBlock_ExpectAndReturn(SD_META_BLOCK, NULL, SD_OK);
    SD_WriteBlock_IgnoreArg_buffer();

    Storage_Init();

    TEST_ASSERT_EQUAL_UINT32(SD_LOG_START_BLOCK, current_block);
}

void test_init_valid_magic_restores_block(void)
{
    SD_Metadata_t meta = {0};
    meta.magic         = SD_META_MAGIC;
    meta.current_block = SD_LOG_START_BLOCK + 3;

    SD_READBLOC_ExpectAndReturn(SD_META_BLOCK, NULL, SD_OK);
    SD_READBLOC_IgnoreArg_buf();
    SD_READBLOC_ReturnMemThruPtr_buf((uint8_t *)&meta, sizeof(meta));

    SD_Status ret = Storage_Init();

    TEST_ASSERT_EQUAL(SD_OK, ret);
    TEST_ASSERT_EQUAL_UINT32(SD_LOG_START_BLOCK + 3, current_block);
}
