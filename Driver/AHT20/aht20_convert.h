/*
 * aht20_convert.h
 *
 *  Created on: Oct 12, 2026
 *      Author: admin
 */

#ifndef AHT20_AHT20_CONVERT_H_
#define AHT20_AHT20_CONVERT_H_

#ifndef AHT20_CONVERT_H
#define AHT20_CONVERT_H

#include <stdint.h>

#include "aht20_types.h"
/* À adapter si ces constantes existent déjà dans ton projet */




SensorStatus aht20_convert(
    const uint8_t data[6],
    int16_t *temp_int,
    uint8_t *temp_frac,
    uint8_t *hum_int,
    uint8_t *hum_frac
);

#endif


#endif /* AHT20_AHT20_CONVERT_H_ */
