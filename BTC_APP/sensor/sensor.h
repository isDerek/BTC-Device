#include "stdint.h"

#ifndef __SPL06001_H__
#define __SPL06001_H__

#define PRESSURE_SENSOR     0
#define TEMPERATURE_SENSOR  1




extern uint16_t SPL06_001_init(void);

void spl0601_rateset(uint8_t iSensor, uint8_t u8OverSmpl, uint8_t u8SmplRate);
void spl0601_get_calib_param(void);
void spl0601_init(void);

void spl0601_get_raw_temp(void);
void spl0601_get_raw_pressure(void);


uint32_t spl0601_get_temperature(void);
uint32_t spl0601_get_pressure(void);

void SPL06_001_Value(void);
void OLED_ShowStr(unsigned char x,unsigned char y,unsigned char *chr);
void OLED_ShowChar(unsigned char x,unsigned char y,unsigned char chr);
void OLED_init(void);
void API_OLED_Clear(void);
void OLED_Set_Pos(unsigned char x, unsigned char y);
void OLED_Welcome(void);
void HDC1050_Init(void);
void HDC1050_Show(void);
void HDC1050_Run(void);
void RGB_Init(void);
void RGB_Show(void);
void RGB_Run(int red, int green, int blue);
void SPL06001_Init(void);
void SPL06001_Show(void);
void SPL06001_Run(void);
void VEML6030_Init(void);
void VEML6030_Show(void);
void VEML6030_Run(void);
void VEML6075_Init(void);
void VEML6075_Show(void);
void VEML6075_Run(void);
void ADXL345_Init(void);
void ADXL345_Show(void);
void ADXL345_Run(void);
void sensorInit(void);

void showUserDEF(void);
#endif
