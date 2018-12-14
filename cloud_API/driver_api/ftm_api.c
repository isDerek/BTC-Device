#include "fsl_ftm.h"
#include "K64_api.h"
#include "stdio.h"
#include "tools.h"
/* PWM */
/* The Flextimer instance/channel used for board */
#define BOARD_FTM_BASEADDR FTM0
#define BOARD_FTM_CHANNEL kFTM_Chnl_3
/* Interrupt number and interrupt handler for the FTM instance used */
#define FTM_INTERRUPT_NUMBER FTM0_IRQn
#define FTM_HANDLER FTM0_IRQHandler
/* Interrupt to enable and flag to read; depends on the FTM channel used */
#define FTM_CHANNEL_INTERRUPT_ENABLE kFTM_Chnl3InterruptEnable
#define FTM_CHANNEL_FLAG kFTM_Chnl3Flag
/* Get source clock for FTM driver */
#define FTM_SOURCE_CLOCK CLOCK_GetFreq(kCLOCK_BusClk)


volatile bool ftmIsrFlag = false;
volatile uint8_t updatedDutycycle = 10U;  
ftm_config_t ftmInfo;
ftm_chnl_pwm_signal_param_t ftmParam;
ftm_pwm_level_select_t pwmLevel = kFTM_HighTrue;



void FTM_updata(int updatedDutycycle)
{	
	/* Use interrupt to update the PWM dutycycle */
	if (true == ftmIsrFlag)
	{
		printf("updatedDutycycle = %d\r\n",updatedDutycycle);
		/* Disable interrupt to retain current dutycycle for a few seconds */
		FTM_DisableInterrupts(BOARD_FTM_BASEADDR, FTM_CHANNEL_INTERRUPT_ENABLE);
		ftmIsrFlag = false;
		/* Disable channel output before updating the dutycycle */
		FTM_UpdateChnlEdgeLevelSelect(BOARD_FTM_BASEADDR, BOARD_FTM_CHANNEL, 0U);
		/* Update PWM duty cycle */
		FTM_UpdatePwmDutycycle(BOARD_FTM_BASEADDR, BOARD_FTM_CHANNEL, kFTM_CenterAlignedPwm, updatedDutycycle);			
		/* Software trigger to update registers */
		FTM_SetSoftwareTrigger(BOARD_FTM_BASEADDR, true);
		/* Start channel output with updated dutycycle */
		FTM_UpdateChnlEdgeLevelSelect(BOARD_FTM_BASEADDR, BOARD_FTM_CHANNEL, pwmLevel);
		/* Delay to view the updated PWM dutycycle */
		delay_30ms();
		/* Enable interrupt flag to update PWM dutycycle */
		FTM_EnableInterrupts(BOARD_FTM_BASEADDR, FTM_CHANNEL_INTERRUPT_ENABLE);
	}
}

void FTM_HANDLER(void)
{
    ftmIsrFlag = true;
//		printf("updatedDutycycle = %d\r\n",updatedDutycycle);
    if ((FTM_GetStatusFlags(BOARD_FTM_BASEADDR) & FTM_CHANNEL_FLAG) == FTM_CHANNEL_FLAG)
    {
        /* Clear interrupt flag.*/
        FTM_ClearStatusFlags(BOARD_FTM_BASEADDR, FTM_CHANNEL_FLAG);
    }
}

void ftm_init(void)
{
	/* Configure ftm params with frequency 24kHZ */
	ftmParam.chnlNumber = BOARD_FTM_CHANNEL;
	ftmParam.level = pwmLevel;
	ftmParam.dutyCyclePercent = updatedDutycycle;
	ftmParam.firstEdgeDelayPercent = 0U;
	FTM_GetDefaultConfig(&ftmInfo);
	/* Initialize FTM module */
	FTM_Init(BOARD_FTM_BASEADDR, &ftmInfo);
	FTM_SetupPwm(BOARD_FTM_BASEADDR, &ftmParam, 1U, kFTM_CenterAlignedPwm, 17000U, FTM_SOURCE_CLOCK);
	/* Enable channel interrupt flag.*/
	FTM_EnableInterrupts(BOARD_FTM_BASEADDR, FTM_CHANNEL_INTERRUPT_ENABLE);
	/* Enable at the NVIC */
	EnableIRQ(FTM_INTERRUPT_NUMBER);
	FTM_StartTimer(BOARD_FTM_BASEADDR, kFTM_SystemClock);		
}

