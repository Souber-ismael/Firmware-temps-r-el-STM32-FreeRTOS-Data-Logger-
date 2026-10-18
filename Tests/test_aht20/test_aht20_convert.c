/*
 * test_aht20_convert.c
 *
 *  Created on: Oct 12, 2026
 *      Author: admin
 */



#include "unity.h"
#include "aht20_convert.h"

void setUp(void) {}
void tearDown(void) {}

void test_conversion_25_degres_50_pourcent(void)
{
    /* status, humidité, humidité/température, température */
    const uint8_t data[6] = {
        0x08, 0x80, 0x00, 0x06, 0x00, 0x00
    };

    int16_t temp_int;
    uint8_t temp_frac;
    uint8_t hum_int;
    uint8_t hum_frac;

    SensorStatus status = aht20_convert(
        data,
        &temp_int, &temp_frac,
        &hum_int, &hum_frac
    );

    TEST_ASSERT_EQUAL(SENSOR_STATUS_OK, status);
    TEST_ASSERT_EQUAL_INT16(25, temp_int);
    TEST_ASSERT_EQUAL_UINT8(0, temp_frac);
    TEST_ASSERT_EQUAL_UINT8(50, hum_int);
    TEST_ASSERT_EQUAL_UINT8(0, hum_frac);
}

void test_conversion_minimal(void)
{
    /* status, humidité, humidité/température, température */
    const uint8_t data[6] = {
        0x08, 0x00, 0x00, 0x00, 0xCC, 0xCD
    };

    int16_t temp_int;
    uint8_t temp_frac;
    uint8_t hum_int;
    uint8_t hum_frac;

    SensorStatus status = aht20_convert(
        data,
        &temp_int, &temp_frac,
        &hum_int, &hum_frac
    );

    TEST_ASSERT_EQUAL(SENSOR_STATUS_OK, status);
    TEST_ASSERT_EQUAL_INT16(-40, temp_int);
    TEST_ASSERT_EQUAL_UINT8(0, temp_frac);
    TEST_ASSERT_EQUAL_UINT8(0, hum_int);
    TEST_ASSERT_EQUAL_UINT8(0, hum_frac);
}

void test_conversion_maximal(void)
{
    /* status, humidité, humidité/température, température */
    const uint8_t data[6] = {
        0x08, 0xFD, 0x67, 0x5A, 0xCC, 0xCD
    };

    int16_t temp_int;
    uint8_t temp_frac;
    uint8_t hum_int;
    uint8_t hum_frac;

    SensorStatus status = aht20_convert(
        data,
        &temp_int, &temp_frac,
        &hum_int, &hum_frac
    );

    TEST_ASSERT_EQUAL(SENSOR_STATUS_OK, status);
    TEST_ASSERT_EQUAL_INT16(85, temp_int);
    TEST_ASSERT_EQUAL_UINT8(0, temp_frac);


}

void test_conversion_horsplage(void)
{
    /* status, humidité, humidité/température, température */
    const uint8_t data[6] = {
        0x08, 0xFF, 0xFF, 0xFC, 0x00, 0x00
    };

    int16_t temp_int;
    uint8_t temp_frac;
    uint8_t hum_int;
    uint8_t hum_frac;

    SensorStatus status = aht20_convert(
        data,
        &temp_int, &temp_frac,
        &hum_int, &hum_frac
    );

    TEST_ASSERT_EQUAL(SENSOR_STATUS_INVALID_DATA, status);
    TEST_ASSERT_EQUAL_INT16(100, temp_int);
    TEST_ASSERT_EQUAL_UINT8(0, temp_frac);
    TEST_ASSERT_EQUAL_UINT8(99, hum_int);
    TEST_ASSERT_EQUAL_UINT8(99, hum_frac);

}

void test_temperature_negative(void){

   const uint8_t data[6] = {
	        0x08, 0x00, 0x00, 0x03, 0x33, 0x33
	    };

   int16_t temp_int;
   uint8_t temp_frac;
   uint8_t hum_int;
   uint8_t hum_frac;

   SensorStatus status = aht20_convert(
          data,
          &temp_int, &temp_frac,
          &hum_int, &hum_frac
      );

   TEST_ASSERT_EQUAL(SENSOR_STATUS_OK, status);
   TEST_ASSERT_EQUAL_INT16(-10, temp_int);
   TEST_ASSERT_EQUAL_UINT8(01, temp_frac);
   TEST_ASSERT_EQUAL_UINT8(0, hum_int);
   TEST_ASSERT_EQUAL_UINT8(0, hum_frac);

}


void test_temp_NULL(void){

   const uint8_t data[6] = {0};

   int16_t *temp_int =NULL ;
   uint8_t temp_frac;
   uint8_t hum_int;
   uint8_t hum_frac;

   SensorStatus status = aht20_convert(
          data,
		  temp_int, &temp_frac,
          &hum_int, &hum_frac
      );

   TEST_ASSERT_EQUAL(SENSOR_STATUS_INVALID_DATA, status);

}

void test_temperature_min_hum_max(void){

   const uint8_t data[6] = {
	        0x08, 0x00, 0x00, 0x00, 0x00, 0xD2
	    };

   int16_t temp_int;
   uint8_t temp_frac;
   uint8_t hum_int;
   uint8_t hum_frac;

   SensorStatus status = aht20_convert(
          data,
          &temp_int, &temp_frac,
          &hum_int, &hum_frac
      );

   TEST_ASSERT_EQUAL(SENSOR_STATUS_INVALID_DATA, status);


}

void test_hum_max(void){

	 const uint8_t data[6] = {
	        0x08, 0xFF, 0xFF, 0xF6, 0x00, 0x00
	    };

	 int16_t temp_int;
	    uint8_t temp_frac;
	    uint8_t hum_int;
	    uint8_t hum_frac;

	    SensorStatus status = aht20_convert(
	           data,
	           &temp_int, &temp_frac,
	           &hum_int, &hum_frac
	       );

    TEST_ASSERT_EQUAL(SENSOR_STATUS_INVALID_DATA, status);
	TEST_ASSERT_EQUAL_UINT8(99, hum_int);

}

void test_tempfrac_NULL(void){

   const uint8_t data[6] = {0};

   int16_t *temp_int ;
   uint8_t *temp_frac = NULL;
   uint8_t hum_int;
   uint8_t hum_frac;

   SensorStatus status = aht20_convert(
          data,
		  temp_int, temp_frac,
          &hum_int, &hum_frac
      );

   TEST_ASSERT_EQUAL(SENSOR_STATUS_INVALID_DATA, status);

}

void test_humint_NULL(void){

   const uint8_t data[6] = {0};

   int16_t temp_int  ;
   uint8_t temp_frac;
   uint8_t *hum_int =NULL;
   uint8_t hum_frac;

   SensorStatus status = aht20_convert(
          data,
		  &temp_int, &temp_frac,
          hum_int, &hum_frac
      );

   TEST_ASSERT_EQUAL(SENSOR_STATUS_INVALID_DATA, status);

}


void test_humfrac_NULL(void){

   const uint8_t data[6] = {0};

   int16_t temp_int  ;
   uint8_t temp_frac;
   uint8_t hum_int ;
   uint8_t *hum_frac =NULL;

   SensorStatus status = aht20_convert(
          data,
		  &temp_int, &temp_frac,
          &hum_int, hum_frac
      );

   TEST_ASSERT_EQUAL(SENSOR_STATUS_INVALID_DATA, status);

}
