/*******************************************************************************
* File Name: pps.h
*
* Description:
*  This file contains the structure declaration and function prototypes used in
*  the USB PD Sink PPS Code example.
*
* Related Document: See README.md
*
*******************************************************************************
* Copyright 2023-2024, Cypress Semiconductor Corporation (an Infineon company) or
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
#ifndef SRC_PPS_H_
#define SRC_PPS_H_

/*******************************************************************************
 * Header files
 ******************************************************************************/
#include "cy_pdutils_sw_timer.h"
#include "cy_pdstack_common.h"

/*******************************************************************************
 * Macros
 ******************************************************************************/

/*
 * The drive signal of the GPIO connected to Capsense Widget LED's to turn it ON.
 */
#define LED_ON                                  (0u)

/*
 * The drive signal of the GPIO connected to Capsense Widget LED's to turn it OFF.
 */
#define LED_OFF                                 (1u)

/*
 * Mask to select PDO type.
 */
#define PDO_MASK                                (0x0F)

/*
 * Mask to select APDO type.
 */
#define APDO_MASK                               (0xF0)

/*****************************************************************************
 * Data struct definition
 ****************************************************************************/
/**
 * @typedef en_supply_type_t
 * @brief Types of power supply.
 */
typedef enum {
    FIXED_SUPPLY                     = 0x00, /**< Fixed Supply */
    BATTERY_SUPPLY                   = 0x01, /**< Battery Supply */
    VARIABLE_SUPPLY                  = 0x02, /**< Variable Supply */
    PROGRAMMABLE_POWER_SUPPLY        = 0x03, /**< Programmable Power Supply */
    EPR_ADJUSTABLE_VOLTAGE_SUPPLY    = 0x13, /**< EPR Adjustable Voltage Supply */
    SPR_ADJUSTABLE_VOLTAGE_SUPPLY    = 0x23, /**< SPR Adjustable Voltage Supply */
} en_supply_type_t;

/******************************************************************************
 * Global function declaration
 ******************************************************************************/

extern void updatePPScontract(int16_t volt, int16_t cur);
void pps_timer_cb(cy_timer_id_t id, void *callbackContext);

#endif /* SRC_PPS_H_ */
