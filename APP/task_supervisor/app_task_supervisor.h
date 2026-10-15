/**
 * @file    app_task_supervisor.h
 * @brief   Interface de la tache superviseur et des types partages entre taches.
 *
 * @details Ce fichier definit :
 *          - La machine d'etat systeme (@ref SystemState_t) via supervisor_logic.h.
 *          - Le prototype de la tache superviseur (@ref Task_Supervisor).
 *
 *          La variable globale @ref current_state est lue par toutes les taches
 *          pour adapter leur comportement a l'etat du systeme.
 */

#ifndef INC_APP_TASK_SUPERVISOR_H_
#define INC_APP_TASK_SUPERVISOR_H_

#include "main.h"
#include "FreeRTOS.h"
#include "task.h"
#include "stdbool.h"
#include "watchdog.h"
#include "supervisor_logic.h"

/**
 * @brief  Tache FreeRTOS de supervision et de gestion du watchdog.
 *
 * @details Cette tache :
 *          - Recoit les rapports de sante depuis health_queue.
 *          - Met a jour la machine d'etat systeme.
 *          - Raffraichit l'IWDG via watchdog_refresh().
 *          - Gere les transitions vers STATE_FATAL_ERROR apres timeout.
 *
 * @param[in] pvParameters  Non utilise (NULL).
 */
void Task_Supervisor(void *pvParameters);

/**
 * @brief  Raffraichit l'IWDG uniquement si toutes les taches sont vivantes.
 *
 * @details Attend (max 2 s) que WD_BIT_SENSOR ET WD_BIT_STORAGE soient
 *          simultanement leves dans wdEventGroup.  Si les deux bits sont
 *          presents, l'IWDG est nourri.  Sinon, l'IWDG n'est pas raffraichi
 *          et expirera apres ~10 s si aucune autre tache ne le nourrit.
 *
 * @note    Appelee depuis Task_Supervisor en etats degrade et fatal.
 */
void watchdog_refresh(void);

#endif /* INC_APP_TASK_SUPERVISOR_H_ */
