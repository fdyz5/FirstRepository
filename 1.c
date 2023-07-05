#include "app.h"

__IO uint32_t LsiFreq = 0;
__IO uint32_t CaptureNumber = 0, PeriodValue = 0;
uint32_t IC1ReadValue1 = 0, IC1ReadValue2 =0;

/**
  * @brief  Configures TIM10 to measure the LSI oscillator frequency. 
  * @param  None
  * @retval LSI Frequency
  */
uint32_t GetLSIFrequency(void)
{
  NVIC_InitTypeDef   NVIC_InitStructure;
  TIM_ICInitTypeDef  TIM_ICInitStructure;
  RCC_ClocksTypeDef  RCC_ClockFreq;

  /* Enable the LSI oscillator ************************************************/
  RCC_LSICmd(ENABLE);
  
  /* Wait till LSI is ready */
  while (RCC_GetFlagStatus(RCC_FLAG_LSIRDY) == RESET)
  {}

  /* TIM10 configuration *******************************************************/ 
  /* Enable TIM10 clock */
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_TIM10, ENABLE);
  
  /* Reset TIM10 registers */
  TIM_DeInit(TIM10);

  /* Configure TIM10 prescaler */
  TIM_PrescalerConfig(TIM10, 0, TIM_PSCReloadMode_Immediate);

  /* Connect LSI clock to TIM10 Input Capture 1 */
  TIM_RemapConfig(TIM10, TIM10_LSI);

  /* TIM10 configuration: Input Capture mode ---------------------
     The reference clock(LSE or external) is connected to TIM10 CH1
     The Rising edge is used as active edge,
     The TIM10 CCR1 is used to compute the frequency value 
  ------------------------------------------------------------ */
  TIM_ICInitStructure.TIM_Channel     = TIM_Channel_1;
  TIM_ICInitStructure.TIM_ICPolarity  = TIM_ICPolarity_Rising;
  TIM_ICInitStructure.TIM_ICSelection = TIM_ICSelection_DirectTI;
  TIM_ICInitStructure.TIM_ICPrescaler = TIM_ICPSC_DIV8;
  TIM_ICInitStructure.TIM_ICFilter = 0x0;
  TIM_ICInit(TIM10, &TIM_ICInitStructure);

  /* Enable the TIM10 global Interrupt */
  NVIC_InitStructure.NVIC_IRQChannel = TIM10_IRQn;
  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
  NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
  NVIC_Init(&NVIC_InitStructure);

  /* Enable TIM10 counter */
  TIM_Cmd(TIM10, ENABLE);

  /* Reset the flags */
  TIM10->SR = 0;
    
  /* Enable the CC4 Interrupt Request */  
  TIM_ITConfig(TIM10, TIM_IT_CC1, ENABLE);


  /* Wait until the TIM10 get 2 LSI edges (refer to TIM10_IRQHandler() in 
    stm32l1xx_it.c file) ******************************************************/
  while(CaptureNumber != 2)
  {
  }
  /* Deinitialize the TIM10 peripheral registers to their default reset values */
  TIM_DeInit(TIM10);


  /* Compute the LSI frequency, depending on TIM10 input clock frequency (PCLK1)*/
  /* Get SYSCLK, HCLK and PCLKx frequency */
  RCC_GetClocksFreq(&RCC_ClockFreq);

  /* Get PCLK1 prescaler */
  if ((RCC->CFGR & RCC_CFGR_PPRE1) == 0)
  { 
    /* PCLK1 prescaler equal to 1 => TIMCLK = PCLK1 */
    return ((RCC_ClockFreq.PCLK1_Frequency / PeriodValue) * 8);
  }
  else
  { /* PCLK1 prescaler different from 1 => TIMCLK = 2 * PCLK1 */
    return (((2 * RCC_ClockFreq.PCLK1_Frequency) / PeriodValue) * 8) ;
  }
}

/**
  * @brief  This function handles TIM10 global interrupt request.
  * @param  None
  * @retval None
  */
void TIM10_IRQHandler(void)
{
  if (TIM_GetITStatus(TIM10, TIM_IT_CC1) != RESET)
  {
    /* Clear TIM10 Capture Compare 1 interrupt pending bit */
    TIM_ClearITPendingBit(TIM10, TIM_IT_CC1);
    
    if(CaptureNumber == 0)
    {
      /* Get the Input Capture value */
      IC1ReadValue1 = TIM_GetCapture1(TIM10);
      CaptureNumber = 1;
    }
    else if(CaptureNumber == 1)
    {
       /* Get the Input Capture value */
       IC1ReadValue2 = TIM_GetCapture1(TIM10); 
       TIM_ITConfig(TIM10, TIM_IT_CC1, DISABLE);

       /* Capture computation */
       if (IC1ReadValue2 > IC1ReadValue1)
       {
         PeriodValue = (IC1ReadValue2 - IC1ReadValue1);
       }
       else
       {
         PeriodValue = ((0xFFFF - IC1ReadValue1) + IC1ReadValue2);
       }
       /* capture of two values is done */
       CaptureNumber = 2;
    }
  }
}

extern void bsp_idle(void);
int main(void)
{ 	
	uint8_t step = 0;
	
	//¸÷¸öÇý¶¯³õÊ¼»¯
	__set_PRIMASK(1);
	bsp_init();
	__set_PRIMASK(0);
	
//	printf("\r\nRCC->CSR: %08X\r\n",RCC->CSR);
//	RCC->CSR |= RCC_CSR_RMVF;	
	
	/* Get the LSI frequency:  TIM10 is used to measure the LSI frequency */
	LsiFreq = GetLSIFrequency();
	printf("\r\nLsiFreq: %u\r\n",LsiFreq);	
	
	{
	RTC_InitTypeDef   RTC_InitStructure;		
	/* Calendar Configuration */
	RTC_InitStructure.RTC_AsynchPrediv = 0x7F;
	RTC_InitStructure.RTC_SynchPrediv	=  (LsiFreq/128) - 1;
	RTC_InitStructure.RTC_HourFormat = RTC_HourFormat_24;
	RTC_Init(&RTC_InitStructure);
	}
//	bsp_power_ble_on();	
//	while(AT_START() == -1);
//	while(1)
//	{
//		uint8_t ch;
//		
//		if(comGetChar(COM_DEBUG, &ch))
//		{
//			comSendChar(COM_RPT, ch);
//		}

//		if(comGetChar(COM_RPT, &ch))
//		{
//			comSendChar(COM_DEBUG, ch);
//		}

//		bsp_led_on(1);
//		bsp_iwdg_clear();		
//	}
	printf("\r\n");
	printf("wait 3s for <xmyns>");
	printf("\r\n");	
	while(1)
	{
		if(bsp_timer_check(TMR_RUN))
		{
			printf("\r\ngo to app mode\r\n");	
			printf("\r\n%s\r\n",cpSTVS);			
			break;
		}		
		else if(comGetRxTimeOut(COM_CFG))
		{
			char cmd[128],*p;	
			
			if(comGetBytes(COM_CFG, (uint8_t *)cmd, sizeof(cmd)))
			{
				p = strstr((char *)cmd,"xmyns");
				if(p != NULL)
				{
					g_ucCfgMode = 1; 
					printf("\r\ngo to config mode\r\n");						
					break;
				}
			}			
		}
	}
	

	while(1)
	{
		int ret = 0;
		char cmd[128],*p;	
		
		switch(step)
		{
		case 0:
			bsp_power_ble_on();	
			bsp_timer_star_once(TMR_RUN, TM_1S);	
			step++;
			break;				
		case 1:
			if(bsp_timer_check(TMR_RUN))
			{
				memset(cmd,0,sizeof(cmd));
				sprintf(cmd,"+++");	
				comSendBuf(COM_CFG, (uint8_t *)cmd, strlen(cmd));
				bsp_timer_star_once(TMR_RUN, TM_500MS);	
				step++;
			}			
			break;	
			
		case 2:	
		case 4:	
			memset(cmd,0,sizeof(cmd));
			sprintf(cmd,"AT+NAME?\r\n");
			comSendBuf(COM_CFG, (uint8_t *)cmd, strlen(cmd));		
			bsp_timer_star_once(TMR_RUN, TM_500MS);	
			step++;			
			break;	

		case 3:
		case 5:			
			if(bsp_timer_check(TMR_RUN))
			{
				step++;
			}			
			else if(comGetRxTimeOut(COM_CFG))
			{
				if(comGetBytes(COM_CFG, (uint8_t *)cmd, sizeof(cmd)))
				p = strstr((char *)cmd,"+NAME: ");
				if(p != NULL)
				{
					char name[64] = {0};
					
					p += 7;
					bcd_to_str(g_tVar.ucaDTUSN,name,5);
					if(strncmp(p+7,name,10))
					{
						memset(cmd,0,sizeof(cmd));
						sprintf(cmd,"AT+NAME=xmsl_%s\r\n",name);
						comSendBuf(COM_CFG, (uint8_t *)cmd, strlen(cmd));		
						bsp_timer_star_once(TMR_RUN, TM_500MS);							
						step=9;						
					}
					else
					{
						step++;
					}
					
				}

			}		
			break;
			
		case 9:			
			if(bsp_timer_check(TMR_RUN))
			{
				step++;
			}	
			break;
			
		default:
			ret = 1;
			bsp_power_ble_off();		
			break;
		}
	
		bsp_iwdg_clear();
		if(ret) break;
	}
	
g_tVar.usTPITV = 600; //²ÉÑù¼ä¸ô 
g_tVar.usSPITV = 30; //ÉÏ±¨¼ä¸ô
//g_tVar.ucPSMD = 3;	
	
	bsp_timer_star_auto(TMR_RUN, TM_1S);
	while(1)
	{
		#ifdef __RS485__
		g_tVar.ucRTUPLS = 0;
		#else
		g_tVar.ucRTUPLS = 1;
		//g_tVar.ucPSMD = 3;		
		#endif
		
		if(g_ucUpdata == 0)
		{
			aqt_process();		
			rpt_process();	
		}			
		cfg_process();
//		lcd_process();			
		bsp_idle();			
	}
}
