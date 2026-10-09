/**
 * @file    SD_DRIVER.h
 * @brief   Interface publique du driver microSD en mode SPI.
 *
 * @details Ce module gere l'initialisation de la carte microSD via le
 *          protocole SPI (mode 0, CPOL=0 CPHA=0) et expose les fonctions
 *          de lecture et d'ecriture de blocs de 512 octets.
 *
 *          Materiel : SPI2 sur PB13 (SCK), PB14 (MISO), PB15 (MOSI).
 *          Chip Select : PA4 (GPIO sortie push-pull).
 *
 *          Sequence d'initialisation : power-up 80 clocks → CMD0 → CMD8
 *          → ACMD41 (max 20 tentatives) → CMD58 (lecture OCR pour SDHC/SDSC).
 */

#ifndef INC_SD_DRIVER_H_
#define INC_SD_DRIVER_H_

#include "stm32f1xx_hal.h"
#include "hal_spi.h"

/**
 * @brief Codes de retour des fonctions du driver SD.
 */
typedef enum {
    SD_OK,               /**< Operation reussie                                   */
    SD_ERROR,            /**< Erreur SPI generique                                */
    SD_TIMEOUT,          /**< Timeout SPI                                         */
    SD_BUSY,             /**< SPI occupe                                          */
    SD_NOT_READY,        /**< Carte SD pas prete (r1 != 0x00 apres ACMD41/CMD58)  */
    SD_ERROR_TIMEOUT,    /**< Timeout lors de l'attente du token de donnees       */
    SD_ERR_DATA_TOKEN,   /**< Token recu invalide (attendu 0xFE)                  */
    SD_ERROR_CDM,        /**< Erreur de reponse a une commande SD (r1 != attendu) */
    SD_ERR_RESPONSE,     /**< Data response byte invalide apres ecriture          */
    SD_ERR_WRITE_TIMEOUT,/**< Timeout lors de l'attente de fin d'ecriture         */
    SD_NOT_FOUND         /**< Commande rejetee par la carte (r1 != 0x00)          */
} SD_Status;

/**
 * @brief  Envoie une commande SD en mode SPI et lit la reponse R1.
 *
 * @details Gere le chip select (CS bas avant, CS haut apres sauf CMD17/CMD24).
 *          Pour CMD17 et CMD24, CS reste bas apres l'appel pour permettre
 *          la transaction de donnees dans la meme session SPI.
 *
 * @param[in] cmd  Numero de commande SD (0..63, sans le bit 0x40).
 * @param[in] arg  Argument 32 bits de la commande.
 * @param[in] crc  Octet CRC (0x95 pour CMD0, 0x87 pour CMD8, 0xFF sinon).
 *
 * @retval SD_OK           Commande acceptee (r1 == 0x00 ou 0x01 selon la commande).
 * @retval SD_ERROR_CDM    Reponse R1 incorrecte.
 * @retval SD_NOT_READY    Carte pas prete (ACMD41, CMD58).
 * @retval SD_NOT_FOUND    Commande rejetee (CMD17, CMD24).
 */
SD_Status SD_SendCmd(uint8_t cmd, uint32_t arg, uint8_t crc);

/**
 * @brief  Effectue la sequence de power-up de la carte SD.
 *
 * @details Maintient CS haut et envoie 80 coups d'horloge (10 octets 0xFF)
 *          pour laisser la carte s'initialiser en mode SPI natif.
 *
 * @retval SD_OK     Sequence reussie.
 * @retval SD_ERROR  Erreur SPI.
 */
SD_Status SD_PowerUpSequence(void);

/**
 * @brief  Initialise la carte microSD en mode SPI.
 *
 * @details Sequence complete :
 *          1. Power-up (80 clocks CS haut).
 *          2. CMD0  → reset en mode SPI (r1 = 0x01).
 *          3. CMD8  → verifie la tension (r7 : 0x01 0xAA attendus).
 *          4. ACMD41 (CMD55 + CMD41) → active la carte (max 20 tentatives).
 *          5. CMD58  → lit l'OCR pour determiner SDHC ou SDSC.
 *
 * @retval SD_OK         Carte initialisee et prete.
 * @retval SD_NOT_READY  Carte ne sort pas de l'etat idle apres 20 tentatives.
 * @retval SD_ERROR_CDM  Reponse inattendue a CMD0 ou CMD8.
 */
SD_Status SD_Init(void);

/**
 * @brief  Lit un bloc de 512 octets depuis la carte SD.
 *
 * @details Envoie CMD17 avec l'adresse, attend le token 0xFE,
 *          puis lit 512 octets + 2 octets CRC (ignores).
 *          Pour SDSC, l'adresse est multipliee par 512 (adressage octet).
 *          Pour SDHC, l'adresse est passee directement (adressage bloc).
 *
 * @param[in]  addr  Numero de bloc (0-base).
 * @param[out] buf   Buffer de reception d'exactement 512 octets.
 *
 * @retval SD_OK              Bloc lu avec succes.
 * @retval SD_NOT_FOUND       CMD17 rejetee par la carte.
 * @retval SD_ERROR_TIMEOUT   Token 0xFE non recu dans les 2 secondes.
 * @retval SD_ERR_DATA_TOKEN  Token recu different de 0xFE.
 */
SD_Status SD_READBLOC(uint32_t addr, uint8_t *buf);

/**
 * @brief  Ecrit un bloc de 512 octets sur la carte SD.
 *
 * @details Sequence :
 *          1. Pulse CS haut + 2 dummy bytes (nettoyage bus).
 *          2. CS bas + attente ready (0xFF).
 *          3. Construction et envoi de CMD24 avec l'adresse correcte.
 *          4. Attente R1 == 0x00 (max 100 tentatives).
 *          5. Envoi token 0xFE + 512 octets de donnees + 2 octets CRC.
 *          6. Lecture data response byte et attente fin d'ecriture interne.
 *
 * @param[in] addr    Numero de bloc cible (0-base).
 * @param[in] buffer  Buffer source de 512 octets a ecrire.
 *
 * @retval SD_OK         Ecriture reussie.
 * @retval SD_NOT_READY  R1 != 0x00 apres CMD24.
 * @retval SD_ERROR      Erreur SPI pendant le transfert.
 */
SD_Status SD_WriteBlock(uint32_t addr, uint8_t *buffer);

#endif /* INC_SD_DRIVER_H_ */