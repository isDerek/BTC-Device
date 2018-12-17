#include "globalParams.h"
#include "api.h"
#include "apiParams.h"
#include "cJSON.h"
#include "sensor.h"
#include "board.h"
#include "K64_api.h"
#include "fsl_i2c.h"
#include "flashLayout.h"

u16_t port = 55551;
struct netconn *tcpsocket;
struct netbuf  *TCPNetbuf;
SocketInfo socketInfo;
extern OTAInfo otaInfo;
BTCInfo btcInfo;

volatile bool showFlag = false;
bool server_connect_Flag = false;
//SocketInfo socketInfo;
float x, y, z;//加速度
uint16_t UVA_data, UVB_data;
float lx;//光亮等级
uint32_t persure;
float temp, hum;
int red, green, blue;
int respCode;

void parseRecvMsgInfo(char *text)
{
  cJSON *json, *json_data;	
	json=cJSON_Parse(text);
    if (!json)
		{
      printf("not json string, start to parse another way!\r\n");
    } 
		else 
		{
			btcInfo.apiId = cJSON_GetObjectItem( json , "apiId")->valueint;
			
			sprintf(btcInfo.msgId,"%s",cJSON_GetObjectItem(json,"msgId")->valuestring);		
					
			respCode = cJSON_GetObjectItem( json , "respCode")->valueint;
						
			json_data = cJSON_GetObjectItem( json , "data");
			if(json_data)
			{
				cJSON *array;
				btcInfo.module = cJSON_GetObjectItem( json_data , "module")->valueint; 
				array = cJSON_GetObjectItem( json_data , "config");
				if(!array){
					
				}
				else
				{
					int size = cJSON_GetArraySize(array);
					if(btcInfo.module == 4)
					{
						for(int i = 0; i<size ; i++)
						{
							cJSON *object = cJSON_GetArrayItem(array,i);
							if( i == 0)
							{
								sprintf(btcInfo.oledOneLine,"%s",object->valuestring);									
							}
							else if(i == 1)
							{
								sprintf(btcInfo.oledSecondLine,"%s",object->valuestring);
							}
							else if(i == 2)
							{
								sprintf(btcInfo.oledThirdLine,"%s",object->valuestring);
							}
							else if(i == 3)
							{
								sprintf(btcInfo.oledForthLine,"%s",object->valuestring);
							}					
						}					
					}
					else
					{
						for(int i = 0; i<size ; i++)
						{
							cJSON *object = cJSON_GetArrayItem(array,i);
							btcInfo.configBuffer[i] = object->valueint;
//							printf("buffer[%d] = %d\n\r",i,btcInfo.configBuffer[i]);
						}					
					}					
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
			if(!ConnectAuthorizationFlag)
			{
				sprintf(socketInfo.outBuffer, API_AUTH_Sendpack, API_AUTH, versionSN, API_AUTH_mac, API_AUTH_reconnect0);			
				netconn_write(tcpsocket, socketInfo.outBuffer, strlen(socketInfo.outBuffer), 1);
				printf("socketInfo.outBuffer = %s \n\r",socketInfo.outBuffer);
				memset( socketInfo.outBuffer, 0, sizeof(socketInfo.outBuffer) );
			}	
			recvMsgHandle();
		}
		vTaskDelay(1000);
	}
}
