/**
 * @file    storage.h
 * @brief   Interface de la couche de stockage logique sur microSD.
 *
 * @details Ce module gere l'organisation des donnees sur la microSD :
 *          - Bloc 999  : metadonnees (magic + pointeur de bloc courant).
 *          - Blocs 1000 a 50999 : journaux de mesures (rotation circulaire).
 *
 *          Au demarrage, Storage_Init() lit les metadonnees pour reprendre
 *          l'ecriture la ou elle s'etait arretee.  Quand le journal est plein,
 *          il repart du premier bloc (ecrasement des plus anciennes donnees).
 */

#ifndef INC_STORAGE_H_
#define INC_STORAGE_H_

#include <stdint.h>
#include "SD_DRIVER.h"

/** @defgroup Storage_Map Organisation des blocs SD
 *  @{
 */
#define SD_META_BLOCK       999    /**< Numero de bloc des metadonnees persistees          */
#define SD_LOG_START_BLOCK  1000   /**< Premier bloc du journal de mesures                */
#define SD_LOG_BLOCK_COUNT  50000  /**< Nombre de blocs alloues au journal (50 000 blocs) */
/** @} */

/**
 * @brief  Magic number pour valider l'integrite des metadonnees.
 *
 * @details Valeur arbitraire 0x53415345 ("SASE" en ASCII).
 *          Si le bloc de metadonnees ne contient pas cette valeur,
 *          le journal est reinitialise au bloc de depart.
 */
#define SD_META_MAGIC  0x53415345U

/**
 * @brief Structure des metadonnees persistees au bloc SD_META_BLOCK.
 *
 * @details Taille totale : 512 octets (taille d'un bloc SD).
 *          Le champ padding assure l'alignement sur un bloc complet.
 */
typedef struct {
    uint32_t magic;         /**< Signature de validite (SD_META_MAGIC = 0x53415345) */
    uint32_t current_block; /**< Prochain bloc de journal a ecrire                  */
    uint8_t  padding[500];  /**< Rembourrage pour atteindre exactement 512 octets   */
} SD_Metadata_t;

/**
 * @brief  Initialise la couche de stockage.
 *
 * @details Lit les metadonnees du bloc SD_META_BLOCK.
 *          Si le magic est valide, reprend l'ecriture a current_block.
 *          Sinon, repart de SD_LOG_START_BLOCK et ecrit les nouvelles metadonnees.
 *
 * @retval SD_OK   Toujours (meme si les metadonnees sont absentes, le systeme repart).
 */
SD_Status Storage_Init(void);

/**
 * @brief  Persiste les metadonnees (pointeur de bloc courant) sur la SD.
 *
 * @details Ecrit la structure SD_Metadata_t au bloc SD_META_BLOCK.
 *          Appele apres chaque ecriture de bloc de journal pour
 *          garantir la reprise correcte apres redemarrage.
 *
 * @retval SD_OK     Sauvegarde reussie.
 * @retval SD_ERROR  Erreur d'ecriture SPI/SD.
 */
SD_Status Storage_SaveMetadata(void);

/**
 * @brief  Ecrit un bloc de 512 octets de donnees dans le journal.
 *
 * @details Ecrit data_block au bloc courant, incremente le pointeur,
 *          applique la rotation si la fin du journal est atteinte,
 *          puis persiste les metadonnees.
 *
 * @param[in] data_block  Pointeur vers un buffer de 512 octets a ecrire.
 *
 * @retval SD_OK     Ecriture et sauvegarde metadonnees reussies.
 * @retval SD_ERROR  Erreur d'ecriture du bloc de donnees ou des metadonnees.
 */
SD_Status Storage_WriteSampleBlock(uint8_t *data_block);

#endif /* INC_STORAGE_H_ */