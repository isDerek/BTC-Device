#include "globalParams.h"
#include "apiParams.h"
#include "api.h"
#include "sensor.h"
#include "board.h"
#include "K64_api.h"
#include "fsl_i2c.h"
#include "flashLayout.h"

volatile bool ConnectAuthorizationFlag = false;
SystemEventHandle eventHandle;

void switchSensorModule(int module, char *msgID)
{
	int updatedDutycycle;
	switch(module)
	{
		case API_module_pwm://pwm
			updatedDutycycle = btcInfo.configBuffer[0];
			FTM_updata(80);
			FTM_updata(updatedDutycycle);
			sprintf(socketInfo.outBuffer, API_SendData_Response, API_SendData, ERR_Success, msgID);				
			break;
		case API_module_rgb://rgb
			RGB_Show();			
			I2C_WriteByte(I2C0, 0X38, (0x00), 1);
			red = btcInfo.configBuffer[0];
			green = btcInfo.configBuffer[1];
			blue = btcInfo.configBuffer[2];
			RGB_Run(red, green, blue);
			sprintf(socketInfo.outBuffer, API_SendData_Response, API_SendData, ERR_Success, msgID);
			break;
		case API_module_oled://oled
			API_OLED_Clear();
			for(int j=0; j<4; j++)
			{
				if( j == 0)
				{
					OLED_ShowStr(0, j*2, (uint8_t*)btcInfo.oledOneLine);
				}
				else if(j == 1)
				{
					OLED_ShowStr(0, j*2, (uint8_t*)btcInfo.oledSecondLine);
				}
				else if(j == 2)
				{
					OLED_ShowStr(0, j*2, (uint8_t*)btcInfo.oledThirdLine);
				}
				else if(j == 3)
				{
					OLED_ShowStr(0, j*2, (uint8_t*)btcInfo.oledForthLine);
				}
//				printf("oledbuffer[%d] = %c \n\r",j,btcInfo.oledBuffer[j*2]);
//				OLED_ShowStr(0, j*2, (uint8_t*)(btcInfo.oledBuffer+j*2));
			}
			sprintf(socketInfo.outBuffer, API_SendData_Response, API_SendData, ERR_Success, msgID);				
			break;
		case API_module_accel://G
			ADXL345_Show();
			ADXL345_Run(); 
			sprintf(socketInfo.outBuffer, API_SendData_Response_ThreeData, API_SendData, ERR_Success, x, y, z, msgID);		
			break;
		case API_module_light://L
			VEML6030_Show();
			VEML6030_Run();								
			sprintf(socketInfo.outBuffer, API_SendData_Response_IntData, API_SendData, ERR_Success, (int)(lx), msgID);			
			break;
		case API_module_uv://UV
			VEML6075_Show();
			VEML6075_Run();
			sprintf(socketInfo.outBuffer, API_SendData_Response_TwoData, API_SendData, ERR_Success, UVA_data, UVB_data, msgID);			
			break;							
		case API_module_temphumi://H&T
			HDC1050_Show();
			HDC1050_Run();
			sprintf(socketInfo.outBuffer, API_SendData_Response_TwoData, API_SendData, ERR_Success, (int)temp, (int)hum, msgID);			
			break;
		case API_module_pressure://P
			SPL06001_Show();
			SPL06001_Run();
			sprintf(socketInfo.outBuffer, API_SendData_Response_FloatData, API_SendData, ERR_Success, (float)(persure/100), msgID);
			break;
		case API_module_ota://ota								
			sprintf(socketInfo.outBuffer, API_SendData_Response, API_SendData, ERR_Success, msgID);									
			break;
	}
	netconn_write(tcpsocket, socketInfo.outBuffer, strlen(socketInfo.outBuffer), 1);
	memset(btcInfo.configBuffer,0,sizeof(btcInfo.configBuffer));
	
}
void api_AUTH_Handle()
{
	if(respCode == ERR_Success)
	{	
		ConnectAuthorizationFlag = true;
		GPIO_WritePinOutput(BOARD_LED_GREEN_GPIO, BOARD_LED_GREEN_GPIO_PIN, 0);
		printf("Connection authorization response completed.\r\n");
	}
	else
	{	
		ConnectAuthorizationFlag = false;
		GPIO_WritePinOutput(BOARD_LED_GREEN_GPIO, BOARD_LED_GREEN_GPIO_PIN, 1);
		printf("Connection authorization response failed.\r\n");
	}	
}
void api_HeartPack_Handle()
{
	if(respCode == ERR_Success)
	{																
//		printf("Heartbeat packet response completed.\r\n");
	}		
}
void api_PushDevice_Handle()
{
	if(respCode == ERR_Success)
	{	
		printf("Device push response completed.\r\n");
	}
	else
	{	
		printf("Device push response failed.\r\n");
	}	
}
void api_OTA_Handle()
{
	char* cdata = (char*)VERSION_STR_ADDRESS;
	int otacodechecksum = (cdata[2]<<8)|cdata[3];
	printf("otacodechecksum = %d\n\r, otaInof.checkSum = %d \n\r",otacodechecksum,otaInfo.checkSum);
	if(otacodechecksum != otaInfo.checkSum)
	{	
		respCode = 100;
		eventHandle.getLatestFWFromServerFlag = true; 
	}
	else
	{
		respCode = 101;
		eventHandle.getLatestFWFromServerFlag = false; 
	}
	btcInfo.apiId = 24;
	sprintf(socketInfo.outBuffer,CMD_RESP_otaUpdate ,btcInfo.msgId,btcInfo.apiId,respCode);
	netconn_write(tcpsocket, socketInfo.outBuffer, strlen(socketInfo.outBuffer), 1);	
}
void apiHandle(int apiId)
{
	switch(apiId){
		case API_apiId_AUTH:
			api_AUTH_Handle();
			break;
		case API_apiId_SendData:
			switchSensorModule(btcInfo.module, btcInfo.msgId);
			break;
		case API_apiId_Heartpack:
			api_HeartPack_Handle();
			break;
		case API_apiId_PushDevice:
			api_PushDevice_Handle();
			break;
		case API_apiId_OTA:
			api_OTA_Handle();
			break;
		default:
			break;
	}
	btcInfo.apiId = 0;
}

void workHandle_thread(void *arg){
	LWIP_UNUSED_ARG(arg);	
	while(1){
			// api ´¦Àí
				apiHandle(btcInfo.apiId);


			vTaskDelay(500);	
	}	
}
