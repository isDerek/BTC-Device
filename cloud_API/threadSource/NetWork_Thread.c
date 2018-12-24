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
//u16_t port = 55551; // Cloud API
u16_t port = 61111;
//u16_t port = 44441;
struct netconn *tcpsocket;
struct netbuf  *TCPNetbuf;
SocketInfo socketInfo;
extern OTAInfo otaInfo;
extern BTCInfo btcInfo;

volatile bool showFlag = false;
bool server_connect_Flag = false;
//SocketInfo socketInfo;
float x, y, z;//加速度
int UVA_data, UVB_data;
int lx;//光亮等级
uint32_t persure;
float temp, hum;
int red, green, blue;
int light_res;
int respCode;

u16_t len1;
extern char tempBuffer[256];//need to be fixed, save data flash use this buffer;
int otaBinTotalSize;

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
								erase_sector(OTA_CODE_START_ADDRESS+blockOffset);
								iapCode = program_flash(OTA_CODE_START_ADDRESS + blockOffset,(uint32_t *)buf, blockSize);  							
								otaInfo.blockOffset = blockOffset;
								strcpy(otaInfo.versionSN,versionSN);
//							  printf("len1 = %d , blockOffset = %d \n\r",len1,blockOffset);
								crc16 = calculate_crc16((char*)(OTA_CODE_START_ADDRESS + blockOffset),len1-BINDATA_POS);
								printf("crc16 rewrite before = %x\n\r",crc16);
								while(crc16 != checksum && reWrite < 10){   //try to rewrite data when verify failed
									printf("rewrite \r\n");
									erase_sector(OTA_CODE_START_ADDRESS+blockOffset);
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

			if(btcInfo.apiId == 24)
			{
				sprintf(otaInfo.versionSN,"%s",cJSON_GetObjectItem(json,"versionSN")->valuestring);
			  otaInfo.versionSize = cJSON_GetObjectItem(json,"versionSize")->valueint;
				sprintf(btcInfo.msgId,"%s",cJSON_GetObjectItem(json,"msgId")->valuestring);
				otaInfo.checkSum = cJSON_GetObjectItem(json,"checkSum")->valueint;
				printf("versionSN = %s\n\r",otaInfo.versionSN);
				printf("msgId = %s, apiId = %d,  versionSN = %s,versionSize =%d, checkSum = %d \n\r",btcInfo.msgId,btcInfo.apiId,otaInfo.versionSN,otaInfo.versionSize,otaInfo.checkSum);
			}
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

void OTAUpdate()
{
	uint16_t upDateBinCounter;
	int len= 0;
	int upDateCount;
	int otacrc16,codecrc16;			
	char* cdata = (char*)VERSION_STR_ADDRESS;
	int appBinTotalSize;
	upDateCount = (otaInfo.versionSize / 4096) + 1;//512
	PRINTF("UPdateCount = %d \r\n",upDateCount);
	for(upDateBinCounter = 0;upDateBinCounter<upDateCount;upDateBinCounter++)
	{		
		btcInfo.apiId = 4;
		otaInfo.blockSize = 4096;	//512
		otaInfo.blockOffset = 4096;//512				
		sprintf(socketInfo.outBuffer,NOTIFY_REQ_updateVersion,btcInfo.apiId,otaInfo.versionSN,otaInfo.blockOffset*upDateBinCounter,otaInfo.blockSize);
    len = strlen(socketInfo.outBuffer);
		printf("notifyMsgSendHandle,send %d bytes: %s\r\n",len,socketInfo.outBuffer);
		netconn_write(tcpsocket,socketInfo.outBuffer,len,1);
    recvMsgHandle();	
	}
	recvMsgHandle();	// if network slower than receive, this can receive otabin
	appBinTotalSize = (cdata[7]<<16)|(cdata[8]<<8)|(cdata[9]);
	codecrc16 = calculate_crc16((char*)(CODE_START_ADDRESS), appBinTotalSize);
	otacrc16 = calculate_crc16((char*)(OTA_CODE_START_ADDRESS), otaBinTotalSize);
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
	printf("otadownloadfinshedtotalsize = %d  appdownloadfinshedtotalsize = %d\n\r",otaBinTotalSize,appBinTotalSize);
	sprintf(tempBuffer+11,"%s",otaInfo.versionSN);
	__disable_irq();
	printf("tempbuff = %d \n\r", strlen(tempBuffer));
	erase_sector(VERSION_STR_ADDRESS);
	program_flash(VERSION_STR_ADDRESS,(uint32_t *)tempBuffer, 256);
	__enable_irq();
	if(otacrc16 == otaInfo.checkSum)
	{	
		btcInfo.deviceStatus = 10;
//		notifyMsgSendHandle(notifyOTAResult);
		btcInfo.apiId = 3;			
		sprintf(socketInfo.outBuffer,NOTIFY_REQ_otaDeviceStatus,btcInfo.apiId,btcInfo.deviceStatus);
		len = strlen(socketInfo.outBuffer);
		printf("notifyMsgSendHandle,send %d bytes: %s\r\n",len,socketInfo.outBuffer);
		netconn_write(tcpsocket,socketInfo.outBuffer,len,NETCONN_COPY);
	}
	else
	{
		btcInfo.deviceStatus = 11;
		btcInfo.apiId = 3;			
		sprintf(socketInfo.outBuffer,NOTIFY_REQ_otaDeviceStatus,btcInfo.apiId,btcInfo.deviceStatus);
		len = strlen(socketInfo.outBuffer);
		printf("notifyMsgSendHandle,send %d bytes: %s\r\n",len,socketInfo.outBuffer);
		netconn_write(tcpsocket,socketInfo.outBuffer,len,NETCONN_COPY);
	}				
	updateCode();	
}
int a = 1;
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
				if(a == 1){
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
					a = 0;
				}
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
