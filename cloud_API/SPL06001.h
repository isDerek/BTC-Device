#ifndef __SPL06001_H__
#define __SPL06001_H__

#define PRESSURE_SENSOR     0
#define TEMPERATURE_SENSOR  1


#define SPL06_001_BANNER_1 ("7 Pressure Sens.")
#define SPL06_001_BANNER_2 ("Temp. : +XX.XX C")
#define SPL06_001_BANNER_3 ("P: XXXX.XX hPa")

/*
	const uint8_t content2[]={"Temp. :"};
	const uint8_t content3[]={"Degree C"};
	const uint8_t content4[]={"P:"};
	const uint8_t content5[]={"hPa"};
*/
struct spl0601_calib_param_t {
	int16_t c0;
	int16_t c1;
	int32_t c00;
	int32_t c10;
	int16_t c01;
	int16_t c11;
	int16_t c20;
	int16_t c21;
	int16_t c30;
};

struct spl0601_t {
    struct spl0601_calib_param_t calib_param;/**<calibration data*/
    uint8_t chip_id; /**<chip id*/
    int32_t i32rawPressure;
    int32_t i32rawTemperature;
    int32_t i32kP;
    int32_t i32kT;
};


extern uint16_t SPL06_001_init(void);

void spl0601_rateset(uint8_t iSensor, uint8_t u8OverSmpl, uint8_t u8SmplRate);
void spl0601_get_calib_param(void);
void spl0601_init(void);

void spl0601_get_raw_temp(void);
void spl0601_get_raw_pressure(void);


uint32_t spl0601_get_temperature(void);
uint32_t spl0601_get_pressure(void);

extern void SPL06_001_Value(void);

#endif


