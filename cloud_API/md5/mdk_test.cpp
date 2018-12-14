#include "md5Std.h"
#include "board.h"
#include "pin_mux.h"
#include "stdio.h"
#include "flashLayout.h"
int calculateBinSize(unsigned char* binStartAddress, int checkTotalSize)
{
		unsigned int binTotalSize;
		for(binTotalSize = 0 ; binTotalSize < CODE_SIZE ; binTotalSize ++)
	{
		int checkSize = 0 , validSize = 0;
		if(*(binStartAddress+binTotalSize) == 0xFF )
		{		
			for(checkSize = 0; checkSize<checkTotalSize;checkSize++)
			{
				if(*(binStartAddress+binTotalSize+checkSize) == 0xFF)
				{
					validSize++;
				}
			}		
		}
		if(validSize == checkTotalSize)
		{
			printf("binTotalSize = %d\n\r",binTotalSize);
			return binTotalSize;
		}
	}		
		return 0;	
}


int main(int argc, char *argv[])  
{  
		CLOCK_EnableClock(kCLOCK_Iocon); 
		CLOCK_AttachClk(BOARD_DEBUG_UART_CLK_ATTACH7);  //added by derek 2018.1.29 for debug uart
	  BOARD_InitPins();
    BOARD_BootClockFROHF48M();
    BOARD_InitDebugConsole();
    MD5_STD_CTX md5;  
    md5Init(&md5);           
    int i;  
		int totalsize;
    unsigned char encrypt[] ="admin";//21232f297a57a5a743894a0e4a801fc3
		unsigned char *binStartAddress= NULL;
    unsigned char decrypt[16];  
		binStartAddress =(unsigned char*)0x8000;
		totalsize = calculateBinSize(binStartAddress,30);
	  printf("totalsize = %d\n\r",totalsize);
		md5Update(&md5,binStartAddress,totalsize);  
//    md5Update(&md5,binStartAddress,strlen((char *)encrypt));  
    md5Final(&md5,decrypt);   
    printf("加密前:%s\n加密后16位:",encrypt); //5995
    for(i=4;i<12;i++)  
    {  
        printf("%02x",decrypt[i]);   
    }  
       
    printf("\n加密前:%s\n加密后32位:",encrypt);  
    for(i=0;i<16;i++)  
    {  
        printf("%02x",decrypt[i]);  
    }  
  
    getchar();  
  
    return 0;  
} 
