/**
 * @file    storage.c
 * @brief   Implementation de la couche de stockage logique sur microSD.
 *
 * @details Gere l organisation circulaire des blocs de journal sur la SD.
 *          Les metadonnees (pointeur de bloc courant) sont persistees au
 *          bloc SD_META_BLOCK apres chaque ecriture pour garantir la
 *          reprise correcte apres un redemarrage ou un reset watchdog.
 */

#include "storage.h"

/** @brief Numero du prochain bloc de journal a ecrire. Persiste sur SD. */
static uint32_t current_block;

/**
 * @brief  Initialise la couche de stockage et reprend le pointeur de bloc.
 * @details Lit le bloc SD_META_BLOCK. Si le magic correspond a SD_META_MAGIC,
 *          reprend current_block a la valeur persistee.
 *          Sinon (premiere utilisation ou SD vierge), repart de
 *          SD_LOG_START_BLOCK et ecrit les metadonnees initiales.
 * @retval SD_OK  Toujours retourne SD_OK.
 */
SD_Status Storage_Init(void)
{
    SD_Metadata_t meta;

    SD_Status ret = SD_READBLOC(SD_META_BLOCK, (uint8_t *)&meta);

    if (ret == SD_OK && meta.magic == SD_META_MAGIC) {
        /* Metadonnees valides : reprise du journal a l endroit exact */
        current_block = meta.current_block;
    } else {
        /* Premiere utilisation ou metadonnees corrompues : reinitialisation */
        current_block = SD_LOG_START_BLOCK;
        Storage_SaveMetadata();
    }

    return SD_OK;
}

/**
 * @brief  Persiste le pointeur de bloc courant sur la microSD.
 * @details Remplit la structure SD_Metadata_t avec le magic et current_block,
 *          puis ecrit le bloc au numero SD_META_BLOCK.
 * @retval SD_OK     Sauvegarde reussie.
 * @retval SD_ERROR  Erreur lors de l ecriture SPI/SD.
 */
SD_Status Storage_SaveMetadata(void)
{
    SD_Metadata_t meta = {0};

    meta.magic         = SD_META_MAGIC;
    meta.current_block = current_block;

    return SD_WriteBlock(SD_META_BLOCK, (uint8_t *)&meta);
}

/**
 * @brief  Ecrit un bloc de donnees de 512 octets dans le journal et avance le pointeur.
 * @details Ecrit data_block au bloc current_block, incremente le compteur,
 *          applique la rotation circulaire si la fin du journal est atteinte,
 *          puis appelle Storage_SaveMetadata() pour persister le nouvel etat.
 * @param[in] data_block  Buffer de 512 octets a ecrire sur la SD.
 * @retval SD_OK     Ecriture et mise a jour des metadonnees reussies.
 * @retval SD_ERROR  Erreur lors de l ecriture du bloc de donnees.
 */
SD_Status Storage_WriteSampleBlock(uint8_t *data_block)
{
    SD_Status ret;

    ret = SD_WriteBlock(current_block, data_block);
    if (ret != SD_OK) {
        return ret;
    }

    current_block++;

    /* Rotation circulaire : retour au debut du journal si la fin est atteinte */
    if (current_block >= SD_LOG_START_BLOCK + SD_LOG_BLOCK_COUNT) {
        current_block = SD_LOG_START_BLOCK;
    }

    ret = Storage_SaveMetadata();

    return ret;
}