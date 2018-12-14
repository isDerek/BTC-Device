#include "globalParams.h"
#include "api.h"
#include "apiParams.h"
#include "cJSON.h"
#include "sensor.h"
#include "board.h"
#include "K64_api.h"
#include "fsl_i2c.h"

u16_t port = 55551;
struct netconn *tcpsocket;
struct netbuf  *TCPNetbuf;
SocketInfo socketInfo;

volatile bool showFlag = false;
bool server_connect_Flag = false;
//SocketInfo socketInfo;
float x, y, z;//加速度
uint16_t UVA_data, UVB_data;
float lx;//光亮等级
uint32_t persure;
float temp, hum;
int red, green, blue;


volatile bool ConnectAuthorizationFlag = false;

void switchSensorModule(int module, char *msgId, cJSON* json_config)
{
	int led;
	char *out;
	cJSON *json_config_object;
	int updatedDutycycle;
	switch(module)
	{
		case API_module_port://port
			led = (cJSON_GetArrayItem(json_config,0)->valueint)^1;
			GPIO_WritePinOutput(BOARD_LED_RED_GPIO, BOARD_LED_RED_GPIO_PIN, led);
			sprintf(socketInfo.outBuffer, API_SendData_Response, API_SendData, ERR_Success, msgId);			
			break;
		case API_module_pwm://pwm
			updatedDutycycle = cJSON_GetArrayItem(json_config,0)->valueint;
			FTM_updata(80);
			FTM_updata(updatedDutycycle);
			sprintf(socketInfo.outBuffer, API_SendData_Response, API_SendData, ERR_Success, msgId);				
			break;
		case API_module_rgb://rgb
			RGB_Show();			
			I2C_WriteByte(I2C0, 0X38, (0x00), 1);
			red = cJSON_GetArrayItem(json_config,0)->valueint;
			green = cJSON_GetArrayItem(json_config,1)->valueint;
			blue = cJSON_GetArrayItem(json_config,2)->valueint;
			RGB_Run(red, green, blue);
			sprintf(socketInfo.outBuffer, API_SendData_Response, API_SendData, ERR_Success, msgId);				
			break;
		case API_module_oled://oled
			API_OLED_Clear();
			for(int j=0; j<4; j++)
			{
				json_config_object = cJSON_GetArrayItem(json_config,j);
				out = json_config_object->valuestring;																		
				OLED_ShowStr(0, j*2, (uint8_t*)out);
			}
			sprintf(socketInfo.outBuffer, API_SendData_Response, API_SendData, ERR_Success, msgId);				
			break;
		case API_module_accel://G
			ADXL345_Show();
			ADXL345_Run(); 
			sprintf(socketInfo.outBuffer, API_SendData_Response_ThreeData, API_SendData, ERR_Success, x, y, z, msgId);		
			break;
		case API_module_light://L
			VEML6030_Show();
			VEML6030_Run();								
			sprintf(socketInfo.outBuffer, API_SendData_Response_IntData, API_SendData, ERR_Success, (int)(lx), msgId);			
			break;
		case API_module_uv://UV
			VEML6075_Show();
			VEML6075_Run();
			sprintf(socketInfo.outBuffer, API_SendData_Response_TwoData, API_SendData, ERR_Success, UVA_data, UVB_data, msgId);			
			break;							
		case API_module_temphumi://H&T
			HDC1050_Show();
			HDC1050_Run();
			sprintf(socketInfo.outBuffer, API_SendData_Response_TwoData, API_SendData, ERR_Success, (int)temp, (int)hum, msgId);			
			break;
		case API_module_pressure://P
			SPL06001_Show();
			SPL06001_Run();
			sprintf(socketInfo.outBuffer, API_SendData_Response_FloatData, API_SendData, ERR_Success, (float)(persure/100), msgId);
			break;
		case API_module_ota://ota								
			sprintf(socketInfo.outBuffer, API_SendData_Response, API_SendData, ERR_Success, msgId);									
			break;
	}
	netconn_write(tcpsocket, socketInfo.outBuffer, strlen(socketInfo.outBuffer), 1);
}
	/* Parse text to JSON, then render back to text, and print! */
void parseRecvMsgInfo(char *text)
{
	int apiId;
	char *msgId;
	int module;
	int respCode;
  cJSON *json, *json_data,*json_config;	
	json=cJSON_Parse(text);
    if (!json)
		{
      printf("not json string, start to parse another way!\r\n");
    } 
		else 
		{
			apiId = cJSON_GetObjectItem( json , "apiId")->valueint;
//			printf("apiId is =%d\r\n",apiId);				

			msgId = cJSON_GetObjectItem( json , "msgId")->valuestring;
//			printf("msgId is =%s\r\n",msgId);				
					
			respCode = cJSON_GetObjectItem( json , "respCode")->valueint;
//			printf("respCode is =%d\r\n",respCode);
						
			json_data = cJSON_GetObjectItem( json , "data");
			if(json_data)
			{
				module = cJSON_GetObjectItem( json_data , "module")->valueint; 
				printf("module is =%d\r\n",module);
				json_config = cJSON_GetObjectItem( json_data , "config");
			}
			if(apiId == API_apiId_AUTH){
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
			else if (apiId == API_apiId_SendData){
				switchSensorModule(module, msgId, json_config);
			}
			else if (apiId ==  API_apiId_Heartpack){
				if(respCode == ERR_Success)
				{																
					printf("Heartbeat packet response completed.\r\n");
				}					
			}
			else if (apiId == API_apiId_PushDevice){
				if(respCode == ERR_Success)
				{	
					printf("Device push response completed.\r\n");
				}
				else
				{	
					printf("Device push response failed.\r\n");
				}				
			}
			else if (apiId == API_apiId_OTA){
				if(respCode == ERR_Success)
				{	
					printf("OTA upgrade response completed.\r\n");
				}
				else
				{	
					printf("OTA upgrade response failed.\r\n");
				}				
			}
		}
		cJSON_Delete(json);
}
u16_t len1;

void recvMsgHandle(void)   
{
  int len;		
	void *data;
	struct netbuf *buf ;
	if(( netconn_recv(tcpsocket, &buf)) == ERR_OK)
	{
		len = sizeof(socketInfo.inBuffer);
    memset(socketInfo.inBuffer,0x0,len);
			if((netbuf_data(buf, &data, &len1)) == ERR_OK)	  
			{
				memcpy(socketInfo.inBuffer,data,len1);
				socketInfo.inBuffer[len1] = '\0';    
        printf("receive %d bytes\r\n", len1);  
        printf("receive : %s \r\n",socketInfo.inBuffer);        		
				parseRecvMsgInfo(socketInfo.inBuffer);
				netbuf_delete(buf);   	
			}
	}
		else 
		{
			netbuf_delete(buf);
		}	
}

void connect_thread(void *arg)
{	
  LWIP_UNUSED_ARG(arg);	
	while(1)
	{
		if(!server_connect_Flag)
		{
			tcpsocket = netconn_new(NETCONN_TCP);
			netconn_set_recvtimeout(tcpsocket, 20);
			if(netconn_connect(tcpsocket, &server_ipaddr, port)!=ERR_OK)
			{
				printf("connect err!\n\r");
				netconn_delete(tcpsocket);
			}	
			else
			{
				printf("\r\n connect ok \n\r");
				server_connect_Flag = true;
			}
		}
		else
		{
			//Connection authorization
			if(!ConnectAuthorizationFlag)
			{
				sprintf(socketInfo.outBuffer, API_AUTH_Sendpack, API_AUTH, versionSN, API_AUTH_mac, API_AUTH_reconnect0);			
				netconn_write(tcpsocket, socketInfo.outBuffer, strlen(socketInfo.outBuffer), 1);
				printf("socketInfo.outBuffer = %s \n\r",socketInfo.outBuffer);
				memset( socketInfo.outBuffer, 0, sizeof(socketInfo.outBuffer) );
			}	
			recvMsgHandle();
		}
		vTaskDelay(100);
	}
}
