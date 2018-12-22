#include "globalParams.h"
#include "apiParams.h"
#include "api.h"
#include "sensor.h"
#include "board.h"
#include "K64_api.h"
#include "fsl_i2c.h"
#include "flashLayout.h"
#include "tools.h"
#include "lib_crc16.h"
#include "md5Std.h"

volatile bool ConnectAuthorizationFlag = false;
SystemEventHandle eventHandle;
BTCInfo btcInfo;

void showUserDEF()
{
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
	}	
}
void switchSensorModule(int module, char *msgID)
{
	int updatedDutycycle;
	switch(module)
	{
		case API_module_pwm://pwm
			updatedDutycycle = btcInfo.configBuffer[0];
			FTM_updata(80);
			FTM_updata(updatedDutycycle);
			sprintf(socketInfo.outBuffer, API_SendData_Response, API_SEND_DATA, ERR_Success, msgID);				
			break;
		case API_module_rgb://rgb
			RGB_Show();			
			I2C_WriteByte(I2C0, 0X38, (0x00), 1);
			red = btcInfo.configBuffer[0];
			green = btcInfo.configBuffer[1];
			blue = btcInfo.configBuffer[2];
			RGB_Run(red, green, blue);
			sprintf(socketInfo.outBuffer, API_SendData_Response, API_SEND_DATA, ERR_Success, msgID);
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
			}
			sprintf(socketInfo.outBuffer, API_SendData_Response, API_SEND_DATA, ERR_Success, msgID);
			oledUserFlag = true;
			break;
//		case API_module_accel://G
//			ADXL345_Show();
//			ADXL345_Run(); 
//			sprintf(socketInfo.outBuffer, API_SendData_Response_ThreeData, API_SEND_DATA, ERR_Success, x, y, z, msgID);		
//			break;
//		case API_module_light://L
//			VEML6030_Show();
//			VEML6030_Run();								
//			sprintf(socketInfo.outBuffer, API_SendData_Response_IntData, API_SEND_DATA, ERR_Success, (int)(lx), msgID);			
//			break;
//		case API_module_uv://UV
//			VEML6075_Show();
//			VEML6075_Run();
//			sprintf(socketInfo.outBuffer, API_SendData_Response_TwoData, API_SEND_DATA, ERR_Success, UVA_data, UVB_data, msgID);			
//			break;							
//		case API_module_temphumi://H&T
//			HDC1050_Show();
//			HDC1050_Run();
//			sprintf(socketInfo.outBuffer, API_SendData_Response_TwoData, API_SEND_DATA, ERR_Success, (int)temp, (int)hum, msgID);			
//			break;
//		case API_module_pressure://P
//			SPL06001_Show();
//			SPL06001_Run();
//			sprintf(socketInfo.outBuffer, API_SendData_Response_FloatData, API_SEND_DATA, ERR_Success, (float)(persure/100), msgID);
//			break;
//		case API_module_ota://ota								
//			sprintf(socketInfo.outBuffer, API_SendData_Response, API_SEND_DATA, ERR_Success, msgID);									
//			break;
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
void test_OTA_Handle()
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

void switchMoudle()
{
	char	*cdata = (char*)VERSION_STR_ADDRESS;
	switch(btcInfo.module)
	{
		case 0:
			__disable_irq();
			btcInfo.deviceID = btcInfo.configBuffer[1];
			btcInfo.userID = btcInfo.configBuffer[2];
			memset(tempBuffer,0x0,VERSION_STR_LEN);
					/*   tempBuffer[0],tempBuffer[1] = appcodechecksum && tempBuffer[2],tempBuffer[3] = otacodechecksum */
			tempBuffer[0] = tempBuffer[2] = cdata[0];
      tempBuffer[1] = tempBuffer[3] = cdata[1];
			tempBuffer[4] = tempBuffer[7] = cdata[4];
			tempBuffer[5] = tempBuffer[8] = cdata[5];
			tempBuffer[6] = tempBuffer[9] =	cdata[6];
			tempBuffer[10] = '1'; // first boot flag
			tempBuffer[11] = btcInfo.configBuffer[0]; // register flag
			tempBuffer[12] = btcInfo.userID; // userID
			tempBuffer[13] = btcInfo.deviceID; // deviceID
			sprintf(tempBuffer+14,"%s",cdata+14);
      erase_sector(VERSION_STR_ADDRESS);
      program_flash(VERSION_STR_ADDRESS,(uint32_t *)tempBuffer,256);
			respCode = 100;
			btcInfo.apiId = 5;
			sprintf(socketInfo.outBuffer, API_SendData_Response, btcInfo.apiId,respCode,btcInfo.msgId);
			netconn_write(tcpsocket, socketInfo.outBuffer, strlen(socketInfo.outBuffer), 1);
			printf("deviceID = %d, userID = %d versionSN = %s\n\r",cdata[12],cdata[13],cdata+14);
			printf("register status Updata success! restet!!!\n\r");
			__enable_irq();
			NVIC_SystemReset();  
			break;
		case API_module_pwm://pwm
			FTM_updata(80);
			if(btcInfo.configBuffer[0] == 0)
			{
				FTM_updata(0);
			}
			else if(btcInfo.configBuffer[0] == 1)
			{
				FTM_updata(100);
			}
//			FTM_updata(btcInfo.configBuffer[0]);
			sprintf(socketInfo.outBuffer, API_SendData_Response, btcInfo.apiId, ERR_Success,btcInfo.msgId);
			netconn_write(tcpsocket, socketInfo.outBuffer, strlen(socketInfo.outBuffer), 1);
			memset(btcInfo.configBuffer,0,sizeof(btcInfo.configBuffer));		
			break;
		case API_module_rgb://rgb
			RGB_Show();			
			I2C_WriteByte(I2C0, 0X38, (0x00), 1);
			red = btcInfo.configBuffer[0];
			green = btcInfo.configBuffer[1];
			blue = btcInfo.configBuffer[2];
			RGB_Run(red, green, blue);
			sprintf(socketInfo.outBuffer, API_SendData_Response, btcInfo.apiId, ERR_Success, btcInfo.msgId);
			netconn_write(tcpsocket, socketInfo.outBuffer, strlen(socketInfo.outBuffer), 1);
			memset(btcInfo.configBuffer,0,sizeof(btcInfo.configBuffer));
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
			}
			sprintf(socketInfo.outBuffer, API_SendData_Response, btcInfo.apiId, ERR_Success, btcInfo.msgId);
			netconn_write(tcpsocket, socketInfo.outBuffer, strlen(socketInfo.outBuffer), 1);
			memset(btcInfo.configBuffer,0,sizeof(btcInfo.configBuffer));
			oledUserFlag = true;
			break;
		default:
			break;
	}
}
int i =1;
void apiHandle(int apiId)
{
	switch(apiId){
		case API_RES_AUTH:
			api_AUTH_Handle();
			break;
		case API_RES_Heartpack:
			api_HeartPack_Handle();
			break;
		case API_RES_SendData:
			break;
		case API_RES_OTA:
			api_OTA_Handle();
			break;
		case API_RES_SERVER_SEND:
			switchMoudle();		
			break;

//		case 24:
//			test_OTA_Handle();
//			break;
		default:
			break;
	}
	btcInfo.apiId = 0;
}
void notifyHandle(){
	if(sensorTxDataFlag == true && ConnectAuthorizationFlag)
	{
		btcInfo.apiId = 3;
		//			ADXL345_Run(); // 加速度传感器
		//		SPL06001_Run(); // 压力传感器
		VEML6030_Run(); // 光强传感器
		VEML6075_Run(); // 紫外线传感器
		HDC1050_Run(); // 温湿度传感器
		light_res = getADCValue();
//		printf("lux : %d  UVA = %d   UVB = %d  Temp= %d  Hum = %d light_res = %d \n\r",lx,UVA_data,UVA_data,(int)temp,(int)hum,light_res);
		sprintf(socketInfo.outBuffer, API_SenSorData_Sendpack, btcInfo.apiId, btcInfo.userID, btcInfo.deviceID,(int)temp,(int)hum,light_res,UVA_data,UVA_data,lx);
		netconn_write(tcpsocket, socketInfo.outBuffer, strlen(socketInfo.outBuffer), 1);
		printf("OutBUffer = %s\n\r",socketInfo.outBuffer);
		memset(btcInfo.configBuffer,0,sizeof(btcInfo.configBuffer));		
		sensorTxDataFlag = false;
		btcInfo.apiId = 0;
	}
}
void workHandle_thread(void *arg){
	LWIP_UNUSED_ARG(arg);	
	while(1){
			// api 处理
			apiHandle(btcInfo.apiId);
			notifyHandle();
			vTaskDelay(500);	
	}	
}
