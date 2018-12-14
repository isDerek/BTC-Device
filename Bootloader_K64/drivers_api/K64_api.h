#include "stdint.h"

void flash_init(void);
int program_flash(uint32_t start, uint32_t *src, uint32_t lengthInBytes);
int erase_sector(uint32_t start);
