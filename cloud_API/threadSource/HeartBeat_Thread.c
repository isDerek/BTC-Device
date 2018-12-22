#include "globalParams.h"
#include "apiParams.h"
#include "api.h"
void heartBeat_thread(void *arg){
	LWIP_UNUSED_ARG(arg);	
	while(1){
			//·¢ÐÄÌø°ü
			if(ConnectAuthorizationFlag)
			{
				sprintf(socketInfo.outBuffer, API_Heartpack_Sendpack, API_SEND_Heartpack);			
				netconn_write(tcpsocket, socketInfo.outBuffer, strlen(socketInfo.outBuffer), 1);
				memset( socketInfo.outBuffer, 0, sizeof(socketInfo.outBuffer) );
			}
			vTaskDelay(30000);	
	}	
}
