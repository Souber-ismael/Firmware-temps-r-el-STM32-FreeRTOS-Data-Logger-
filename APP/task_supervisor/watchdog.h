/**
 * @file    watchdog.h
 * @brief   Interface du module watchdog materiel (IWDG) et du groupe d'evenements
 *          de supervision des taches FreeRTOS.
 *
 * @details Le module combine deux mecanismes complementaires :
 *
 *          1. **IWDG materiel** : temporisateur independant cadence sur le LSI
 *             (~40 kHz).  Avec prescaler 128 et reload 3242, le timeout est
 *             d'environ 10 s.  Si personne ne raffraichit l'IWDG dans ce delai,
 *             le MCU se remet a zero.
 *
 *          2. **Event group logiciel** : chaque tache active set son bit
 *             (WD_BIT_SENSOR ou WD_BIT_STORAGE) a chaque iteration.
 *             La tache superviseur verifie que TOUS les bits sont leves avant
 *             de raffraichir l'IWDG.  Cela garantit que le watchdog n'est nourri
 *             que si toutes les taches sont vivantes.
 *
 * @note    Appeler IWDG_INIT() depuis main() avant vTaskStartScheduler().
 *          Ne pas appeler IWDG_Start() seul — il ne creerait pas le wdEventGroup.
 */

#ifndef INC_WATCHDOG_H_
#define INC_WATCHDOG_H_

#include "stm32f1xx_hal.h"
#include "FreeRTOS.h"
#include "event_groups.h"

/** @defgroup WD_Bits Bits de vivacite des taches dans wdEventGroup
 *  @{
 */
#define WD_BIT_SENSOR   (1 << 0) /**< Bit set par Task_Main (capteur) a chaque mesure    */
#define WD_BIT_STORAGE  (1 << 1) /**< Bit set par Task_Storage a chaque iteration        */
#define WD_ALL_BITS     (WD_BIT_SENSOR | WD_BIT_STORAGE) /**< Masque de tous les bits    */
/** @} */

/** @brief Handle du groupe d'evenements de supervision. Cree par IWDG_INIT(). */
extern EventGroupHandle_t wdEventGroup;

/** @brief Handle du watchdog materiel IWDG. Configure par IWDG_INIT(). */
extern IWDG_HandleTypeDef hiwdg;

/**
 * @brief  Configure et demarre le watchdog materiel IWDG.
 *
 * @details Prescaler = 128, Reload = 3242.
 *          Timeout = (3242 + 1) * 128 / 40000 Hz ≈ 10.37 secondes.
 *          Une fois demarre, l'IWDG ne peut plus etre arrete sans reset.
 *
 * @note    Appelee en interne par IWDG_INIT(). Peut etre appelee seule
 *          pour les tests, mais preferer IWDG_INIT() en production.
 */
void IWDG_Start(void);

/**
 * @brief  Initialise le watchdog materiel ET cree le groupe d'evenements FreeRTOS.
 *
 * @details Appelle IWDG_Start() puis xEventGroupCreate() pour initialiser
 *          wdEventGroup.  A appeler depuis main() apres l'initialisation
 *          des peripheriques et avant vTaskStartScheduler().
 *
 * @warning Ne pas appeler depuis une tache FreeRTOS.  L'IWDG demarre
 *          immediatement apres cet appel : les taches doivent nourrir
 *          le watchdog dans les 10 secondes suivantes.
 */
void IWDG_INIT(void);

#endif /* INC_WATCHDOG_H_ */