#include "api.h"
#include "globalParams.h"
#include "sensor.h"

bool oledUserFlag = false;
bool oledSwitchFlag = false;
bool sensorTxDataFlag = false;
int oledUserTimer = 0;
int sensorPeriodTmr = 55;

void oneSecondTimer_thread(void *arg){
	LWIP_UNUSED_ARG(arg);	
	while(1){
		if(ConnectAuthorizationFlag)
		{
			sensorPeriodTmr ++;
//			printf("sensorTmr = %d\n\r",sensorPeriodTmr);
			if(sensorPeriodTmr % 60 == 0)
			{
				sensorTxDataFlag = true;
				sensorPeriodTmr = 0;
			}
			if(oledUserFlag == true)
			{
				oledUserTimer++;
				if(oledUserTimer % 5 == 0)
				{
					oledUserTimer = 0;
					if(oledSwitchFlag == false)
					{
						OLED_Welcome();
						oledSwitchFlag = true;
					}
					else
					{
						showUserDEF();
						oledSwitchFlag = false;
					}
				}
			}			
		}
		vTaskDelay(1000);	
	}	
}
