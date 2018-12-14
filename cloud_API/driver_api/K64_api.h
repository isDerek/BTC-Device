#include "stdint.h"
#include "fsl_flash.h"
void i2c_init(void);

void gpio_init(void);

void ftm_init(void);

void FTM_updata(int updatedDutycycle);

void Network_Init(void);

void adc_init(void);

int getADCValue(void);

void flash_init(void);

int erase_sector(uint32_t start);

int program_flash(uint32_t start, uint32_t *src, uint32_t lengthInBytes);

void OTAInit(void);
