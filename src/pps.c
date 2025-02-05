/******************************************************************************
* File Name: pps.c
*
* Description: This file contains the functions related to PPS
*
* Related Document: See README.md
*
*******************************************************************************
* Copyright 2021-2024, Cypress Semiconductor Corporation (an Infineon company) or
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
 * Header files
 ******************************************************************************/
#include "pps.h"
#include "cy_pdutils_sw_timer.h"
#include "cy_pdl.h"
#include "cybsp.h"
#include "cy_usbpd_common.h"
#include "cy_pdstack_common.h"
#include "cy_pdstack_dpm.h"
#include "cy_pdutils.h"
#include "config.h"
#include "cy_app.h"

/******************************************************************************
 * Macro definitions
 ******************************************************************************/


/******************************************************************************
 * Global variables declaration
 ******************************************************************************/
/* Variable to store the current contract voltage */
static uint16_t gl_cur_voltage;

/* Variable to store the min and max pps voltage */
static uint16_t gl_max_pps_vol;

/* Timer context */
extern cy_stc_pdutils_sw_timer_t gl_TimerCtx;

/* USB PD context */
extern cy_stc_pdstack_context_t gl_PdStackPort0Ctx;

/*******************************************************************************
* Function Name: pps_timer_cb
********************************************************************************
* Summary:
*  Sets the desire PPS contract request rate
*
* Parameters:
*  id - Timer ID
*  callbackContext - Context
*
* Return:
*  None
*
*******************************************************************************/
void pps_timer_cb(
        cy_timer_id_t id,            /**< Timer ID for which callback is being generated. */
        void *callbackContext)       /**< Timer module Context. */
{
    uint16_t cur = 900;             //Default snk current in mA
    static uint16_t volt;

    if(gl_max_pps_vol == 0)
    {
        volt = VSAFE_5V;                    //First PPS contract
    }
    else
    {
        volt += PPS_STEP;
        if(volt > gl_max_pps_vol)
        {
            volt = VSAFE_5V;                //Minimum PPS voltage is limited to 5V
        }
    }

    updatePPScontract(volt, cur);
    Cy_PdUtils_SwTimer_Start (&gl_TimerCtx, callbackContext, id, PPS_REQ_TIMER, pps_timer_cb);
}


/*******************************************************************************
* Function Name: is_request_valid
********************************************************************************
* Summary:
*  Validates user request
*
* Parameters:
*  context - PdStack context
*  volt - Voltage in 50mV
*  cur - Current in 10mA
*
* Return:
*  true if the request is valid otherwise false
*
*******************************************************************************/
static bool is_request_valid(cy_stc_pdstack_context_t *context, uint16_t volt, uint16_t cur)
{
    cy_stc_pdstack_dpm_status_t *dpm_stat = &(context->dpmStat);
#if CY_PD_EPR_ENABLE
    cy_stc_pdstack_dpm_ext_status_t *dpmExt = &(context->dpmExtStat);
#endif /* CY_PD_EPR_ENABLE */
    cy_en_pdstack_pdo_t supply_type;
    uint8_t snk_pdo_idx;
    uint8_t snk_pdo_len = dpm_stat->curSnkPdocount;
    cy_pd_pd_do_t* pdo_snk;

#if CY_PD_EPR_ENABLE
    if(dpmExt->eprActive)
    {
        snk_pdo_len = CY_PD_MAX_NO_OF_PDO + dpmExt->curEprSnkPdoCount;
    }
#endif /* CY_PD_EPR_ENABLE */

    for(snk_pdo_idx = 0; snk_pdo_idx < snk_pdo_len; snk_pdo_idx++)
    {
        pdo_snk = (cy_pd_pd_do_t*)&(dpm_stat->curSnkPdo[snk_pdo_idx]);
        supply_type = (cy_en_pdstack_pdo_t)pdo_snk->fixed_snk.supplyType;

        switch(supply_type)
        {
            case CY_PDSTACK_PDO_FIXED_SUPPLY:
                if((volt == pdo_snk->fixed_snk.voltage) && (cur <= pdo_snk->fixed_snk.opCurrent))
                {
                    return true;
                }
                break;
            case CY_PDSTACK_PDO_VARIABLE_SUPPLY:
                if((volt >= pdo_snk->var_snk.minVoltage) && (volt <= pdo_snk->var_snk.maxVoltage))
                {
                    if(cur <= pdo_snk->var_snk.opCurrent)
                    {
                        return true;
                    }
                }
                break;
            case CY_PDSTACK_PDO_BATTERY:
                if((volt >= pdo_snk->bat_snk.minVoltage) && (volt <= pdo_snk->bat_snk.maxVoltage))
                {
                    uint16_t power = CY_PDUTILS_DIV_ROUND_UP(volt * cur, 500);
                    if(power <= pdo_snk->bat_snk.opPower)
                    {
                        return true;
                    }
                }
                break;
            case CY_PDSTACK_PDO_AUGMENTED:
                if(pdo_snk->pps_snk.apdoType == CY_PDSTACK_APDO_PPS)
                {
                    /* Convert PDO voltage to 50 mV from 100 mV unit */
                    if((volt >= pdo_snk->pps_snk.minVolt * 2u) && (volt <= pdo_snk->pps_snk.maxVolt * 2u))
                    {
                        /* Convert PDO current to 10mA from 50 mA unit */
                        if(cur <= pdo_snk->pps_snk.opCur * 5u)
                        {
                            return true;
                        }
                    }
                }
#if (CY_PD_EPR_AVS_ENABLE)
                if(pdo_snk->epr_avs_snk.apdoType == CY_PDSTACK_APDO_AVS)
                {
                    /* Convert PDO voltage to 50 mV from 100 mV unit */
                    if((volt >= pdo_snk->epr_avs_snk.minVolt * 2u) && (volt <= pdo_snk->epr_avs_snk.maxVolt * 2u))
                    {
                        /* Calculate the power in 250mW units */
                        uint16_t power = CY_PDUTILS_DIV_ROUND_UP(volt * cur, 500);
                        /* Convert PDP to 250mW units */
                        if(power <= pdo_snk->epr_avs_snk.pdp * 4u)
                        {
                            return true;
                        }
                    }
                }
#endif /* CY_PD_EPR_AVS_ENABLE */
                break;
            default:
                /* Do Nothing */
                break;
        }
    }
    return false;
}

/*******************************************************************************
* Function Name: select_src_pdo
********************************************************************************
* Summary:
*  Choose the source PDO that can provide requested voltage and current
*
* Parameters:
*  context - PdStack context
*  supply_type - Supply type
*  volt - Voltage in 50mV
*  Cur - Current in 10mA
*  srcCap - Pointer to source capabilities message
*
* Return:
*  uint8_t - Object position
*
*******************************************************************************/
static uint8_t select_src_pdo(cy_stc_pdstack_context_t *context, en_supply_type_t supply_type, uint16_t volt, uint16_t cur,
                              cy_stc_pdstack_pd_packet_t* srcCap)
{
    bool status = false;
#if CY_PD_EPR_ENABLE
    cy_stc_pdstack_dpm_ext_status_t *dpmExt = &(context->dpmExtStat);
#endif /* CY_PD_EPR_ENABLE */
    uint8_t src_pdo_idx;
    uint8_t src_pdo_len = srcCap->len;
    cy_pd_pd_do_t* pdo_src;
    uint8_t obj_pos = 0u;

#if CY_PD_EPR_ENABLE
    if(srcCap->hdr.hdr.extd && dpmExt->eprActive)
    {
        src_pdo_len = srcCap->hdr.hdr.dataSize / 4u;
    }
#endif /* CY_PD_EPR_ENABLE */

    for(src_pdo_idx = 0; src_pdo_idx < src_pdo_len; src_pdo_idx++)
    {
        status = false;
        pdo_src = (cy_pd_pd_do_t*)(&srcCap->dat[src_pdo_idx]);
        if((supply_type & PDO_MASK) == (cy_en_pdstack_pdo_t)pdo_src->fixed_src.supplyType)
        {
            switch((cy_en_pdstack_pdo_t)pdo_src->fixed_src.supplyType)
            {
                case CY_PDSTACK_PDO_FIXED_SUPPLY:
                    if((volt == pdo_src->fixed_src.voltage) && (cur <= pdo_src->fixed_src.maxCurrent))
                    {
                        status = true;
                    }
                    break;
                case CY_PDSTACK_PDO_VARIABLE_SUPPLY:
                    if((volt >= pdo_src->var_src.minVoltage) && (volt <= pdo_src->var_src.maxVoltage))
                    {
                        if(cur <= pdo_src->var_src.maxCurrent)
                        {
                            status = true;
                        }
                    }
                    break;
                case CY_PDSTACK_PDO_BATTERY:
                    if((volt >= pdo_src->bat_src.minVoltage) && (volt <= pdo_src->bat_src.maxVoltage))
                    {
                        uint16_t power = CY_PDUTILS_DIV_ROUND_UP(volt * cur, 500);
                        if(power <= pdo_src->bat_src.maxPower)
                        {
                            status = true;
                        }
                    }
                    break;
                case CY_PDSTACK_PDO_AUGMENTED:
                    if((supply_type >> 4u) == pdo_src->spr_avs_src.apdoType)
                    {
                        if(pdo_src->pps_src.apdoType == CY_PDSTACK_APDO_PPS)
                        {
                            /* Convert PDO voltage to 50 mV from 100 mV unit */
                            if((volt >= pdo_src->pps_src.minVolt * 2u) && (volt <= pdo_src->pps_src.maxVolt * 2u))
                            {
                                /* Convert PDO current to 10mA from 50 mA unit */
                                if(cur <= pdo_src->pps_src.maxCur * 5u)
                                {
                                    gl_max_pps_vol = pdo_src->pps_src.maxVolt * 100;
                                    status = true;
                                }
                            }
                        }
                        if(pdo_src->spr_avs_src.apdoType == CY_PDSTACK_APDO_SPR_AVS)
                        {
                            if((volt >= VSAFE_9V_IN_50MV) && (volt <= VSAFE_15V_IN_50MV))
                            {
                                if(cur <= pdo_src->spr_avs_src.maxCur1)
                                {
                                    status = true;
                                }
                            }
                            else if((volt > VSAFE_15V_IN_50MV) && (volt <= VSAFE_20V_IN_50MV))
                            {
                                if(cur <= pdo_src->spr_avs_src.maxCur2)
                                {
                                    status = true;
                                }
                            }
                        }
#if (CY_PD_EPR_AVS_ENABLE)
                        if(pdo_src->epr_avs_src.apdoType == CY_PDSTACK_APDO_AVS)
                        {
                            /* Convert PDO voltage to 50 mV from 100 mV unit */
                            if((volt >= pdo_src->epr_avs_src.minVolt * 2u) && (volt <= pdo_src->epr_avs_src.maxVolt * 2u))
                            {
                                /* Calculate the power in 250mW units */
                                uint16_t power = CY_PDUTILS_DIV_ROUND_UP(volt * cur, 500);
                                /* Convert PDP to 250mW units */
                                if(power <= pdo_src->epr_avs_src.pdp * 4u)
                                {
                                    status = true;
                                }
                            }
                        }
#endif /* CY_PD_EPR_AVS_ENABLE */
                    }
                    break;
                default:
                    /* Do Nothing */
                    break;
            }
            if(status == true)
            {
                obj_pos = src_pdo_idx + 1u;
            }
        }
    }

    return obj_pos;
}

/*******************************************************************************
* Function Name: send_request
********************************************************************************
* Summary:
*  Forms RDO and sends request message
*
* Parameters:
*  context - PdStack context
*  pdo_no - PDO number
*  volt - Voltage in 50mV
*  Cur - Current in 10mA
*  srcCap - Pointer to source capabilities message
*
* Return:
* CY_PDSTACK_STAT_SUCCESS if the request is successful
* CY_PDSTACK_STAT_FAILURE if the request is failed
*
*******************************************************************************/
static cy_en_pdstack_status_t send_request(cy_stc_pdstack_context_t *context, uint8_t pdo_no, uint16_t volt, uint16_t cur, cy_stc_pdstack_pd_packet_t* srcCap)
{
    cy_en_pdstack_status_t status;
#if (CY_PD_EPR_ENABLE)
    const cy_stc_pdstack_dpm_ext_status_t *dpmExtStat = &(context->dpmExtStat);
#endif /* (CY_PD_EPR_ENABLE) */
    cy_stc_pdstack_dpm_pd_cmd_buf_t cmd_buf;
    cy_pd_pd_do_t* pdo_src = &srcCap->dat[pdo_no - 1u];
    cy_pd_pd_do_t snkRdo;

    snkRdo.val = 0u;
    snkRdo.rdo_gen.noUsbSuspend = context->dpmStat.snkUsbSuspEn;
    snkRdo.rdo_gen.usbCommCap = context->dpmStat.snkUsbCommEn;
    snkRdo.rdo_gen.capMismatch = 0u;
#if (CY_PD_EPR_ENABLE)
    /* In request PDO index is SPR 1...7, EPR 8...13 */
    if(pdo_no > CY_PD_MAX_NO_OF_PDO)
    {
        /* if PDO index > 7, set the bit 31 and limit EPR obj_pos in 0...5 range */
        snkRdo.rdo_gen.eprPdo = true;
    }
    snkRdo.rdo_gen.objPos = (pdo_no & CY_PD_MAX_NO_OF_PDO);
#else
    snkRdo.rdo_gen.objPos = pdo_no;
#endif /* CY_PD_EPR_ENABLE */

    if(pdo_src->fixed_src.supplyType != CY_PDSTACK_PDO_AUGMENTED)
    {
        snkRdo.rdo_gen.giveBackFlag = false;
        if(pdo_src->fixed_src.supplyType == CY_PDSTACK_PDO_BATTERY)
        {
            uint16_t power = CY_PDUTILS_DIV_ROUND_UP(volt * cur, 500);
            snkRdo.rdo_gen.opPowerCur = power;
            snkRdo.rdo_gen.minMaxPowerCur = power;
        }
        else
        {
            snkRdo.rdo_gen.opPowerCur = cur;
            snkRdo.rdo_gen.minMaxPowerCur = cur;
        }
    }
    else
    {
        if((pdo_src->spr_avs_src.apdoType == CY_PDSTACK_APDO_SPR_AVS) ||
           (pdo_src->epr_avs_src.apdoType == CY_PDSTACK_APDO_AVS))
        {
            /* Convert voltage to 25 mV unit */
            snkRdo.rdo_spr_avs.outVolt = volt * 2u;
            /* Convert current to 50 mA unit */
            snkRdo.rdo_spr_avs.opCur = cur / 5u;
        }
        else if(pdo_src->pps_src.apdoType == CY_PDSTACK_APDO_PPS)
        {
            /* Convert voltage to 20 mV unit */
            snkRdo.rdo_pps.outVolt = (uint16_t)(volt * 25)/10;
            /* Convert current to 50 mA unit */
            snkRdo.rdo_pps.opCur = cur / 5u;
        }
    }

#if (CY_PD_REV3_ENABLE)
    /* Supports unchunked extended messages in PD 3.0 mode. */
    if (context->dpmConfig.specRevSopLive >= CY_PD_REV3)
    {
        snkRdo.rdo_gen.unchunkSup = true;
    }

#if (CY_PD_EPR_ENABLE)
    if (context->dpmConfig.specRevSopLive >= CY_PD_REV3)
    {
        snkRdo.rdo_gen.eprModeCapable = dpmExtStat->epr.snkEnable;
    }
#endif /* CY_PD_EPR_ENABLE */
#endif /* CY_PD_REV3_ENABLE */

    /* Prepare the DPM command buffer */
    cmd_buf.cmdSop = (cy_en_pd_sop_t)CY_PD_SOP;
    cmd_buf.noOfCmdDo = 1u;
    cmd_buf.cmdDo[0] = snkRdo;
#if (CY_PD_EPR_ENABLE)
    if(dpmExtStat->eprActive == true)
    {
        cmd_buf.noOfCmdDo = 2u;
        cmd_buf.cmdDo[1].val = pdo_src->val;

        status = Cy_PdStack_Dpm_SendPdCommand(context, CY_PDSTACK_DPM_CMD_SEND_EPR_REQUEST, &cmd_buf, false, NULL);
    }
    else
#endif /* (CY_PD_EPR_ENABLE) */
    {
        status = Cy_PdStack_Dpm_SendPdCommand(context, CY_PDSTACK_DPM_CMD_SEND_REQUEST, &cmd_buf, false, NULL);
    }

    return status;
}

/*******************************************************************************
* Function Name: snk_request_new_contract
********************************************************************************
* Summary:
*  Evaluates the user request and sends request message to the source
*
* Parameters:
*  context - PdStack context
*  supply_type - Supply type
*  volt - Voltage in mV
*  Cur - Current in mA
*
* Return:
* CY_PDSTACK_STAT_SUCCESS if the request is successful.
* CY_PDSTACK_STAT_FAILURE if the request is failed.
*
*******************************************************************************/
static cy_en_pdstack_status_t snk_request_new_contract(cy_stc_pdstack_context_t *context, en_supply_type_t supply_type, uint16_t volt, uint16_t cur)
{
    cy_en_pdstack_status_t status = CY_PDSTACK_STAT_FAILURE;
    uint8_t obj_pos = 0u;
    /* Convert voltage to 50mV units */
    volt = volt / 50u;
    /* Convert current to 10mA units */
    cur = cur / 10u;

    if(is_request_valid(context, volt, cur))
    {
        obj_pos = select_src_pdo(context, supply_type, volt, cur, context->dpmStat.srcCapP);
        if(obj_pos != 0u)
        {
            status = send_request(context, obj_pos, volt, cur, context->dpmStat.srcCapP);
        }
    }

    return status;
}

/*******************************************************************************
* Function Name: updatePPScontract
********************************************************************************
* Summary:
* Requesting PPS contract every 10 seconds 
*
* Parameters:
*  volt - Voltage in mV
*  Cur - Current in mA
*
* Return:
*  None
*
*******************************************************************************/
void updatePPScontract(int16_t volt, int16_t cur)
{
    bool newContractReq = false;
    cy_stc_pdstack_context_t *ptrPdStackContext = &gl_PdStackPort0Ctx;

    en_supply_type_t supply_type = PROGRAMMABLE_POWER_SUPPLY;

    /* Set new contract request flag */
    newContractReq = true;

    if(newContractReq == true)
    {
        newContractReq = false;
        /* Request for a new contract if the voltage is changed
         * or send same request again if PPS contract is in effect
         * PPS needs repetitive request every 10 seconds */
        if((volt != gl_cur_voltage)||(supply_type == PROGRAMMABLE_POWER_SUPPLY))
        {
            if(snk_request_new_contract(ptrPdStackContext, supply_type, volt, cur) == CY_PDSTACK_STAT_SUCCESS)
            {
                gl_cur_voltage = volt;
            }
        }
    }
    else
    {
        /* Do Nothing */
    }
}

/* [] END OF FILE */

