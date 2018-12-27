#include "api.h"
#include "globalParams.h"
#include "sensor.h"

bool oledUserFlag = false;	// 自定义 OLED 功能是否被使用标志位
bool oledSwitchFlag = false;	// 自定义 OLED 与用户界面切换标志位
bool sensorTxDataFlag = false; // 发送 Sensor 数据标志位
bool startSWTmr = false; // 开始计算按键展示时间
int oledUserTimer = 0;		// 展示用户界面时间
int sensorPeriodTmr = 55; // 第一次连接上服务器 5 S 后发送传感器消息
int connectTmr = 0; // 连接服务器计时，有响应清空，无响应 +1

// 按键切换计时事件
void keyHandle_OLED()
{
	if(startSWTmr == true)
	{
		swTimer ++;
	}
	if(swTimer == 3)
	{
		startSWTmr = false;
		swTimer = 0;
		OLED_Welcome();	
	}				
}
// OLED 自定义与用户界面切换事件
void userDefChange_OLED()
{
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
// 发送传感器数据计时事件
void sendSensorDataTmr()
{
	sensorPeriodTmr ++;
//	printf("sensorTmr = %d\n\r",sensorPeriodTmr);
	if(sensorPeriodTmr % 60 == 0)
	{
		sensorTxDataFlag = true;
		sensorPeriodTmr = 0;
	}	
}
// 连接异常计时事件
void connectTmr_Exp()
{			
	connectTmr++;
	if(connectTmr % 60 == 0)
	{
		expEvent.connectErrorFlag = true;
	}	
}

void oneSecondTimer_thread(void *arg){
	LWIP_UNUSED_ARG(arg);	
	while(1){
		if(ConnectAuthorizationFlag)
		{
			keyHandle_OLED();
			sendSensorDataTmr();
			userDefChange_OLED();
			connectTmr_Exp();
		}
		vTaskDelay(1000);	
	}	
}
