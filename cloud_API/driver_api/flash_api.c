#include "fsl_flash.h"
#include "stdio.h"
#include "flashLayout.h"
/*******************************************************************************
 * Definitions
 ******************************************************************************/


/*******************************************************************************
 * Variables
 ******************************************************************************/

/*! @brief Flash driver Structure */
static flash_config_t s_flashDriver;
flash_security_state_t securityStatus = kFLASH_SecurityStateNotSecure; /* Return protection status *///返回保护状态
/*******************************************************************************
 * Code
 ******************************************************************************/

/*
* @brief Gets called when an error occurs.
*
* @details Print error message and trap forever.
*/
void error_trap(void)
{
    printf("\r\n\r\n\r\n\t---- HALTED DUE TO FLASH ERROR! ----");
    while (1)
    {
    }
}

void flash_init(void)
{
		status_t result;    /* Return code from each flash driver function *///从每个闪存驱动程序函数返回代码
	    /* Clean up Flash driver Structure*/
    memset(&s_flashDriver, 0, sizeof(flash_config_t));

    /* Setup flash driver structure for device and initialize variables. *///设置设备的闪存驱动程序结构并初始化变量
    result = FLASH_Init(&s_flashDriver);
    if (kStatus_FLASH_Success != result)
    {
        error_trap();
    }		
    /* Check security status. *///检查安全状态
    result = FLASH_GetSecurityState(&s_flashDriver, &securityStatus);
    if (kStatus_FLASH_Success != result)
    {
        error_trap();
    }
    /* Print security status. */
    switch (securityStatus)
    {
        case kFLASH_SecurityStateNotSecure:
//            printf("\r\n Flash is UNSECURE!");
            break;
        case kFLASH_SecurityStateBackdoorEnabled:
            printf("\r\n Flash is SECURE, BACKDOOR is ENABLED!");
            break;
        case kFLASH_SecurityStateBackdoorDisabled:
            printf("\r\n Flash is SECURE, BACKDOOR is DISABLED!");
            break;
        default:
            break;
    }
    printf("\r\n");


}

int erase_sector(uint32_t start)
{
	status_t result;    /* Return code from each flash driver function *///从每个闪存驱动程序函数返回代码
    
	/* Test pflash basic opeation only if flash is unsecure. *///仅在闪存不安全时测试其基本操作
  if (kFLASH_SecurityStateNotSecure == securityStatus)
  {
    /* Debug message for user. */
    /* Erase several sectors on upper pflash block where there is no code *///擦除 flash 上没有代码的扇区 
//    printf("\r\n Erase a sector of flash");
	}	
	
	
	result = FLASH_Erase(&s_flashDriver, start, SECTOR_SIZE, kFLASH_ApiEraseKey);
	if (kStatus_FLASH_Success != result)
  {
    error_trap();
  }

  /* Verify sector if it's been erased. *///验证扇区是否被擦除
  result = FLASH_VerifyErase(&s_flashDriver, start, SECTOR_SIZE, kFLASH_MarginValueUser);
  if (kStatus_FLASH_Success != result)
  {
    error_trap();
  }
  /* Print message for user. */
//  printf("\r\n Successfully Erased Sector 0x%x -> 0x%x\r\n", start, (start + SECTOR_SIZE));
	return 1;
}

int program_flash(uint32_t start, uint32_t *src, uint32_t lengthInBytes)
{
	
	uint32_t failAddr, failDat;
	status_t result;    /* Return code from each flash driver function *///从每个闪存驱动程序函数返回代码
	/*! @brief Buffer for readback */
	static uint32_t s_buffer_rbc[4096]; // 1 sector size
/* Program user buffer into flash*///将 user buffer 编写进 flash
	result = FLASH_Program(&s_flashDriver, start, src, lengthInBytes);
  if (kStatus_FLASH_Success != result)
  {
    error_trap();
  }
	/* Verify programming by Program Check command with user margin levels *///通过 Program Check 命令和 user margin levels 来检验编程
	result = FLASH_VerifyProgram(&s_flashDriver, start, lengthInBytes, src, kFLASH_MarginValueUser, &failAddr, &failDat);
  if (kStatus_FLASH_Success != result)
  {
		error_trap();
  }
  /* Verify programming by reading back from flash directly*///通过读取 flash 的回读来检验编程
  for (uint32_t i = 0; i < (lengthInBytes/4); i++)
  {
		s_buffer_rbc[i] = *(volatile uint32_t *)(start + i * 4);
//		printf("\r\n%d\r\n",s_buffer_rbc[i]);
    if (s_buffer_rbc[i] != src[i])
    {
			error_trap();
    }
  }
//	printf("\r\n Successfully Programmed and Verified Location 0x%x -> 0x%x \r\n", start,
//  (start + lengthInBytes));
	return 1;
}
