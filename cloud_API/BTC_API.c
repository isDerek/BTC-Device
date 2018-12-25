/*******************************************************************************
 * Includes
 ******************************************************************************/
#include "K64_api.h"
#include "thread_api.h"
#include "sensor.h"
#include "api.h"
#include "board.h"
#include "fsl_debug_console.h"
#include "fsl_common.h"
#include "pin_mux.h"
#include "clock_config.h"
#include "userConfig.h"

#include "tools.h"
#include "stdlib.h"
#include "globalParams.h"
#include "apiParams.h"
#include "flashLayout.h"
/*******************************************************************************
 * Definitions
 ******************************************************************************/

/*! @brief Stack size of the temporary lwIP initialization thread. */
#define INIT_THREAD_STACKSIZE 1024
/*! @brief Priority of the temporary lwIP initialization thread. */
#define INIT_THREAD_PRIO DEFAULT_THREAD_PRIO

/*******************************************************************************
* Prototypes
******************************************************************************/

/*******************************************************************************
* Variables
******************************************************************************/

/*******************************************************************************
 * Code
 ******************************************************************************/
void device_init()
{
	btcInfo.deviceRegister = 0;
	btcInfo.module = -1;
	btcInfo.deviceID = 0;
	btcInfo.userID = 0;
	
}
int main(void)
{
		char a[1024];
    BOARD_InitPins();
    BOARD_BootClockRUN();
    BOARD_InitDebugConsole();
		i2c_init();	
		Network_Init();
		sensorInit();	
		gpio_init();
		ftm_init();
		adc_init();
	  flash_init();
		OTAInit();	
		device_init();
//		__disable_irq();
//		for(int i =0; i< 255; i++)
//		{
//			a[i] = i;
//		}
//		
//		erase_DefSize(0x47c00,1024);
//		program_flash(0x47c00,(uint32_t *)a,1024);
//		__enable_irq();
//	while(1)
//	{
//		
//	}
		printf("OTA Version 5.0!\n\r");
		if(sys_thread_new("socket_connect", connect_thread, NULL, INIT_THREAD_STACKSIZE, INIT_THREAD_PRIO) == NULL)
					LWIP_ASSERT("socket_init(): Task creation failed.", 0);
		if(sys_thread_new("heartBeatTheard", heartBeat_thread, NULL, INIT_THREAD_STACKSIZE, INIT_THREAD_PRIO) == NULL)
					LWIP_ASSERT("heartBeatTheard(): Task creation failed.", 0);
		if(sys_thread_new("workHandleTheard", workHandle_thread, NULL, INIT_THREAD_STACKSIZE, INIT_THREAD_PRIO) == NULL)
					LWIP_ASSERT("workHandleTheard(): Task creation failed.", 0);
		if(sys_thread_new("oneSecondTimerTheard", oneSecondTimer_thread, NULL, INIT_THREAD_STACKSIZE, INIT_THREAD_PRIO) == NULL)
					LWIP_ASSERT("oneSecondTimerTheard(): Task creation failed.", 0);				
		/* Will not get here unless a task calls vTaskEndScheduler ()*/
  	vTaskStartScheduler();	 		
}

