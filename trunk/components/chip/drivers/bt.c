/***********************************************************************//** 
 * \file  timer.c
 * \brief  csi timer driver
 * \copyright Copyright (C) 2015-2021 @ APTCHIP
 * <table>
 * <tr><th> Date  <th>Version  <th>Author  <th>Description
 * <tr><td> 2021-5-13 <td>V0.0 <td>ZJY     <td>initial
 * </table>
 * *********************************************************************
*/
#include <sys_clk.h>
#include <drv/bt.h>
#include <drv/irq.h>
#include <drv/tick.h>

/* Private macro------------------------------------------------------*/
/* externs function---------------------------------------------------*/
/* externs variablesr-------------------------------------------------*/
/* Private variablesr-------------------------------------------------*/

/** \brief initialize bt data structure
 * 
 *  \param[in] ptBtBase: pointer of bt register structure
 *  \param[in] wTimeOut: the timeout for bt, unit: us
 *  \return error code \ref csi_error_t
 */ 
csi_error_t csi_bt_timer_init(csp_bt_t *ptBtBase, uint32_t wTimeOut)
{
	uint32_t wTmLoad;
	uint32_t wClkDiv;
	
	csi_clk_enable((uint32_t *)ptBtBase);									//bt clk enable
	csp_bt_soft_rst(ptBtBase);												//reset bt
	
	wClkDiv = (soc_get_pclk_freq()/100000 * wTimeOut/600000);				//bt clk div value
	if(wClkDiv == 0)
		wClkDiv  = 1;
	wTmLoad = (soc_get_pclk_freq() / (wClkDiv * 20000)) * wTimeOut / 50;	//bt prdr load value
	
	csp_bt_set_cr(ptBtBase, (BT_IMMEDIATE << BT_SHDW_POS) | (BT_CONTINUOUS << BT_OPM_POS) |		//bt work mode
			(BT_PCLKDIV << BT_EXTCKM_POS) | (BT_CNTRLD_EN << BT_CNTRLD_POS) | BT_CLK_EN );
	csp_bt_set_pscr(ptBtBase, (uint16_t)wClkDiv - 1);						//bt clk div	
	csp_bt_set_prdr(ptBtBase, (uint16_t)wTmLoad);							//bt prdr load value
	csp_bt_set_int(ptBtBase, BT_PEND_INT, true);							//enable PEND interrupt
	csi_irq_enable((uint32_t *)ptBtBase);									//enable bt irq
	
    return CSI_OK;
}
/** \brief start bt
 * 
 *  \param[in] ptBtBase: pointer of bt register structure
 *  \return error code \ref csi_error_t
 */ 
csi_error_t csi_bt_start(csp_bt_t *ptBtBase)
{
	csp_bt_start(ptBtBase);
	return CSI_OK;
}
/** \brief stop bt
 * 
 *  \param[in] ptBtBase: pointer of bt register structure
 *  \return error code \ref csi_error_t
 */ 
csi_error_t csi_bt_stop(csp_bt_t *ptBtBase)
{
    csp_bt_stop(ptBtBase);
	return CSI_OK;
}
/** \brief get bt remaining value
 * 
 *  \param[in] ptBtBase: pointer of bt register structure
 *  \return none
 */ 
uint32_t csi_bt_get_remaining_value(csp_bt_t *ptBtBase)
{
    uint32_t wTimeVal = csp_bt_get_prdr(ptBtBase) - csp_bt_get_cnt(ptBtBase);

	return wTimeVal;
}
/** \brief get bt load value
 * 
 *  \param[in] ptBtBase: pointer of bt register structure
 *  \return none
 */ 
uint32_t csi_bt_get_load_value(csp_bt_t *ptBtBase)
{
	return csp_bt_get_prdr(ptBtBase);
}
/** \brief get bt load value
 * 
 *  \param[in] ptBtBase: pointer of bt register structure
 *  \return rue: running; false: stopped
 */ 
bool csi_bt_is_running(csp_bt_t *ptBtBase)
{
	CSI_PARAM_CHK(ptBtBase, false);
	 
	if(ptBtBase->RSSR & BT_START)
		return true;
	else 
		return false;
}

/** \brief bt pwm init 
 * 
 *  \param[in] ptBtBase: pointer of bt register structure
 *  \param[in] ptBtPwmCfg: pointer of bt pwm parameter config structure
 *  \return error code \ref csi_error_t
 */ 
csi_error_t csi_bt_pwm_init(csp_bt_t *ptBtBase, csi_bt_pwm_config_t *ptBtPwmCfg)
{
	uint32_t wClkDiv;
	uint32_t wCrVal;
	uint32_t wCmpLoad; 
	uint32_t wPrdrLoad; 
	
	if(ptBtPwmCfg->wFreq == 0 || ptBtPwmCfg->byDutyCycle  == 0 || ptBtPwmCfg->byDutyCycle == 100)
		return CSI_ERROR;
	
	csi_clk_enable((uint32_t *)ptBtBase);								//bt clk enable
	csp_bt_soft_rst(ptBtBase);											//reset bt
		
	wClkDiv = (soc_get_pclk_freq() / ptBtPwmCfg->wFreq / 60000);			//bt clk div value
	if(wClkDiv == 0)
		wClkDiv = 1;
	
	wPrdrLoad  = (soc_get_pclk_freq() / (wClkDiv * ptBtPwmCfg->wFreq));	//prdr load value
	wCmpLoad = wPrdrLoad * ptBtPwmCfg->byDutyCycle /100;					//cmp load value
		
	wCrVal = BT_CLK_EN | (BT_IMMEDIATE << BT_SHDW_POS) | (BT_CONTINUOUS << BT_OPM_POS) | (BT_PCLKDIV << BT_EXTCKM_POS) |
				(BT_CNTRLD_EN << BT_CNTRLD_POS) | (ptBtPwmCfg->byIdleLevel << BT_IDLEST_POS) | (ptBtPwmCfg->byIdleLevel << BT_STARTST_POS);
	csp_bt_set_cr(ptBtBase, wCrVal);									//set bt work mode
	csp_bt_set_pscr(ptBtBase, (uint16_t)wClkDiv - 1);					//bt clk div
	csp_bt_set_prdr(ptBtBase, (uint16_t)wPrdrLoad);						//bt prdr load value
	csp_bt_set_cmp(ptBtBase, (uint16_t)wCmpLoad);						//bt cmp load value
	
	if(ptBtPwmCfg->byInter)
	{
		csp_bt_set_int(ptBtBase, ptBtPwmCfg->byInter, true);				//enable interrupt
		csi_irq_enable((uint32_t *)ptBtBase);							//enable bt irq
	}
	
	return CSI_OK;
}
/** \brief  updata timer para and cmp reg value
 * 
 *  \param[in] ptBtBase: pointer of timer register structure
 *  \param[in] hwPrdr: timer padr reg  value
 *  \param[in] hwCmp: timer cmp reg value
 *  \return none
 */
csi_error_t csi_bt_prdr_cmp_updata(csp_bt_t *ptBtBase, uint16_t hwPrdr, uint16_t hwCmp) 
{
	csp_bt_set_prdr(ptBtBase, hwPrdr);						//bt prdr load value
	csp_bt_set_cmp(ptBtBase, hwCmp);						//bt cmp load value
	return CSI_OK;
}
/** \brief  updata timer pwm duty cycle
 * 
 *  \param[in] ptBtBase: pointer of timer register structure
 *  \param[in] byDutyCycle: duty cycle(0 -> 100)
 *  \return none
 */
csi_error_t csi_bt_pwm_duty_cycle_updata(csp_bt_t *ptBtBase, uint8_t byDutyCycle) 
{
	uint32_t wCmpLoad = csp_bt_get_prdr(ptBtBase) * byDutyCycle /100;
	
	csp_bt_set_cmp(ptBtBase, wCmpLoad);
	//csp_bt_updata_en(ptBtBase);
	return CSI_OK;
}

/** \brief  updata timer pwm freq and duty cycle
 * 
 *  \param[in] ptBtBase: pointer of timer register structure
 *  \param[in] wfreq: pwm frequency  
 *  \param[in] byDutyCycle: pwm duty cycle(0 -> 100)
 *  \return none
 */
csi_error_t csi_bt_pwm_updata(csp_bt_t *ptBtBase, uint32_t wfreq, uint8_t byDutyCycle) 
{
	uint16_t hwClkDiv = csp_bt_get_pscr(ptBtBase) + 1;
	uint32_t wPrdrLoad  = (soc_get_pclk_freq() / (hwClkDiv * wfreq));
	uint32_t wCmpLoad = wPrdrLoad * byDutyCycle /100;
	
	csp_bt_set_prdr(ptBtBase, wPrdrLoad);					//bt prdr load value
	csp_bt_set_cmp(ptBtBase, wCmpLoad);						//bt cmp load value
	
	return CSI_OK;
}

/** \brief timer sync input evtrg config  
 * 
 *  \param[in] ptBtBase: pointer of timer register structure
 *  \param[in] eTrgin: timer sync evtrg input channel(0~1)
 *  \param[in] eTrgMode: timer evtrg mode, continuous/once
 *  \param[in] bAutoRearm: auto rearm, ENABLE/DISABLE(true/false)
 *  \return error code \ref csi_error_t
 */
csi_error_t csi_bt_set_sync(csp_bt_t *ptBtBase,csi_bt_trgin_e eTrgin, csi_bt_trgmode_e eTrgMode, bool bAutoRearm)
{
	bt_sync_e eSync = BT_SYNC_EN;
	
	if(eTrgin == BT_TRGIN_SYNCEN0)
	{
		eTrgMode = eTrgMode << 1;
		eSync = eSync << 1;
	}
	ptBtBase->CR = (ptBtBase->CR & 0xffff30ff) | (eSync << BT_SYNC_POS) | (BT_SYNCMD_DIS << BT_SYNCCMD_POS) | 
				(eTrgMode << BT_OSTMD_POS) | (bAutoRearm << BT_AREARM_POS);
	
	return CSI_OK;
}
/** \brief rearm timer sync evtrg  
 * 
 *  \param[in] ptBtBase: pointer of timer register structure
 *  \param[in] eTrgin: timer sync evtrg input channel(0~1)
 *  \return error code \ref csi_error_t
 */
csi_error_t csi_bt_rearm_sync(csp_bt_t *ptBtBase,csi_bt_trgin_e eTrgin)
{
	csp_bt_rearm_sync(ptBtBase, eTrgin);
	
	return CSI_OK;
}
/** \brief timer evtrg output config
 * 
 *  \param[in] ptBtBase:pointer of timer register structure
 *  \param[in] byEvtrg: trigger event (byEvtrg =  0)
 *  \param[in] eTrgSrc: BT evtrg source(1~3) 
 *  \param[in] eTrgPrd: event count period
 *  \return error code \ref csi_error_t
 */
csi_error_t csi_bt_set_evtrg(csp_bt_t *ptBtBase, uint8_t byEvtrg, csi_bt_trgsrc_e eTrgSrc, uint8_t eTrgPrd)
{
	if (byEvtrg > 0)
		return CSI_ERROR;
	
	if(eTrgSrc == BT_TRGSRC_DIS)
		csp_bt_set_evtrg(ptBtBase,0x00);
	else
		csp_bt_set_evtrg(ptBtBase,(eTrgSrc | (BT_TRGOE_EN << BT_TRGOE_POS)));
		
	return CSI_OK;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
/** \brief start timer by sync event
 * 
 *  \param[in] ptBtBase: handle timer handle to operate
 *  \param[in] wTimeOut: tthe timeout for timer, unit: us
 *  \return error code \ref csi_error_t
 */ 
csi_error_t csi_bt_start_sync(csp_bt_t *ptBtBase, uint32_t wTimeOut)
{
	uint32_t wTmLoad;
	uint32_t wClkDiv;
	
	csi_clk_enable((uint32_t *)ptBtBase);									//bt clk enable
	csp_bt_soft_rst(ptBtBase);												//reset bt
	
	wClkDiv = (soc_get_pclk_freq()/100000 * wTimeOut/600000);				//bt clk div
	if(wClkDiv == 0)
		wClkDiv  = 1;
	wTmLoad = (soc_get_pclk_freq() / (wClkDiv * 20000)) * wTimeOut / 50;	//bt prdr load value
	
	csp_bt_set_cr(ptBtBase, (BT_IMMEDIATE << BT_SHDW_POS) | (BT_CONTINUOUS << BT_OPM_POS) |		//bt work mode
			(BT_PCLKDIV << BT_EXTCKM_POS) | (BT_CNTRLD_EN << BT_CNTRLD_POS) | BT_CLK_EN );
	csp_bt_set_pscr(ptBtBase, (uint16_t)wClkDiv - 1);						//bt clk div	
	csp_bt_set_prdr(ptBtBase, (uint16_t)wTmLoad);							//bt prdr load value
	csp_bt_set_int(ptBtBase, BT_PEND_INT, true);							//enable PEND interrupt
	csi_irq_enable((uint32_t *)ptBtBase);									//enable bt irq
	
	return CSI_OK;
}