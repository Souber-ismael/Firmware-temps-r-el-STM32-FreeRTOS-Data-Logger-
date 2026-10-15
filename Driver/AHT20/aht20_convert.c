/*
 * aht20_convert.c
 *
 *  Created on: Oct 12, 2026
 *      Author: admin
 */



#include "aht20_convert.h"
#include "stddef.h"

SensorStatus aht20_convert(
    const uint8_t data[6],
    int16_t *temp_int,
    uint8_t *temp_frac,
    uint8_t *hum_int,
    uint8_t *hum_frac)
{
    if (data == NULL ||
        temp_int == NULL || temp_frac == NULL ||
        hum_int == NULL || hum_frac == NULL)
    {
        return SENSOR_STATUS_INVALID_DATA;
    }

    /* Extraction des valeurs brutes sur 20 bits */
    uint32_t rawHum =
        ((uint32_t)data[1] << 12) |
        ((uint32_t)data[2] << 4)  |
        ((uint32_t)data[3] >> 4);

    uint32_t rawTemp =
        ((uint32_t)(data[3] & 0x0F) << 16) |
        ((uint32_t)data[4] << 8) |
        (uint32_t)data[5];

    /* Calcul en centièmes pour éviter les float */
    uint32_t hum_centi =
        (uint32_t)(((uint64_t)rawHum * 10000ULL)
                   / 1048576ULL);

    int32_t temp_centi =
        (int32_t)(((uint64_t)rawTemp * 20000ULL)
                  / 1048576ULL) - 5000;

    /* Séparer partie entière et fraction */
    *hum_int  = (uint8_t)(hum_centi / 100);
    *hum_frac = (uint8_t)(hum_centi % 100);

    *temp_int = (int16_t)(temp_centi / 100);

    int32_t frac = temp_centi % 100;
    if (frac < 0)
    {
        frac = -frac;
    }

    *temp_frac = (uint8_t)frac;

    /* Vérification des plages */
    if (*temp_int > SENSOR_TEMP_MAX ||
        *temp_int < SENSOR_TEMP_MIN ||
        *hum_int > SENSOR_HUM_MAX)
    {
        return SENSOR_STATUS_INVALID_DATA;
    }

    return SENSOR_STATUS_OK;
}
