#include "globalParams.h"

void exceptionHandle_thread(void *arg){
	LWIP_UNUSED_ARG(arg);	
	while(1){
			
			vTaskDelay(1000);	
	}	
}
