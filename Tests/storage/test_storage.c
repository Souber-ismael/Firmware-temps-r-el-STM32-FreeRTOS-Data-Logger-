#include "unity.h"
#include <string.h>
#include "storage.h"

/* ---------- Les doublures ---------- */

/* Ce que la fausse SD va "répondre" */
static SD_Status     fake_read_ret;
static SD_Metadata_t fake_read_meta;
static SD_Status     fake_write_ret;

/* Ce que la fausse SD va "se rappeler" */
static int           write_count;
static uint32_t      write_blocks[10];       /* numéros de blocs écrits, dans l'ordre */
static SD_Metadata_t last_meta_written;

SD_Status SD_READBLOC(uint32_t block, uint8_t *buf)
{
    (void)block;
    memcpy(buf, &fake_read_meta, sizeof(SD_Metadata_t));
    return fake_read_ret;
}

SD_Status SD_WriteBlock(uint32_t block, uint8_t *buf)
{
    write_blocks[write_count] = block;
    write_count++;
    if (block == SD_META_BLOCK) {
        memcpy(&last_meta_written, buf, sizeof(SD_Metadata_t));
    }
    return fake_write_ret;
}

#include "storage.c"
/* ---------- Préparation avant chaque test ---------- */
void setUp(void)
{
    fake_read_ret  = SD_OK;
    fake_write_ret = SD_OK;
    memset(&fake_read_meta, 0, sizeof(fake_read_meta));
    memset(&last_meta_written, 0, sizeof(last_meta_written));
    memset(write_blocks, 0, sizeof(write_blocks));
    write_count   = 0;
    current_block = 0;
}
void tearDown(void) {}

/* ---------- Les tests ---------- */

void test_init_valid_magic_restores_block(void)
{
    fake_read_meta.magic         = SD_META_MAGIC;
    fake_read_meta.current_block = SD_LOG_START_BLOCK + 3;

    SD_Status ret = Storage_Init();

    TEST_ASSERT_EQUAL(SD_OK, ret);
    TEST_ASSERT_EQUAL_UINT32(SD_LOG_START_BLOCK + 3, current_block);
    TEST_ASSERT_EQUAL_INT(0, write_count);   /* rien écrit : tout est déjà valide */
}

void test_init_bad_magic_resets_and_saves(void)
{
    fake_read_meta.magic         = 0x12345678u;   /* faux magic */
    fake_read_meta.current_block = SD_LOG_START_BLOCK + 3;

    Storage_Init();

    TEST_ASSERT_EQUAL_UINT32(SD_LOG_START_BLOCK, current_block);
    TEST_ASSERT_EQUAL_INT(1, write_count);
    TEST_ASSERT_EQUAL_UINT32(SD_META_BLOCK, write_blocks[0]);
    TEST_ASSERT_EQUAL_UINT32(SD_META_MAGIC, last_meta_written.magic);
    TEST_ASSERT_EQUAL_UINT32(SD_LOG_START_BLOCK, last_meta_written.current_block);
}

void test_init_read_error_resets(void)
{
    fake_read_ret = SD_ERROR;
    fake_read_meta.magic         = SD_META_MAGIC;   /* piège : magic bon mais lecture KO */
    fake_read_meta.current_block = SD_LOG_START_BLOCK + 3;

    Storage_Init();

    TEST_ASSERT_EQUAL_UINT32(SD_LOG_START_BLOCK, current_block);
}

void test_write_sample_advances_and_saves_meta(void)
{
    uint8_t data[512] = {0};
    current_block = SD_LOG_START_BLOCK;

    SD_Status ret = Storage_WriteSampleBlock(data);

    TEST_ASSERT_EQUAL(SD_OK, ret);
    TEST_ASSERT_EQUAL_INT(2, write_count);
    TEST_ASSERT_EQUAL_UINT32(SD_LOG_START_BLOCK, write_blocks[0]);  /* 1) la donnée */
    TEST_ASSERT_EQUAL_UINT32(SD_META_BLOCK,      write_blocks[1]);  /* 2) les métadonnées */
    TEST_ASSERT_EQUAL_UINT32(SD_LOG_START_BLOCK + 1, current_block);
    TEST_ASSERT_EQUAL_UINT32(SD_LOG_START_BLOCK + 1, last_meta_written.current_block);
}

void test_write_sample_error_does_not_advance(void)
{
    uint8_t data[512] = {0};
    current_block  = SD_LOG_START_BLOCK;
    fake_write_ret = SD_ERROR;

    SD_Status ret = Storage_WriteSampleBlock(data);

    TEST_ASSERT_EQUAL(SD_ERROR, ret);
    TEST_ASSERT_EQUAL_UINT32(SD_LOG_START_BLOCK, current_block);
    TEST_ASSERT_EQUAL_INT(1, write_count);   /* pas de sauvegarde des métadonnées */
}

void test_write_sample_wraps_at_end_of_log(void)
{
    uint8_t data[512] = {0};
    current_block = SD_LOG_START_BLOCK + SD_LOG_BLOCK_COUNT - 1;   /* dernier bloc */

    Storage_WriteSampleBlock(data);

    TEST_ASSERT_EQUAL_UINT32(SD_LOG_START_BLOCK, current_block);
}

/* ---------- main ---------- */
