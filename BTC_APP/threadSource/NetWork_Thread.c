#include "globalParams.h"
#include "api.h"
#include "apiParams.h"
#include "cJSON.h"
#include "sensor.h"
#include "board.h"
#include "K64_api.h"
#include "fsl_i2c.h"
#include "flashLayout.h"
#include "lib_crc16.h"
#include "ota.h"
#include "stdlib.h"
#include "tools.h"
#include "md5Std.h"

u16_t port = 61111;  // 服务器的端口号

struct netconn *tcpsocket;  // Socket 套接字
struct netbuf  *TCPNetbuf; //网络数据 Buf
SocketInfo socketInfo; // socket 结构体
extern OTAInfo otaInfo; // ota 结构体
extern BTCInfo btcInfo; // 设备信息结构体

bool server_connect_Flag = false; // 设备是否连接服务器
float x, y, z;//加速度传感器返回的值
int UVA_data, UVB_data; // 紫外线传感器返回的值
int lx;//光强传感器返回的值
uint32_t persure; // 气压计返回的值
float temp, hum; // 温湿度传感器返回的值
int red, green, blue; // RGB 灯参数
int light_res; // 光敏电阻参数
int respCode;  // 响应状态

u16_t len1; // socket 接收数据长度
extern char tempBuffer[256];// 版本消息的 Buffer
int otaBinTotalSize; // ota 版本文件大小

/*
        HEADER     BLOCK OFFSET       BLOCK SIZE  CHECKSUM    Bin data
       "OTABIN" 0x00 0x00 0x00 0x00    0x10 0x00  0xFF 0xFF
*/
#define SOCKET_OTA_HEADER  "OTABIN"
#define BLOCKOFFSET_POS 6
#define BLOCKSIZE_POS 10
#define CHECKSUM_POS 12
#define BINDATA_POS 14
void parseBincodeBuffer(char *text)
{
    char* buf = NULL;
    int blockOffset = 0;
    int blockSize = 0;
    int checksum = 0;
    int crc16 = NULL;
	  int reWrite = 0;  	
    if(strncmp(text, SOCKET_OTA_HEADER, strlen(SOCKET_OTA_HEADER))== NULL) {
        blockOffset = ((text[BLOCKOFFSET_POS] << 24) | (text[BLOCKOFFSET_POS+1] << 16) |(text[BLOCKOFFSET_POS+2] << 8)|text[BLOCKOFFSET_POS+3]);
        blockSize = ((text[BLOCKSIZE_POS] << 8) | text[BLOCKSIZE_POS+1]);
        checksum = ((text[CHECKSUM_POS] << 8) | text[CHECKSUM_POS+1]);
        printf("read from server: blockoffset = %d blocksize = %d checksum = %x\r\n",blockOffset,blockSize,checksum);
        buf = (char*)malloc(blockSize);
        if(buf == NULL) 
				{
            printf("can't malloc enough memory!\r\n");
        } 
				else 
					{
            memcpy(buf,text+BINDATA_POS,blockSize);
						crc16 = calculate_crc16(buf, len1-BINDATA_POS);
            printf("crc16 = %x checkSum = %x\r\n",crc16,checksum);
            if( crc16 == checksum) {
                // update current sector to flash ota partition;
								int iapCode;
							  strcpy(versionSN,otaInfo.versionSN);
								__disable_irq();
								iapCode = program_flash(OTA_CODE_START_ADDRESS + blockOffset,(uint32_t *)buf, blockSize);  							
								otaInfo.blockOffset = blockOffset;
								strcpy(otaInfo.versionSN,versionSN);
//							  printf("len1 = %d , blockOffset = %d \n\r",len1,blockOffset);
								crc16 = calculate_crc16((char*)(OTA_CODE_START_ADDRESS + blockOffset),len1-BINDATA_POS);
								printf("crc16 rewrite before = %x\n\r",crc16);
								while(crc16 != checksum && reWrite < 10){   //try to rewrite data when verify failed
									printf("rewrite \r\n");
									erase_DefSize(OTA_CODE_START_ADDRESS+blockOffset,blockSize);
									iapCode = program_flash(OTA_CODE_START_ADDRESS + blockOffset,(uint32_t *)buf, len1-BINDATA_POS); 
									printf("iapCode=%d\r\n",iapCode);
									crc16 = calculate_crc16((char*)(OTA_CODE_START_ADDRESS + blockOffset),len1-BINDATA_POS);
									printf("crc16 rewrite later = %x\n\r",crc16);
									reWrite++;
								}
								otaBinTotalSize += (len1-BINDATA_POS);
								printf("rewrite times = %d\r\n",reWrite);
								if(crc16 == checksum)
									{
									printf("write and verify successfully\r\n");
									}
								else
									{
									printf("write failed!\r\n");
									eventHandle.getLatestFWFromServerFlag = false;   //stop update!
									NVIC_SystemReset();  //restart to recover ota
								}					
            }	       
        }
			free(buf);
    }
		else
		{
		}
		__enable_irq();
}
void parseRecvMsgInfo(char *text)
{
  cJSON *json, *json_data;	
	json=cJSON_Parse(text);
    if (!json)
		{
			parseBincodeBuffer(text);
      printf("not json string, start to parse another way!\r\n");
    } 
		else 
		{
			btcInfo.apiId = cJSON_GetObjectItem( json , "apiId")->valueint;
//			printf("apiId = %d\n\r",btcInfo.apiId);
			sprintf(btcInfo.msgId,"%s",cJSON_GetObjectItem(json,"msgId")->valuestring);		
			respCode = cJSON_GetObjectItem( json , "respCode")->valueint;
//			printf("respCode = %d\n\r",respCode);			
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
					else if(btcInfo.module == 100)
					{
						for(int i = 0; i<size ; i++)
						{
							if(i == 0)
							{
								cJSON *object = cJSON_GetArrayItem(array,i);
								sprintf(otaInfo.versionSN,"%s",object->valuestring);
							}
							else
							{
								cJSON *object = cJSON_GetArrayItem(array,i);
								btcInfo.configBuffer[i] = object->valueint;
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

void OTAUpdate()
{
	uint16_t upDateBinCounter;
	int len= 0;
	int upDateCount;
	int otacrc16,codecrc16;			
	char* cdata = (char*)VERSION_STR_ADDRESS;
	int appBinTotalSize;
	upDateCount = (otaInfo.versionSize / 1024) + 1;//512
	PRINTF("UPdateCount = %d \r\n",upDateCount);
	__disable_irq();
   for(int i=0; i<CODE_SECTOR_NUM; i++) {
     erase_sector(OTA_CODE_START_ADDRESS+i*SECTOR_SIZE);
   }
	 __enable_irq();
	for(upDateBinCounter = 0;upDateBinCounter<upDateCount;upDateBinCounter++)
	{		
		btcInfo.apiId = 4;
		otaInfo.blockSize = 1024;	//512
		otaInfo.blockOffset = 1024;//512				
		sprintf(socketInfo.outBuffer,NOTIFY_REQ_updateVersion,btcInfo.apiId,otaInfo.versionSN,otaInfo.blockOffset*upDateBinCounter,otaInfo.blockSize);
    len = strlen(socketInfo.outBuffer);
		printf("notifyMsgSendHandle,send %d bytes: %s\r\n",len,socketInfo.outBuffer);
		netconn_write(tcpsocket,socketInfo.outBuffer,len,1);
    recvMsgHandle();
		delay_30ms();
	}
	recvMsgHandle();	// if network slower than receive, this can receive otabin
	appBinTotalSize = (cdata[7]<<16)|(cdata[8]<<8)|(cdata[9]);
	codecrc16 = calculate_crc16((char*)(CODE_START_ADDRESS), appBinTotalSize);
	otacrc16 = calculate_crc16((char*)(OTA_CODE_START_ADDRESS), otaBinTotalSize);
	printf("appcrc16 = %x, otacrc16 = %x\n\r",codecrc16,otacrc16);
	memset(tempBuffer,0x0,VERSION_STR_LEN);		
	tempBuffer[0] = (codecrc16 >> 8) & 0xff;  // appcodechecksum
	tempBuffer[1] = codecrc16 & 0xff; // appcodechecksum			
	tempBuffer[2] = (otacrc16 >> 8) & 0xff;  // otacodechecksum
	tempBuffer[3] = otacrc16 & 0xff; // otacodechecksum
	tempBuffer[4] = (otaBinTotalSize >> 16) & 0xff; // otatotal real size
	tempBuffer[5] = (otaBinTotalSize >> 8) & 0xff;// otatotal real size
	tempBuffer[6] = (otaBinTotalSize) & 0xff;// otatotal real size
	tempBuffer[7] = (appBinTotalSize >> 16) & 0xff; // otatotal real size
	tempBuffer[8] = (appBinTotalSize >> 8) & 0xff;// otatotal real size
	tempBuffer[9] = (appBinTotalSize) & 0xff;// otatotal real size		
	tempBuffer[10] = '1'; //1 means finish the first boot , 0 is not
	tempBuffer[11] = cdata[11];
	tempBuffer[12] = cdata[12];
	tempBuffer[13] = cdata[13];
	printf("otadownloadfinshedtotalsize = %d  appdownloadfinshedtotalsize = %d\n\r",otaBinTotalSize,appBinTotalSize);
	sprintf(tempBuffer+14,"%s",otaInfo.versionSN);
	printf("tempBuffer +14  = %s otacrc16 = %x otaInfo.checkSum = %x \n\r",tempBuffer+14,otacrc16,otaInfo.checkSum);
	__disable_irq();
	erase_sector(VERSION_STR_ADDRESS);
	program_flash(VERSION_STR_ADDRESS,(uint32_t *)tempBuffer, 256);
	__enable_irq();
	if(otacrc16 == otaInfo.checkSum)
	{	
		btcInfo.deviceStatus = 10;
//		notifyMsgSendHandle(notifyOTAResult);
		btcInfo.apiId = 4 ;			
		sprintf(socketInfo.outBuffer,NOTIFY_REQ_otaDeviceStatus,btcInfo.apiId,btcInfo.deviceStatus);
		len = strlen(socketInfo.outBuffer);
		printf("notifyMsgSendHandle,send %d bytes: %s\r\n",len,socketInfo.outBuffer);
		netconn_write(tcpsocket,socketInfo.outBuffer,len,NETCONN_COPY);
	}
	else
	{
		btcInfo.deviceStatus = 11;
		btcInfo.apiId = 4;			
		sprintf(socketInfo.outBuffer,NOTIFY_REQ_otaDeviceStatus,btcInfo.apiId,btcInfo.deviceStatus);
		len = strlen(socketInfo.outBuffer);
		printf("notifyMsgSendHandle,send %d bytes: %s\r\n",len,socketInfo.outBuffer);
		netconn_write(tcpsocket,socketInfo.outBuffer,len,NETCONN_COPY);
	}		
	updateCode();	
}
void connect_thread(void *arg)
{	
  LWIP_UNUSED_ARG(arg);	
	while(1)
	{
		if(!server_connect_Flag)
		{
			LED_GREEN_OFF();
			tcpsocket = netconn_new(NETCONN_TCP);
			netconn_set_recvtimeout(tcpsocket, 20);
			if(netconn_connect(tcpsocket, &server_ipaddr, port)!=ERR_OK)
			{
				printf("connect err!\n\r");
				netconn_delete(tcpsocket);
			}	
			else
			{
				LED_GREEN_ON();
				printf("\r\n connect ok \n\r");
				server_connect_Flag = true;
			}
		}
		else
		{
			if(!ConnectAuthorizationFlag)
			{
				char* cdata = (char*)VERSION_STR_ADDRESS;
				btcInfo.userID = cdata[12];
				btcInfo.deviceID = cdata[13];
				if(btcInfo.userID == 0 && btcInfo.deviceID == 0)
				{
					char *noRegister = "No Register";
					API_OLED_Clear();
					OLED_ShowStr(0, 0, (uint8_t*)noRegister);
				}
//				printf("firstboot = %c,register = %d, userid = %d,deviceid = %d\n\r",cdata[10],cdata[11],cdata[12],cdata[13]);
				sprintf(socketInfo.outBuffer, API_AUTH_Sendpack, API_SEND_AUTH, versionSN, btcInfo.mac[0], btcInfo.mac[1], btcInfo.mac[2], btcInfo.mac[3], btcInfo.mac[4], btcInfo.mac[5], btcInfo.userID, btcInfo.deviceID);			
				netconn_write(tcpsocket, socketInfo.outBuffer, strlen(socketInfo.outBuffer), 1);
				printf("socketInfo.outBuffer = %s \n\r",socketInfo.outBuffer);
				delay_s();
			}
			else
			{
				if(eventHandle.getLatestFWFromServerFlag == true) 
				{
				//	printf("update OTA from server!\n\r");
					OTAUpdate();
					eventHandle.getLatestFWFromServerFlag =false;
				}
			}
			recvMsgHandle();
		}
		vTaskDelay(500);
	}
}
