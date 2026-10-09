/**
 * @file    aht20.h
 * @brief   Interface publique du driver AHT20 (capteur temperature/humidite).
 *
 * @details Ce module expose les fonctions d'initialisation et de lecture
 *          du capteur AHT20 via I2C1.  Les valeurs retournees sont decomposees
 *          en partie entiere et partie fractionnaire afin d'eviter l'usage
 *          de flottants sur le Cortex-M3.
 *
 *          Adresse I2C fixe : 0x38.
 *          Plage temperature : -40 a +85 degres C.
 *          Plage humidite    :   0 a 100 %.
 */

#ifndef AHT20_H
#define AHT20_H

/** @defgroup AHT20_Config Configuration du capteur AHT20
 *  @{
 */
#define AHT20_ADDR      0x38  /**< Adresse I2C du capteur AHT20 (fixe, non modifiable) */
#define SENSOR_TEMP_MAX  85   /**< Temperature maximale valide en degres Celsius        */
#define SENSOR_TEMP_MIN (-40) /**< Temperature minimale valide en degres Celsius        */
#define SENSOR_HUM_MAX  100   /**< Humidite maximale valide en pourcentage              */
#define SENSOR_HUM_MIN    0   /**< Humidite minimale valide en pourcentage              */
/** @} */

#include "stm32f1xx_hal.h"
#include "hal_i2c.h"

/**
 * @brief Codes de retour des fonctions du driver AHT20.
 */
typedef enum {
    SENSOR_STATUS_OK = 0,        /**< Operation reussie                              */
    SENSOR_STATUS_NOT_FOUND,     /**< Capteur absent sur le bus I2C (NACK)           */
    SENSOR_COMMUNICATION_ERROR,  /**< Erreur de communication I2C (timeout, busy...) */
    SENSOR_STATUS_INVALID_DATA,  /**< Mesure hors des plages physiques acceptables   */
    SENSOR_STATUS_BUSY,          /**< Capteur occupé, mesure non disponible          */
    SENSOR_STATUS_BUS_ERROR,     /**< Erreur generique du bus I2C                    */
    SENSOR_NOT_CALIBRATED        /**< Capteur non calibre apres la commande 0xBE     */
} SensorStatus;

/**
 * @brief  Initialise le capteur AHT20.
 *
 * @details Sequence :
 *          1. Sonde l'adresse I2C pour verifier la presence du capteur.
 *          2. Envoie la commande de reset logiciel (0xBA).
 *          3. Lit le registre de statut ; si le bit de calibration (bit 3)
 *             est absent, envoie la commande d'initialisation (0xBE 0x08 0x00)
 *             et re-verifie la calibration.
 *
 * @retval SENSOR_STATUS_OK         Capteur initialise et calibre.
 * @retval SENSOR_STATUS_NOT_FOUND  Capteur absent sur le bus.
 * @retval SENSOR_NOT_CALIBRATED    Echec de calibration apres initialisation.
 * @retval SENSOR_COMMUNICATION_ERROR Erreur I2C.
 */
SensorStatus AHT20_Init(void);

/**
 * @brief  Lit la temperature et l'humidite du capteur AHT20.
 *
 * @details Sequence :
 *          1. Envoie la commande de mesure (0xAC 0x33 0x00).
 *          2. Attend la reponse du capteur (jusqu'a 5 tentatives, 20 ms entre chaque).
 *          3. Verifie le bit de busy dans l'octet de statut.
 *          4. Extrait les 20 bits bruts d'humidite et les 20 bits bruts de temperature.
 *          5. Convertit en valeurs physiques decomposees : partie entiere + fraction.
 *          6. Valide les plages.
 *
 * @param[out] temp_int   Partie entiere de la temperature (ex : 23 pour 23.45 C).
 * @param[out] temp_frac  Partie fractionnaire sur 2 chiffres (ex : 45 pour 23.45 C).
 * @param[out] hum_int    Partie entiere de l'humidite (ex : 58 pour 58.12 %).
 * @param[out] hum_frac   Partie fractionnaire sur 2 chiffres (ex : 12 pour 58.12 %).
 *
 * @retval SENSOR_STATUS_OK           Mesure valide.
 * @retval SENSOR_STATUS_BUSY         Capteur toujours occupe apres double tentative.
 * @retval SENSOR_STATUS_INVALID_DATA Valeurs hors plage physique.
 * @retval SENSOR_COMMUNICATION_ERROR Erreur I2C.
 */
SensorStatus AHT20_Read(uint8_t *temp_int, uint8_t *temp_frac,
                         uint8_t *hum_int,  uint8_t *hum_frac);

/**
 * @brief  Convertit un code d'erreur I2C en code d'erreur SensorStatus.
 *
 * @param[in] i2c_status  Code de retour de la couche HAL I2C.
 * @retval    SensorStatus equivalent.
 */
SensorStatus sensor_convert_i2c_status(i2c_status_t i2c_status);

#endif /* AHT20_H */