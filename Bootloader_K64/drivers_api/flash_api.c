#include "K64_api.h"
#include "stdio.h"
#include "fsl_flash.h"
#if defined(FSL_FEATURE_HAS_L1CACHE) && FSL_FEATURE_HAS_L1CACHE
#include "fsl_cache.h" 
#endif /* FSL_FEATURE_HAS_L1CACHE */
/*******************************************************************************
 * Definitions
 ******************************************************************************/
#define FLASH_SECTOR_SIZE 0x1000
/*******************************************************************************
 * Variables
 ******************************************************************************/
 /*! @brief Flash driver Structure */
static flash_config_t s_flashDriver;
/*! @brief Flash cache driver Structure */
static ftfx_cache_config_t s_cacheDriver;

ftfx_security_state_t securityStatus = kFTFx_SecurityStateNotSecure; /* Return protection status */
 /*
* @brief Gets called when an error occurs.
*
* @details Print error message and trap forever.
*/
void error_trap(void)
{
    printf("\r\n\r\n\r\n\t---- HALTED DUE TO FLASH ERROR! ----");
	  while(1){
		}
}


void flash_init(void)
	{

	status_t result;    /* Return code from each flash driver function */

    /* Clean up Flash, Cache driver Structure*/
	
    memset(&s_flashDriver, 0, sizeof(flash_config_t));
    memset(&s_cacheDriver, 0, sizeof(ftfx_cache_config_t));
	
/* Setup flash driver structure for device and initialize variables. */
    result = FLASH_Init(&s_flashDriver);
    if (kStatus_FTFx_Success != result)
    {
        error_trap();
    }
    /* Setup flash cache driver structure for device and initialize variables. */
    result = FTFx_CACHE_Init(&s_cacheDriver);
    if (kStatus_FTFx_Success != result)
    {
        error_trap();
    }
		
    /* Check security status. */
    result = FLASH_GetSecurityState(&s_flashDriver, &securityStatus);
    if (kStatus_FTFx_Success != result)
    {
        error_trap();
    }
    /* Print security status. */
    switch (securityStatus)
    {
        case kFTFx_SecurityStateNotSecure:
//            printf("\r\n Flash is UNSECURE!");
            break;
        case kFTFx_SecurityStateBackdoorEnabled:
            printf("\r\n Flash is SECURE, BACKDOOR is ENABLED!");
            break;
        case kFTFx_SecurityStateBackdoorDisabled:
            printf("\r\n Flash is SECURE, BACKDOOR is DISABLED!");
            break;
        default:
            break;
    }
    printf("\r\n");	
}
int erase_sector(uint32_t start)
	{
				status_t result; 
    /* Test pflash basic opeation only if flash is unsecure. */
				if (kFTFx_SecurityStateNotSecure == securityStatus)
				{
						/* Pre-preparation work about flash Cache/Prefetch/Speculation. */
						FTFx_CACHE_ClearCachePrefetchSpeculation(&s_cacheDriver, true);

						/* Debug message for user. */
						/* Erase several sectors on upper pflash block where there is no code */
						printf("\r\n Erase a sector of flash");
				}
        result = FLASH_Erase(&s_flashDriver, start, FLASH_SECTOR_SIZE, kFTFx_ApiEraseKey);
        if (kStatus_FTFx_Success != result)
        {
            error_trap();
        }
        /* Verify sector if it's been erased. */
        result = FLASH_VerifyErase(&s_flashDriver, start, FLASH_SECTOR_SIZE, kFTFx_MarginValueUser);
        if (kStatus_FTFx_Success != result)
        {
            error_trap();
        }

        /* Print message for user. */
        printf("\r\n Successfully Erased Sector 0x%x -> 0x%x\r\n", start, (start + FLASH_SECTOR_SIZE));	
				return 1;
}

int program_flash(uint32_t start, uint32_t *src, uint32_t lengthInBytes)
{
	static uint32_t s_buffer_rbc[4096];	 // 1 sector size
	status_t result; 
	uint32_t failAddr, failDat;
/* Program user buffer into flash*/
        result = FLASH_Program(&s_flashDriver, start, (uint8_t *)src, lengthInBytes);
        if (kStatus_FTFx_Success != result)
        {
            error_trap();
        }

        /* Verify programming by Program Check command with user margin levels */
        result = FLASH_VerifyProgram(&s_flashDriver, start, lengthInBytes, (const uint8_t *)src, kFTFx_MarginValueUser,
                                     &failAddr, &failDat);
        if (kStatus_FTFx_Success != result)
        {
            error_trap();
        }

        /* Post-preparation work about flash Cache/Prefetch/Speculation. */
        FTFx_CACHE_ClearCachePrefetchSpeculation(&s_cacheDriver, false);

#if defined(FSL_FEATURE_HAS_L1CACHE) && FSL_FEATURE_HAS_L1CACHE
        L1CACHE_InvalidateCodeCache();
#endif /* FSL_FEATURE_HAS_L1CACHE */

#if defined(__DCACHE_PRESENT) && __DCACHE_PRESENT
        /* Clean the D-Cache before reading the flash data*/
        SCB_CleanInvalidateDCache();
#endif
        /* Verify programming by reading back from flash directly*/
        for (uint32_t i = 0; i < (lengthInBytes/4); i++)
        {
            s_buffer_rbc[i] = *(volatile uint32_t *)(start + i * 4);
            if (s_buffer_rbc[i] != src[i])
            {
                error_trap();
            }
        }
        printf("\r\n Successfully Programmed and Verified Location 0x%x -> 0x%x \r\n", start,
               (start + lengthInBytes));
					return 1;
}
