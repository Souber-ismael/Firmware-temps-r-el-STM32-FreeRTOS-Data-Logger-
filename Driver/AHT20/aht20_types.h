/*
 * aht20_types.h
 *
 *  Created on: Oct 13, 2026
 *      Author: admin
 */

#ifndef AHT20_AHT20_TYPES_H_
#define AHT20_AHT20_TYPES_H_

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


/** @defgroup AHT20_Config Configuration du capteur AHT20
 *  @{
 */
#define AHT20_ADDR      0x38  /**< Adresse I2C du capteur AHT20 (fixe, non modifiable) */
#define SENSOR_TEMP_MAX  85   /**< Temperature maximale valide en degres Celsius        */
#define SENSOR_TEMP_MIN (-40) /**< Temperature minimale valide en degres Celsius        */
#define SENSOR_HUM_MAX  100   /**< Humidite maximale valide en pourcentage              */
#define SENSOR_HUM_MIN    0   /**< Humidite minimale valide en pourcentage              */
/** @} */

#endif /* AHT20_AHT20_TYPES_H_ */
