#include "fsl_device_registers.h"
#include "fsl_debug_console.h"
#include "board.h"
#include "clock_config.h"
#include "fsl_flash.h"
#if defined(FSL_FEATURE_HAS_L1CACHE) && FSL_FEATURE_HAS_L1CACHE
#include "fsl_cache.h"
#endif /* FSL_FEATURE_HAS_L1CACHE */

#include "pin_mux.h"

/*******************************************************************************
 * Definitions
 ******************************************************************************/

#define kStatusGroupFtfxDriver 1
#define BUFFER_LEN 4

/*! @brief FTFx cache driver state information.
 *
 * An instance of this structure is allocated by the user of the flash driver and
 * passed into each of the driver APIs.
 */
typedef struct _ftfx_cache_config
{
    uint8_t flashMemoryIndex;     /*!< 0 - primary flash; 1 - secondary flash*/
    uint8_t reserved[3];
    uint32_t *comBitOperFuncAddr; /*!< An buffer point to the flash execute-in-RAM function. */
} ftfx_cache_config_t;

/*!
 * @brief Enumeration for the three possible FTFx security states.
 */
typedef enum _ftfx_security_state
{
    kFTFx_SecurityStateNotSecure = 0xc33cc33cU,       /*!< Flash is not secure.*/
    kFTFx_SecurityStateBackdoorEnabled = 0x5aa55aa5U, /*!< Flash backdoor is enabled.*/
    kFTFx_SecurityStateBackdoorDisabled = 0x5ac33ca5U /*!< Flash backdoor is disabled.*/
} ftfx_security_state_t;

/*!
 * @brief FTFx driver status codes.
 */
enum _ftfx_status
{
    kStatus_FTFx_Success = MAKE_STATUS(kStatusGroupGeneric, 0),         /*!< API is executed successfully*/
    kStatus_FTFx_InvalidArgument = MAKE_STATUS(kStatusGroupGeneric, 4), /*!< Invalid argument*/
    kStatus_FTFx_SizeError = MAKE_STATUS(kStatusGroupFtfxDriver, 0),   /*!< Error size*/
    kStatus_FTFx_AlignmentError =
        MAKE_STATUS(kStatusGroupFtfxDriver, 1), /*!< Parameter is not aligned with the specified baseline*/
    kStatus_FTFx_AddressError = MAKE_STATUS(kStatusGroupFtfxDriver, 2), /*!< Address is out of range */
    kStatus_FTFx_AccessError =
        MAKE_STATUS(kStatusGroupFtfxDriver, 3), /*!< Invalid instruction codes and out-of bound addresses */
    kStatus_FTFx_ProtectionViolation = MAKE_STATUS(
        kStatusGroupFtfxDriver, 4), /*!< The program/erase operation is requested to execute on protected areas */
    kStatus_FTFx_CommandFailure =
        MAKE_STATUS(kStatusGroupFtfxDriver, 5), /*!< Run-time error during command execution. */
    kStatus_FTFx_UnknownProperty = MAKE_STATUS(kStatusGroupFtfxDriver, 6), /*!< Unknown property.*/
    kStatus_FTFx_EraseKeyError = MAKE_STATUS(kStatusGroupFtfxDriver, 7),   /*!< API erase key is invalid.*/
    kStatus_FTFx_RegionExecuteOnly =
        MAKE_STATUS(kStatusGroupFtfxDriver, 8), /*!< The current region is execute-only.*/
    kStatus_FTFx_ExecuteInRamFunctionNotReady =
        MAKE_STATUS(kStatusGroupFtfxDriver, 9), /*!< Execute-in-RAM function is not available.*/
    kStatus_FTFx_PartitionStatusUpdateFailure =
        MAKE_STATUS(kStatusGroupFtfxDriver, 10), /*!< Failed to update partition status.*/
    kStatus_FTFx_SetFlexramAsEepromError =
        MAKE_STATUS(kStatusGroupFtfxDriver, 11), /*!< Failed to set FlexRAM as EEPROM.*/
    kStatus_FTFx_RecoverFlexramAsRamError =
        MAKE_STATUS(kStatusGroupFtfxDriver, 12), /*!< Failed to recover FlexRAM as RAM.*/
    kStatus_FTFx_SetFlexramAsRamError = MAKE_STATUS(kStatusGroupFtfxDriver, 13), /*!< Failed to set FlexRAM as RAM.*/
    kStatus_FTFx_RecoverFlexramAsEepromError =
        MAKE_STATUS(kStatusGroupFtfxDriver, 14), /*!< Failed to recover FlexRAM as EEPROM.*/
    kStatus_FTFx_CommandNotSupported = MAKE_STATUS(kStatusGroupFtfxDriver, 15), /*!< Flash API is not supported.*/
    kStatus_FTFx_SwapSystemNotInUninitialized =
        MAKE_STATUS(kStatusGroupFtfxDriver, 16), /*!< Swap system is not in an uninitialzed state.*/
    kStatus_FTFx_SwapIndicatorAddressError =
        MAKE_STATUS(kStatusGroupFtfxDriver, 17), /*!< The swap indicator address is invalid.*/
    kStatus_FTFx_ReadOnlyProperty = MAKE_STATUS(kStatusGroupFtfxDriver, 18), /*!< The flash property is read-only.*/
    kStatus_FTFx_InvalidPropertyValue =
        MAKE_STATUS(kStatusGroupFtfxDriver, 19), /*!< The flash property value is out of range.*/
    kStatus_FTFx_InvalidSpeculationOption =
        MAKE_STATUS(kStatusGroupFtfxDriver, 20), /*!< The option of flash prefetch speculation is invalid.*/
};
/*@}*/

status_t FLASH_Init(flash_config_t *config)
{
    status_t returnCode;

    if (config == NULL)
    {
        return kStatus_FTFx_InvalidArgument;
    }

    for (uint8_t flashIndex = 0; flashIndex < FTFx_FLASH_COUNT; flashIndex++)
    {
        uint32_t pflashStartAddress;
        uint32_t pflashBlockSize;
        uint32_t pflashBlockCount;
        uint32_t pflashBlockSectorSize;
        uint32_t pflashProtectionRegionCount;
        uint32_t pflashBlockWriteUnitSize;
        uint32_t pflashSectorCmdAlignment;
        uint32_t pflashSectionCmdAlignment;
        uint32_t pfsizeMask;
        uint32_t pfsizeShift;
        uint32_t facssValue;
        uint32_t facsnValue;

        config->ftfxConfig[flashIndex].flashDesc.type = kFTFx_MemTypePflash;
        config->ftfxConfig[flashIndex].flashDesc.index = flashIndex;
        flash_init_features(&config->ftfxConfig[flashIndex]);

#if FTFx_DRIVER_HAS_FLASH1_SUPPORT
        if(flashIndex == 1)
        {
            pflashStartAddress = FLASH1_FEATURE_PFLASH_START_ADDRESS;
            pflashBlockSize = FLASH1_FEATURE_PFLASH_BLOCK_SIZE;
            pflashBlockCount = FLASH1_FEATURE_PFLASH_BLOCK_COUNT;
            pflashBlockSectorSize = FLASH1_FEATURE_PFLASH_BLOCK_SECTOR_SIZE;
            pflashProtectionRegionCount = FLASH1_FEATURE_PFLASH_PROTECTION_REGION_COUNT;
            pflashBlockWriteUnitSize = FLASH1_FEATURE_PFLASH_BLOCK_WRITE_UNIT_SIZE;
            pflashSectorCmdAlignment = FLASH1_FEATURE_PFLASH_SECTOR_CMD_ADDRESS_ALIGMENT;
            pflashSectionCmdAlignment = FLASH1_FEATURE_PFLASH_SECTION_CMD_ADDRESS_ALIGMENT;
            pfsizeMask = SIM_FLASH1_PFSIZE_MASK;
            pfsizeShift = SIM_FLASH1_PFSIZE_SHIFT;
            facssValue = FTFx_FACSSS_REG;
            facsnValue = FTFx_FACSNS_REG;
        }
        else
#endif
        {
            pflashStartAddress = FLASH0_FEATURE_PFLASH_START_ADDRESS;
            pflashBlockSize = FLASH0_FEATURE_PFLASH_BLOCK_SIZE;
            pflashBlockCount = FLASH0_FEATURE_PFLASH_BLOCK_COUNT;
            pflashBlockSectorSize = FLASH0_FEATURE_PFLASH_BLOCK_SECTOR_SIZE;
            pflashProtectionRegionCount = FLASH0_FEATURE_PFLASH_PROTECTION_REGION_COUNT;
            pflashBlockWriteUnitSize = FLASH0_FEATURE_PFLASH_BLOCK_WRITE_UNIT_SIZE;
            pflashSectorCmdAlignment = FLASH0_FEATURE_PFLASH_SECTOR_CMD_ADDRESS_ALIGMENT;
            pflashSectionCmdAlignment = FLASH0_FEATURE_PFLASH_SECTION_CMD_ADDRESS_ALIGMENT;
            pfsizeMask = SIM_FLASH0_PFSIZE_MASK;
            pfsizeShift = SIM_FLASH0_PFSIZE_SHIFT;
            facssValue = FTFx_FACSS_REG;
            facsnValue = FTFx_FACSN_REG;
        }

        config->ftfxConfig[flashIndex].flashDesc.blockBase = pflashStartAddress;
        config->ftfxConfig[flashIndex].flashDesc.blockCount = pflashBlockCount;
        config->ftfxConfig[flashIndex].flashDesc.sectorSize = pflashBlockSectorSize;

        if (config->ftfxConfig[flashIndex].flashDesc.feature.isIndBlock &&
            config->ftfxConfig[flashIndex].flashDesc.feature.hasIndPfsizeReg)
        {
            config->ftfxConfig[flashIndex].flashDesc.totalSize = flash_calculate_mem_size(pflashBlockCount, pflashBlockSize, pfsizeMask, pfsizeShift);
        }
        else
        {
            config->ftfxConfig[flashIndex].flashDesc.totalSize = pflashBlockCount * pflashBlockSize;
        }

        if (config->ftfxConfig[flashIndex].flashDesc.feature.hasXaccControl)
        {
            ftfx_spec_mem_t *specMem;
            specMem = &config->ftfxConfig[flashIndex].flashDesc.accessSegmentMem;
            if (config->ftfxConfig[flashIndex].flashDesc.feature.hasIndXaccReg)
            {
                specMem->base = config->ftfxConfig[flashIndex].flashDesc.blockBase;
                specMem->size = kFTFx_AccessSegmentUnitSize << facssValue;
                specMem->count = facsnValue;
            }
            else
            {
                specMem->base = config->ftfxConfig[0].flashDesc.blockBase;
                specMem->size = kFTFx_AccessSegmentUnitSize << FTFx_FACSS_REG;
                specMem->count = FTFx_FACSN_REG;
            }
        }

        if (config->ftfxConfig[flashIndex].flashDesc.feature.hasProtControl)
        {
            ftfx_spec_mem_t *specMem;
            specMem = &config->ftfxConfig[flashIndex].flashDesc.protectRegionMem;
            if (config->ftfxConfig[flashIndex].flashDesc.feature.hasIndProtReg)
            {
                specMem->base = config->ftfxConfig[flashIndex].flashDesc.blockBase;
                specMem->count = pflashProtectionRegionCount;
                specMem->size = flash_calculate_prot_segment_size(config->ftfxConfig[flashIndex].flashDesc.totalSize, specMem->count);
            }
            else
            {
                uint32_t pflashTotalSize = 0;
                specMem->base = config->ftfxConfig[0].flashDesc.blockBase;
                specMem->count = FLASH0_FEATURE_PFLASH_PROTECTION_REGION_COUNT;
#if (FTFx_FLASH_COUNT != 1)
                if (flashIndex == FTFx_FLASH_COUNT - 1)
#endif
                {
                    uint32_t segmentSize;
                    for (uint32_t i = 0; i < FTFx_FLASH_COUNT; i++)
                    {
                         pflashTotalSize += config->ftfxConfig[flashIndex].flashDesc.totalSize;
                    }
                    segmentSize = flash_calculate_prot_segment_size(pflashTotalSize, specMem->count);
                    for (uint32_t i = 0; i < FTFx_FLASH_COUNT; i++)
                    {
                         config->ftfxConfig[i].flashDesc.protectRegionMem.size = segmentSize;
                    }
                }
            }
        }

        config->ftfxConfig[flashIndex].opsConfig.addrAligment.blockWriteUnitSize = pflashBlockWriteUnitSize;
        config->ftfxConfig[flashIndex].opsConfig.addrAligment.sectorCmd = pflashSectorCmdAlignment;
        config->ftfxConfig[flashIndex].opsConfig.addrAligment.sectionCmd = pflashSectionCmdAlignment;
        config->ftfxConfig[flashIndex].opsConfig.addrAligment.resourceCmd = FSL_FEATURE_FLASH_PFLASH_RESOURCE_CMD_ADDRESS_ALIGMENT;
        config->ftfxConfig[flashIndex].opsConfig.addrAligment.checkCmd = FSL_FEATURE_FLASH_PFLASH_CHECK_CMD_ADDRESS_ALIGMENT;
        config->ftfxConfig[flashIndex].opsConfig.addrAligment.swapCtrlCmd = FSL_FEATURE_FLASH_PFLASH_SWAP_CONTROL_CMD_ADDRESS_ALIGMENT;

        /* Init FTFx Kernel */
        returnCode = FTFx_API_Init(&config->ftfxConfig[flashIndex]);
        if (returnCode != kStatus_FTFx_Success)
        {
            return returnCode;
        }
    }

    return kStatus_FTFx_Success;
}
