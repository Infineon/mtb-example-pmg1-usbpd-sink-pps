/******************************************************************************
* File Name: pps.c
*
* Description: This source file implements function prototypes for the
*              Programmable Power Supply (PPS) functions which are part of
*              the PMG1 MCU USB-PD Sink PPS demo Code Example for ModusToolBox.
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

/*******************************************************************************
 * Include header files
 ******************************************************************************/
#include "pps.h"

/******************************************************************************
 * Global variables declaration
 ******************************************************************************/
/* Structure for PPS Source PDO */
extern pps_source_t gl_pps_src_pdo;

/* Flag for PPS task callback */
volatile bool ppsTaskFlag = false;

/* Flag for PPS initialize contract */
volatile bool ppsInitContractFlag = false;

/* Flag for voltage increment/decrement (Increment = true, Decrement = false) */
volatile bool voltIncDecFlag = true;

/* VBUS voltage */
extern uint32_t vbus_volt;

/* USB PD context */
extern cy_stc_pdstack_context_t gl_PdStackPort0Ctx;

/* Timer context */
extern cy_stc_pdutils_sw_timer_t gl_TimerCtx;

/*******************************************************************************
* Function Name: pps_callback
********************************************************************************
* Summary:
*   - Set the ppsTaskFlag
*
* Parameters:
*  id - Timer ID
*  callbackContext
*
* Return:
*  None
*
*******************************************************************************/
void pps_callback (
        cy_timer_id_t id,            /**< Timer ID for which callback is being generated. */
        void *callbackContext)       /**< Timer module Context. */
{
    ppsTaskFlag = true;
}

/*******************************************************************************
* Function Name: pps_init_contract_callback
********************************************************************************
* Summary:
*   - Set the ppsInitContractFlag
*
* Parameters:
*  id - Timer ID
*  callbackContext
*
* Return:
*  None
*
*******************************************************************************/
void pps_init_contract_callback (
        cy_timer_id_t id,            /**< Timer ID for which callback is being generated. */
        void *callbackContext)       /**< Timer module Context. */
{
    ppsInitContractFlag = true;
}

/*******************************************************************************
* Function Name: led_callback
********************************************************************************
* Summary:
*   - Toggles the User LED at an interval of 500ms if PPS source is connected
*   - Glows the User LED if Non-PPS source is connected
*
* Parameters:
*  id - Timer ID
*  callbackContext
*
* Return:
*  None
*
*******************************************************************************/
void led_callback (
        cy_timer_id_t id,            /**< Timer ID for which callback is being generated. */
        void *callbackContext)       /**< Timer module Context. */
{
    /* Toggle the User LED */
    Cy_GPIO_Inv(CYBSP_USER_LED_PORT, CYBSP_USER_LED_PIN);

    /* Start the software timer again */
    Cy_PdUtils_SwTimer_Start (&gl_TimerCtx, callbackContext, id,
            USER_LED_BLINK_DURATION, led_callback);
}

/*******************************************************************************
* Function Name: pps_request_5v
********************************************************************************
* Summary:
*   - Requests PPS 5V
*
* Parameters:
*  None
*
* Return:
*  None
*
*******************************************************************************/
void pps_request_5v()
{
    cy_pd_pd_do_t pps_pdo;

    /* Structure to hold PD command buffer */
    cy_stc_pdstack_dpm_pd_cmd_buf_t extd_dpm_buf;

    /* Assigning the PPS RDO with voltage */
    pps_pdo.rdo_pps.outVolt = 0xFA;

    /* Assigning the PPS RDO with minimum current */
    pps_pdo.rdo_pps.opCur = gl_pps_src_pdo.min_curr;

    /* Setting false to USB communication capabilities */
    pps_pdo.rdo_pps.usbCommCap = 0x0;

    /* Assigning the object position */
    pps_pdo.rdo_pps.objPos = gl_pps_src_pdo.obj_position;

    /* Setting true to unChunked extended message support */
    pps_pdo.rdo_pps.unchunkSup = true;

    /* Setting zero to reserved bits */
    pps_pdo.rdo_pps.rsvd1 = 0x0;
    pps_pdo.rdo_pps.rsvd2 = 0x0;
    pps_pdo.rdo_pps.rsvd3 = 0x0;

    /* Extended data */
    extd_dpm_buf.cmdSop                = (cy_en_pd_sop_t)CY_PD_SOP;
    extd_dpm_buf.noOfCmdDo             = 1;
    extd_dpm_buf.datPtr                = NULL;
    extd_dpm_buf.timeout               = 33;
    extd_dpm_buf.cmdDo[0]              = pps_pdo;

    /* Send Request PD command */
    Cy_PdStack_Dpm_SendPdCommand(&gl_PdStackPort0Ctx, CY_PDSTACK_DPM_CMD_SEND_REQUEST, &extd_dpm_buf, false, NULL);

    /* Conversion (As min and max voltage is parsed from source capabilities, it will
     * be in terms of 100mV, but for PPS Request Data object, it should be in terms of
     * 20mV) */
    gl_pps_src_pdo.min_volt = gl_pps_src_pdo.min_volt * PPS_VOLTAGE_CONVERT;

    gl_pps_src_pdo.max_volt = gl_pps_src_pdo.max_volt * PPS_VOLTAGE_CONVERT;

    vbus_volt = gl_pps_src_pdo.min_volt;

    /* Start a timer for 500 milli seconds for Policy engine to request any PPS PDO */
    Cy_PdUtils_SwTimer_Start (&gl_TimerCtx, (void *)&gl_PdStackPort0Ctx, (cy_timer_id_t)PPS_REQUEST_TIMER_ID,
            PPS_REQ_TIME, pps_callback);
}

/*******************************************************************************
* Function Name: increment_decrement_voltage
********************************************************************************
* Summary:
*   - Increments the VBUS voltage from minimum to maximum (Step size: 100mV)
*   - Decrements the VBUS voltage from maximum to minimum (Step size: 100mV)
*
* Parameters:
*  None
*
* Return:
*  None
*
*******************************************************************************/
void increment_decrement_voltage()
{
    /* Structure to hold PD command buffer */
    cy_stc_pdstack_dpm_pd_cmd_buf_t extd_dpm_buf;

    /* PPS data object */
    cy_pd_pd_do_t pps_pdo;

    /* Checking if the vbus_volt variable is equal to the minimum voltage parsed
     * from the PPS source PDOs. If it is equal, vbus_volt is assigned with the
     * minimum voltage and voltIncDecFlag is set to true (Increment) */
    if (vbus_volt == gl_pps_src_pdo.min_volt)
    {
        vbus_volt = gl_pps_src_pdo.min_volt;
        voltIncDecFlag = true;
    }

    /* Assigning the PPS RDO with voltage */
    pps_pdo.rdo_pps.outVolt = vbus_volt;

    /* Checking if voltIncDecFlag is true (increment) */
    if (voltIncDecFlag == true)
    {
        /* Checking if vbus_volt is less than the maximum voltage from the PPS source.
         * If true, vbus_volt is incremented by 5 (As one unit is 20mV) */
        if (vbus_volt < gl_pps_src_pdo.max_volt)
        {
            vbus_volt += PPS_STEP_INCREMENT;
        }
        /* Checking if vbus_volt is equal to the maximum voltage from the PPS source.
         * If true, voltIncDecFlag is set to false (decrement) */
        else if (vbus_volt == gl_pps_src_pdo.max_volt)
        {
            voltIncDecFlag = false;
        }
    }
    /* Checking if voltIncDecFlag is false (decrement) */
    else if (voltIncDecFlag == false)
    {
        /* Checking if vbus_volt is greater than the minimum voltage from the PPS source.
         * If true, vbus_volt is decremented by 5 (As one unit is 20mV) */
        if (vbus_volt > gl_pps_src_pdo.min_volt)
        {
             vbus_volt -= PPS_STEP_DECREMENT;
        }
        /* Checking if vbus_volt is equal to the minimum voltage from the PPS source.
         * If true, voltIncDecFlag is set to true (increment) */
        else if (vbus_volt == gl_pps_src_pdo.min_volt)
        {
            voltIncDecFlag = true;
        }
    }

    /* Assigning the PPS RDO with minimum current */
    pps_pdo.rdo_pps.opCur = gl_pps_src_pdo.min_curr;

    /* Setting false to USB communication capabilities */
    pps_pdo.rdo_pps.usbCommCap = 0x0;

    /* Assigning the object position */
    pps_pdo.rdo_pps.objPos = gl_pps_src_pdo.obj_position;

    /* Setting true to unChunked extended message support */
    pps_pdo.rdo_pps.unchunkSup = true;

    /* Setting zero to reserved bits */
    pps_pdo.rdo_pps.rsvd1 = 0x0;
    pps_pdo.rdo_pps.rsvd2 = 0x0;
    pps_pdo.rdo_pps.rsvd3 = 0x0;

    /* Extended data */
    extd_dpm_buf.cmdSop                = (cy_en_pd_sop_t)CY_PD_SOP;
    extd_dpm_buf.noOfCmdDo             = 1;
    extd_dpm_buf.datPtr                = NULL;
    extd_dpm_buf.timeout               = 33;
    extd_dpm_buf.cmdDo[0]              = pps_pdo;

    /* Send Request PD command */
    Cy_PdStack_Dpm_SendPdCommand(&gl_PdStackPort0Ctx, CY_PDSTACK_DPM_CMD_SEND_REQUEST, &extd_dpm_buf, false, NULL);
}

/*******************************************************************************
* Function Name: pps_task
********************************************************************************
* Summary:
*   - Increments the VBUS voltage from minimum to maximum
*   - Decrements the VBUS voltage from maximum to minimum
*   - Starts the software time of 500ms
*
* Parameters:
*  None
*
* Return:
*  None
*
*******************************************************************************/
void pps_task(void)
{
    if (ppsInitContractFlag == true)
    {
        /* Request PPS 5V */
        pps_request_5v();

        ppsInitContractFlag = false;
    }

    if (ppsTaskFlag == true)
    {
        /* Increment and decrement VBUS voltage */
        increment_decrement_voltage();

        /* Start a timer for 500 milli seconds for Policy engine to request any PPS PDO */
        Cy_PdUtils_SwTimer_Start (&gl_TimerCtx, (void *)&gl_PdStackPort0Ctx, (cy_timer_id_t)PPS_REQUEST_TIMER_ID,
                PPS_REQ_TIME, pps_callback);

        /* Clear the PPS task flag */
        ppsTaskFlag = false;
    }
}
