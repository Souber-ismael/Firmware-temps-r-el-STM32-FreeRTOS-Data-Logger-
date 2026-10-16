/*
 * test_supervisor_logic.c
 *
 * Tests unitaires de la machine d'etat superviseur (Unity + GCC hote).
 */

#include "unity.h"
#include "supervisor_logic.h"

static HealthReport_t make_report(HealthSource_t source, SystemHealthEvent_t event)
{
    HealthReport_t report;

    report.source = source;
    report.event = event;
    report.last_error = 0;
    return report;
}

static void feed(HealthSource_t source, SystemHealthEvent_t event, uint32_t now_ms)
{
    HealthReport_t report = make_report(source, event);

    supervisor_process_event(&report, now_ms);
    supervisor_update(now_ms);
}

void setUp(void)
{
    supervisor_reset();
}

void tearDown(void) {}

void test_etat_initial_running(void)
{
    TEST_ASSERT_EQUAL(STATE_RUNNING, current_state);
}

void test_erreur_capteur_passe_en_sensor_degraded(void)
{
    feed(HEALTH_SRC_SENSOR, SYS_SENSOR_DEGRADED, 1000);

    TEST_ASSERT_EQUAL(STATE_SENSOR_DEGRADED, current_state);
}

void test_erreur_stockage_passe_en_storage_degraded(void)
{
    feed(HEALTH_SRC_STORAGE, SYS_STORAGE_DEGRADED, 1000);

    TEST_ASSERT_EQUAL(STATE_STORAGE_DEGRADED, current_state);
}

void test_rapport_null_ne_change_pas_letat(void)
{
    supervisor_process_event(NULL, 0);
    supervisor_update(0);

    TEST_ASSERT_EQUAL(STATE_RUNNING, current_state);
}

void test_ok_capteur_en_running_reste_running(void)
{
    feed(HEALTH_SRC_SENSOR, SYS_OK, 0);
    feed(HEALTH_SRC_STORAGE, SYS_OK, 10);


    TEST_ASSERT_EQUAL(STATE_RUNNING, current_state);
}

void test_deux_ok_capteur_ne_suffisent_pas_a_recuperer(void)
{
    feed(HEALTH_SRC_SENSOR, SYS_SENSOR_DEGRADED, 0);
    feed(HEALTH_SRC_SENSOR, SYS_OK, 100);
    feed(HEALTH_SRC_SENSOR, SYS_OK, 200);

    TEST_ASSERT_EQUAL(STATE_SENSOR_DEGRADED, current_state);
}

void test_trois_ok_capteur_recuperent_running(void)
{
    feed(HEALTH_SRC_SENSOR, SYS_SENSOR_DEGRADED, 0);
    feed(HEALTH_SRC_SENSOR, SYS_OK, 100);
    feed(HEALTH_SRC_SENSOR, SYS_OK, 200);
    feed(HEALTH_SRC_SENSOR, SYS_OK, 300);

    TEST_ASSERT_EQUAL(STATE_RUNNING, current_state);
}

void test_une_erreur_capteur_casse_le_streak(void)
{
    feed(HEALTH_SRC_SENSOR, SYS_SENSOR_DEGRADED, 0);
    feed(HEALTH_SRC_SENSOR, SYS_OK, 100);
    feed(HEALTH_SRC_SENSOR, SYS_OK, 200);
    feed(HEALTH_SRC_SENSOR, SYS_SENSOR_DEGRADED, 300);
    feed(HEALTH_SRC_SENSOR, SYS_OK, 400);
    feed(HEALTH_SRC_SENSOR, SYS_OK, 500);

    TEST_ASSERT_EQUAL(STATE_SENSOR_DEGRADED, current_state);
}

void test_un_ok_stockage_recupere_running(void)
{
    feed(HEALTH_SRC_STORAGE, SYS_STORAGE_DEGRADED, 0);
    feed(HEALTH_SRC_STORAGE, SYS_OK, 100);

    TEST_ASSERT_EQUAL(STATE_RUNNING, current_state);
}

void test_sensor_degraded_ignore_erreur_stockage_pour_letat(void)
{
    feed(HEALTH_SRC_SENSOR, SYS_SENSOR_DEGRADED, 0);
    feed(HEALTH_SRC_STORAGE, SYS_STORAGE_DEGRADED, 50);

    TEST_ASSERT_EQUAL(STATE_SENSOR_DEGRADED, current_state);
}

void test_storage_degraded_ignore_erreur_capteur_pour_letat(void)
{
    feed(HEALTH_SRC_STORAGE, SYS_STORAGE_DEGRADED, 0);
    feed(HEALTH_SRC_SENSOR, SYS_SENSOR_DEGRADED, 50);

    TEST_ASSERT_EQUAL(STATE_STORAGE_DEGRADED, current_state);
}

void test_timeout_capteur_des_t0_passe_en_fatal(void)
{
    feed(HEALTH_SRC_SENSOR, SYS_SENSOR_DEGRADED, 0);
    supervisor_update(DEGRADED_TO_FATAL_MS);

    TEST_ASSERT_EQUAL(STATE_FATAL_ERROR, current_state);
}

void test_timeout_capteur_40s_passe_en_fatal(void)
{
    feed(HEALTH_SRC_SENSOR, SYS_SENSOR_DEGRADED, 1000);
    supervisor_update(1000u + DEGRADED_TO_FATAL_MS);

    TEST_ASSERT_EQUAL(STATE_FATAL_ERROR, current_state);
}

void test_juste_avant_timeout_capteur_reste_degraded(void)
{
    feed(HEALTH_SRC_SENSOR, SYS_SENSOR_DEGRADED, 1000);
    supervisor_update(1000u + DEGRADED_TO_FATAL_MS - 1u);

    TEST_ASSERT_EQUAL(STATE_SENSOR_DEGRADED, current_state);
}

void test_timeout_stockage_40s_passe_en_fatal(void)
{
    feed(HEALTH_SRC_STORAGE, SYS_STORAGE_DEGRADED, 500);
    supervisor_update(500u + DEGRADED_TO_FATAL_MS);

    TEST_ASSERT_EQUAL(STATE_FATAL_ERROR, current_state);
}

void test_horloge_timeout_ne_redemarre_pas_sur_erreur_suivante(void)
{
    feed(HEALTH_SRC_SENSOR, SYS_SENSOR_DEGRADED, 0);
    feed(HEALTH_SRC_SENSOR, SYS_SENSOR_DEGRADED, 10000);
    supervisor_update(DEGRADED_TO_FATAL_MS);

    TEST_ASSERT_EQUAL(STATE_FATAL_ERROR, current_state);
}

void test_apres_recuperation_nouvelle_erreur_redevient_degraded(void)
{
    feed(HEALTH_SRC_STORAGE, SYS_STORAGE_DEGRADED, 0);
    feed(HEALTH_SRC_STORAGE, SYS_OK, 10);
    TEST_ASSERT_EQUAL(STATE_RUNNING, current_state);

    feed(HEALTH_SRC_STORAGE, SYS_STORAGE_DEGRADED, 20);
    TEST_ASSERT_EQUAL(STATE_STORAGE_DEGRADED, current_state);
}

void test_fatal_reste_fatal_sur_update(void)
{
    feed(HEALTH_SRC_SENSOR, SYS_SENSOR_DEGRADED, 0);
    supervisor_update(DEGRADED_TO_FATAL_MS);
    TEST_ASSERT_EQUAL(STATE_FATAL_ERROR, current_state);

    supervisor_update(DEGRADED_TO_FATAL_MS + 5000u);
    TEST_ASSERT_EQUAL(STATE_FATAL_ERROR, current_state);
}

