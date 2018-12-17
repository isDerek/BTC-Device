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

int main(void)
{
    BOARD_InitPins();
    BOARD_BootClockRUN();
    BOARD_InitDebugConsole();
		i2c_init();	
		sensorInit();		
		Network_Init();
		gpio_init();
		ftm_init();
		adc_init();
	  flash_init();
		OTAInit();
//	flash_init();
//	erase_sector(0x30000);
//	uint32_t a[4]={1,2,3,4};
//	program_flash(0x30000, a, sizeof(a));
//	while(1){
//		getADCValue();
//	}
		if(sys_thread_new("socket_connect", connect_thread, NULL, INIT_THREAD_STACKSIZE, INIT_THREAD_PRIO) == NULL)
					LWIP_ASSERT("socket_init(): Task creation failed.", 0);
		if(sys_thread_new("heartBeatTheard", heartBeat_thread, NULL, INIT_THREAD_STACKSIZE, INIT_THREAD_PRIO) == NULL)
					LWIP_ASSERT("heartBeatTheard(): Task creation failed.", 0);
		if(sys_thread_new("workHandleTheard", workHandle_thread, NULL, INIT_THREAD_STACKSIZE, INIT_THREAD_PRIO) == NULL)
					LWIP_ASSERT("workHandleTheard(): Task creation failed.", 0);			
		/* Will not get here unless a task calls vTaskEndScheduler ()*/
  	vTaskStartScheduler();	 		
}

