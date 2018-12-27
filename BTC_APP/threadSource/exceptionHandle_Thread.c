#include "globalParams.h"

ExpEvent expEvent;
void exceptionHandle_thread(void *arg){
	expEvent.connectErrorFlag = false;
	LWIP_UNUSED_ARG(arg);	
	while(1){
			if(expEvent.connectErrorFlag)
			{
				NVIC_SystemReset();  
			}
			vTaskDelay(1000);	
	}	
}
