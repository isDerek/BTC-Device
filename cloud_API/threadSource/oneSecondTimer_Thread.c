#include "api.h"
#include "globalParams.h"
#include "sensor.h"

bool oledUserFlag = false;
bool oledSwitchFlag = false;
int oledUserTimer = 0;

void oneSecondTimer_thread(void *arg){
	LWIP_UNUSED_ARG(arg);	
	while(1){
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
			vTaskDelay(1000);	
	}	
}