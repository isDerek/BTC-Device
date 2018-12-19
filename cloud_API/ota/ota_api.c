/*
* Note:last modification by orange cai in 20170328
*/

#include "ota.h"
#include "flashLayout.h"
#include "lib_crc16.h"
#include "stdio.h"
#include "string.h"
#include "md5Std.h"
#include "K64_api.h"
#include "tools.h"
#include "globalParams.h"

OTAInfo otaInfo;
char versionSN [33] ="00000000000000000000000000000000";

void updateCode(void);

void OTAInit(void)
{
    char* cdata = (char*)VERSION_STR_ADDRESS;
    char* codePartition = (char*) CODE_START_ADDRESS;
    char* OTACodePartition = (char*) OTA_CODE_START_ADDRESS;
    int i = 0;
		int binTotalSize;
    int codecrc16,otacodecrc16,codechecksum;
		char tempBuffer[256];
		unsigned char* appStartAddress = (unsigned char*) CODE_START_ADDRESS;
    codechecksum = (cdata[0]<<8)|cdata[1];
		printf("app calculate codecrc16 =%x codechecksum = %x\n\r",codecrc16,codechecksum);
//	printf("cdata10 = %x \n\r",cdata[10]);
    if((codechecksum==0xFFFF)||(codechecksum==0x0)) {

        printf("first boot, recover ota partition!\r\n");
			 __disable_irq();

        for(i=0; i<CODE_SECTOR_NUM; i++) {
            erase_sector(OTA_CODE_START_ADDRESS+i*SECTOR_SIZE);
            program_flash(OTA_CODE_START_ADDRESS+i*SECTOR_SIZE,(uint32_t *)(codePartition+i*SECTOR_SIZE),SECTOR_SIZE);
        }
				binTotalSize = calculateBinSize(codePartition,20);
				md5Calculate(appStartAddress,binTotalSize,(unsigned char*)otaInfo.versionSN);
//				printf("binTotalSize = %d\n\r",binTotalSize);
				codecrc16 = calculate_crc16(codePartition,binTotalSize);
        otacodecrc16 = calculate_crc16(OTACodePartition, binTotalSize);
//				printf("otacodecrc16 = %x codecrc16 = %x\n\r",otacodecrc16, codecrc16);
        if(otacodecrc16 == codecrc16) {
            memset(tempBuffer,0x0,VERSION_STR_LEN);
					/*   tempBuffer[0],tempBuffer[1] = appcodechecksum && tempBuffer[2],tempBuffer[3] = otacodechecksum */
            tempBuffer[0] = tempBuffer[2] = (codecrc16 >> 8) & 0xff;  // appcodechecksum && otacodechecksum
            tempBuffer[1] = tempBuffer[3] = codecrc16 & 0xff; // appcodechecksum && otacodechecksum
						tempBuffer[4] = tempBuffer[7] = (binTotalSize >> 16) & 0xff;  // first boot otabin is  real size
						tempBuffer[5] = tempBuffer[8] = (binTotalSize >> 8) & 0xff;  // first boot otabin is  real size
					  tempBuffer[6] = tempBuffer[9] =	(binTotalSize) & 0xff;  // first boot otabin is  real size
					  tempBuffer[10] = '1';
						sprintf(versionSN,"%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x",otaInfo.versionSN[0],otaInfo.versionSN[1],otaInfo.versionSN[2],otaInfo.versionSN[3],otaInfo.versionSN[4],otaInfo.versionSN[5],otaInfo.versionSN[6],otaInfo.versionSN[7],otaInfo.versionSN[8],otaInfo.versionSN[9],otaInfo.versionSN[10],otaInfo.versionSN[11],otaInfo.versionSN[12],otaInfo.versionSN[13],otaInfo.versionSN[14],otaInfo.versionSN[15]);
//						printf("×ª»»ºó :%s\n\r",versionSN);	
						sprintf(tempBuffer+11,"%s",versionSN);
            erase_sector(VERSION_STR_ADDRESS);
            program_flash(VERSION_STR_ADDRESS,(uint32_t *)tempBuffer, 256);
        }
				__enable_irq();
				printf("update, system will be reset now, please wait...\r\n");
				NVIC_SystemReset();  
    } else {		
						binTotalSize = calculateBinSize(codePartition,20);
						md5Calculate(appStartAddress,binTotalSize,(unsigned char*)otaInfo.versionSN);
						sprintf(versionSN,"%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x",otaInfo.versionSN[0],otaInfo.versionSN[1],otaInfo.versionSN[2],otaInfo.versionSN[3],otaInfo.versionSN[4],otaInfo.versionSN[5],otaInfo.versionSN[6],otaInfo.versionSN[7],otaInfo.versionSN[8],otaInfo.versionSN[9],otaInfo.versionSN[10],otaInfo.versionSN[11],otaInfo.versionSN[12],otaInfo.versionSN[13],otaInfo.versionSN[14],otaInfo.versionSN[15]);
						
						printf("Current APP checksum: %2X%2X OTA checksum: %2X%2X  otaBintotal: %d  appBintotal: %d  versionSN:  %s \n\r",cdata[0],cdata[1],cdata[2],cdata[3],(cdata[4]<<16)|(cdata[5]<<8)|(cdata[6]),(cdata[7]<<16)|(cdata[8]<<8)|(cdata[9]),cdata+11);
						sprintf(otaInfo.versionSN,"%s",versionSN);	
//						printf("otaInfo.versionSN =  %s\n\r",otaInfo.versionSN);
            printf("the FW is already the newest!\r\n");
    }

}


void updateCode(void)
{
    printf("update, system will be reset now, please wait...\r\n");
    NVIC_SystemReset();  
}




