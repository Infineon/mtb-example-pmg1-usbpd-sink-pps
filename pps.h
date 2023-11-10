/******************************************************************************
* File Name: pps.h
*
* Description: This header file contains the structure declarations and
*              function prototypes which are part of the PMG1 MCU
*              USB-PD Sink PPS demo Code Example for ModusToolBox.
*
* Related Document: See README.md
*
*******************************************************************************
* Copyright 2021-2023, Cypress Semiconductor Corporation (an Infineon company) or
* an affiliate of Cypress Semiconductor Corporation.  All rights reserved.
*
* This software, including source code, documentation and related
* materials ("Software") is owned by Cypress Semiconductor Corporation
* or one of its affiliates ("Cypress") and is protected by and subject to
* worldwide patent protection (United States and foreign),
* United States copyright laws and international treaty provisions.
* Therefore, you may use this Software only as provided in the license
* agreement accompanying the software package from which you
* obtained this Software ("EULA").
* If no EULA applies, Cypress hereby grants you a personal, non-exclusive,
* non-transferable license to copy, modify, and compile the Software
* source code solely for use in connection with Cypress's
* integrated circuit products.  Any reproduction, modification, translation,
* compilation, or representation of this Software except as specified
* above is prohibited without the express written permission of Cypress.
*
* Disclaimer: THIS SOFTWARE IS PROVIDED AS-IS, WITH NO WARRANTY OF ANY KIND,
* EXPRESS OR IMPLIED, INCLUDING, BUT NOT LIMITED TO, NONINFRINGEMENT, IMPLIED
* WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE. Cypress
* reserves the right to make changes to the Software without notice. Cypress
* does not assume any liability arising out of the application or use of the
* Software or any product or circuit described in the Software. Cypress does
* not authorize its products for use in any products where a malfunction or
* failure of the Cypress product may reasonably be expected to result in
* significant property damage, injury or death ("High Risk Product"). By
* including Cypress's product in a High Risk Product, the manufacturer
* of such system or application assumes all risk of such use and in doing
* so agrees to indemnify Cypress against all liability.
*******************************************************************************/

#ifndef SRC_APP_PPS_H_
#define SRC_APP_PPS_H_

/*******************************************************************************
 * Include header files
 ******************************************************************************/
#include "cybsp.h"
#include "cy_pdl.h"
#include "cy_pdutils_sw_timer.h"
#include "cy_pdstack_common.h"
#include "cy_pdstack_dpm.h"
#include <stdio.h>

/*******************************************************************************
 * Macro declarations
 ******************************************************************************/
/* PPS Request Timer */
#define PPS_REQUEST_TIMER_ID                (CY_PDUTILS_TIMER_USER_START_ID + 1)

/* PPS LED Timer */
#define PPS_LED_TIMER_ID                    (CY_PDUTILS_TIMER_USER_START_ID + 2)

/* PPS Initial contract timer */
#define PPS_INIT_CONTRACT_TIMER_ID          (CY_PDUTILS_TIMER_USER_START_ID + 3)

/* PPS Initial contract time delay (250 milliseconds) */
#define PPS_INIT_CONTRACT_DELAY             (1000U)

/* PPS Request Time Delay (500 milliseconds) */
#define PPS_REQ_TIME                        (500U)

/* User LED Blink Rate */
#define USER_LED_BLINK_DURATION             (500U)

/* PPS Supply Source */
#define PPS_SUPPLY_SOURCE                   (3U)

/* Voltage Conversion (100mV to 20mV) */
#define PPS_VOLTAGE_CONVERT                 (5U)

/* Voltage step increment */
#define PPS_STEP_INCREMENT                  (5U)

/* Voltage step decrement */
#define PPS_STEP_DECREMENT                  (5U)

/******************************************************************************
 * Structure/Enum type declaration
 ******************************************************************************/
typedef struct
{
    /* PPS Source PDO Maximum voltage in 20mV units */
    uint32_t max_volt;

    /* PPS Source PDO Minimum voltage in 20mV units */
    uint32_t min_volt;

    /* PPS Source PDO Maximum current in 50mA units */
    uint32_t min_curr;

    /* Object position */
    uint8_t obj_position;
}pps_source_t;

/******************************************************************************
 * Global function declaration
 ******************************************************************************/
void increment_decrement_voltage(void);
void pps_callback (cy_timer_id_t id, void *callbackContext);
void led_callback (cy_timer_id_t id, void *callbackContext);
void pps_init_contract_callback (cy_timer_id_t id, void *callbackContext);
void pps_request_5v(void);
void pps_task(void);

#endif /* SRC_APP_PPS_H_ */
