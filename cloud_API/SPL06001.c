#include "fsl_i2c.h"
#include "CharConvert.h"
/*
#include "qm_pwm.h"
#include "qm_scss.h"
#include "qm_interrupt.h"
*/
/*
const uint8_t SPL06_002=0x78;
The SPL06-002 device 7-bits address is 0x78*/

#include "SPL06001.h"
#include <stdio.h>
extern void delay_30ms(void);
extern _ARMABI int printf(const char * __restrict /*format*/, ...) __attribute__((__nonnull__(1)));
extern void OLED_ShowStr(unsigned char x,unsigned char y,unsigned char *chr);

const uint8_t SPL06_001 = 0x77;
/*The SPL06-001 device 7-bits address is 0x77*/

static struct spl0601_t spl0601;

static struct spl0601_t *p_spl0601;

int16_t c0, c1, c01, c11, c20, c21, c30;
int32_t c00, c10;
uint32_t spl0601_get_pressure(void)
{

	uint32_t fTsc, fPsc;
	int32_t qua2, qua3;
	uint32_t fPCompensate;

	int32_t arithmetic;

	fTsc = (p_spl0601->i32kT>>16);
  fTsc = p_spl0601->i32rawTemperature / (int32_t)fTsc;

	fPsc = (p_spl0601->i32kP>>16);
  fPsc = p_spl0601->i32rawPressure / (int32_t)fPsc;
/*
    qua2 = p_spl0601->calib_param.c10 + fPsc * (p_spl0601->calib_param.c20 + fPsc* p_spl0601->calib_param.c30);
*/
	arithmetic = c30;
  if(arithmetic<0)
	{
    arithmetic = (arithmetic>>16)|0xFFFF0000;
  }
	else
	{
    arithmetic = arithmetic>>16;
  }

  arithmetic = arithmetic+c20;

  arithmetic = fPsc*arithmetic;

  if(arithmetic<0)
	{
    arithmetic = (arithmetic>>16)|0xFFFF0000;
  }
	else
	{
    arithmetic = arithmetic>>16;
  }

  arithmetic = arithmetic+c10;

  arithmetic = arithmetic*fPsc;
  if(arithmetic<0)
	{
    arithmetic = (arithmetic>>16)|0xFFFF0000;
  }
	else
	{
    arithmetic = arithmetic>>16;
  }
  qua2 = arithmetic;

 /*
    qua3 = fTsc * fPsc * (p_spl0601->calib_param.c11 + fPsc * p_spl0601->calib_param.c21);
 */
	arithmetic = fPsc * c21;
	if(arithmetic<0)
	{
		arithmetic = (arithmetic>>16)|0xFFFF0000;
	}
	else
	{
		arithmetic = arithmetic>>16;
	}
	arithmetic = arithmetic+c11;
	arithmetic = arithmetic*fPsc;
	if(arithmetic<0)
	{
		arithmetic = (arithmetic>>16)|0xFFFF0000;
	}
	else
	{
		arithmetic = arithmetic>>16;
	}
	arithmetic = arithmetic*fTsc;
	if(arithmetic<0)
	{
		arithmetic = (arithmetic>>16)|0xFFFF0000;
	}
	else
	{
		arithmetic = arithmetic>>16;
	}
	qua3 = arithmetic;
/*
    fPCompensate = p_spl0601->calib_param.c00 + fPsc * qua2 + fTsc * p_spl0601->calib_param.c01 + qua3;
*/
	arithmetic = fTsc * c01;
	if(arithmetic<0)
	{
		arithmetic = (arithmetic>>16)|0xFFFF0000;
	}
	else
	{
		arithmetic = arithmetic>>16;
	}
	fPCompensate =  arithmetic + qua3 + c00 + qua2;
	return fPCompensate;
}

uint32_t spl0601_get_temperature(void)
{
	uint32_t fTCompensate;
	uint32_t fTsc;

	fTsc =  (p_spl0601->i32kT>>16);
	fTsc = p_spl0601->i32rawTemperature/fTsc;

/*  fTCompensate =  (p_spl0601->calib_param.c0 * 0.5) + (p_spl0601->calib_param.c1 * fTsc);

    fTCompensate =  (p_spl0601->calib_param.c0 >>1) + ((p_spl0601->calib_param.c1 * fTsc)>>16);*/

	fTCompensate = (c0<<15)+(c1 * fTsc);

	return  fTCompensate;
}

void spl0601_get_raw_pressure(void)
{

/*
    SoftI2C_start();
    SoftI2C_write_byte(HW_ADR << 1);
    SoftI2C_write_byte(0x00);
    SoftI2C_start();
    SoftI2C_write_byte((HW_ADR << 1)|0x01);
    h = SoftI2C_read_byte(0);
    m = SoftI2C_read_byte(0);
    l = SoftI2C_read_byte(1);
    SoftI2C_stop();
*/
		uint8_t h, m, l;
    uint8_t read_data[3];
    const uint8_t PRS_B2 = 0x00;
    /*PRS_B2 The Highest of the three bytes measured pressure value. address=0x00 */
		I2C_Readdata(I2C0, 0x77, PRS_B2, read_data, 3);

    h = read_data[0];
    m = read_data[1];
    l = read_data[2];


    p_spl0601->i32rawPressure = (int32_t)h<<16 | (int32_t)m<<8 | (int32_t)l;

/*    p_spl0601->i32rawPressure= (p_spl0601->i32rawPressure&0x800000) ? (0xFF000000|p_spl0601->i32rawPressure) : p_spl0601->i32rawPressure;*/

    if((p_spl0601->i32rawPressure&0x800000) == 0x800000)
		{
    	 p_spl0601->i32rawPressure = (0xFF000000|p_spl0601->i32rawPressure);
    }
/*Assignment to itself
    else{
		p_spl0601->i32rawPressure=p_spl0601->i32rawPressure;
    }
*/
}

void spl0601_get_raw_temp(void)
{
    uint8_t h, m, l;
/*
    SoftI2C_start();
    SoftI2C_write_byte(HW_ADR << 1);
    SoftI2C_write_byte(0x03);
    SoftI2C_start();
    SoftI2C_write_byte((HW_ADR << 1)|0x01);

    h = SoftI2C_read_byte(0);
    m = SoftI2C_read_byte(0);
    l = SoftI2C_read_byte(1);

    SoftI2C_stop();

  */
    uint8_t read_data[3];
    const uint8_t TMP_B2 = 0x03;
    /*TMP_B2 The Highest of the three bytes measured temperature value. address=0x03 */
//    write_data[0]=TMP_B2;
		I2C_Readdata(I2C0, 0x77, TMP_B2, read_data, 3);

    h = read_data[0];
    m = read_data[1];
    l = read_data[2];

    p_spl0601->i32rawTemperature = (int32_t)h<<16 | (int32_t)m<<8 | (int32_t)l;
/*p_spl0601->i32rawTemperature= (p_spl0601->i32rawTemperature&0x800000) ? (0xFF000000|p_spl0601->i32rawTemperature) : p_spl0601->i32rawTemperature;*/

    if((p_spl0601->i32rawTemperature&0x800000) == 0x800000)
		{
    	p_spl0601->i32rawTemperature = (0xFF000000|p_spl0601->i32rawTemperature);
    }
/*Assignment to itself
    else{
    	p_spl0601->i32rawTemperature=p_spl0601->i32rawTemperature;
    }
*/
}

void spl0601_rateset(uint8_t iSensor, uint8_t u8SmplRate, uint8_t u8OverSmpl)
{
    uint8_t reg = 0;
    int32_t i32kPkT = 0;
#if 0
    switch(u8SmplRate)
    {
        case 2:
            reg |= (1<<4);
            break;
        case 4:
            reg |= (2<<4);
            break;
        case 8:
            reg |= (3<<4);
            break;
        case 16:
            reg |= (4<<4);
            break;
        case 32:
            reg |= (5<<4);
            break;
        case 64:
            reg |= (6<<4);
            break;
        case 128:
            reg |= (7<<4);
            break;
        case 1:
        default:
            break;
    }
    switch(u8OverSmpl)
    {
        case 2:
            reg |= 1;
            i32kPkT = 1572864;
            break;
        case 4:
            reg |= 2;
            i32kPkT = 3670016;
            break;
        case 8:
            reg |= 3;
            i32kPkT = 7864320;
            break;
        case 16:
            i32kPkT = 253952;
            reg |= 4;
            break;
        case 32:
            i32kPkT = 516096;
            reg |= 5;
            break;
        case 64:
            i32kPkT = 1040384;
            reg |= 6;
            break;
        case 128:
            i32kPkT = 2088960;
            reg |= 7;
            break;
        case 1:
        default:
            i32kPkT = 524288;
            break;
    }
#endif
    reg |= (5<<4);
    reg |= 3;
    i32kPkT = 7864320;
    if(iSensor == 0)
    {
        p_spl0601->i32kP = i32kPkT;
				I2C_WriteByte(I2C0, 0x77, 0x06, reg);
        if(u8OverSmpl > 8)
        {
						I2C_Readdata(I2C0, 0x77, 0x09, &reg, 1);
						I2C_WriteByte(I2C0, 0x77, 0x09, reg | 0x04);
        }
    }

    if(iSensor == 1)
    {
        p_spl0601->i32kT = i32kPkT;
				//Using mems temperature
        reg=reg|0x80;
				I2C_WriteByte(I2C0, 0x77, 0x07, reg);
        if(u8OverSmpl > 8)
        {
						I2C_Readdata(I2C0, 0x77, 0x09, &reg, 1);
						I2C_WriteByte(I2C0, 0x77, 0x09, reg | 0x08);
        }
    }
}

void spl0601_init(void)
{
    p_spl0601 = &spl0601; /* read Chip Id */
    p_spl0601->i32rawPressure = 0;
    p_spl0601->i32rawTemperature = 0;
    p_spl0601->chip_id = 0x34;
    spl0601_get_calib_param();
    /* sampling rate = 1Hz; Pressure oversample = 2;*/
    spl0601_rateset(PRESSURE_SENSOR,32, 8);
    /* sampling rate = 1Hz; Temperature oversample = 1;*/
    spl0601_rateset(TEMPERATURE_SENSOR,32, 8);
    /*Start background measurement*/
}	

void spl0601_get_calib_param(void)
{
		uint8_t coef[18];
		I2C_Readdata(I2C0, 0x77, 0x10, coef, 18);
		c0 = (int16_t) ((coef[0]<<4) | (coef[1]>>4) );
    c0 = (c0&0x0800)?(0xF000|c0):c0;
    c1 = (int16_t)(coef[1]&0x0F)<<8 | coef[2];
    c1 = (c1&0x0800)?(0xF000|c1):c1;
    c00 = (int32_t)coef[3]<<12 | (int32_t)coef[4]<<4 | (int32_t)coef[5]>>4;
    if((c00&0x080000) == 0x080000)
		{
    	 c00 = (0xFFF00000|c00);
    }
    c10 = (int32_t)coef[5]<<16 | (int32_t)coef[6]<<8 | coef[7];
    if((c10&0x080000) == 0x080000)
		{
    	c10 = (0xFFF00000|c10);
    }
    c01 = (int16_t)coef[8]<<8 | coef[9];
    c11 = (int16_t)coef[10]<<8 | coef[11];
    c20 = (int16_t)coef[12]<<8 | coef[13];
    c21 = (int16_t)coef[14]<<8 | coef[15];
    c30 = (int16_t)coef[16]<<8 | coef[17];
//		p_spl0601->calib_param.c0 = c0;
//		p_spl0601->calib_param.c1 = c1;
//		p_spl0601->calib_param.c00 = c00;
//		p_spl0601->calib_param.c10 = c10;
//		p_spl0601->calib_param.c01 = c01;
//		p_spl0601->calib_param.c11 = c11;
//		p_spl0601->calib_param.c20 = c20;
//		p_spl0601->calib_param.c21 = c21;
//		p_spl0601->calib_param.c30 = c30;
}

uint16_t SPL06_001_init(void)
{
	uint16_t temp;

	const uint8_t MEAS_CFG = 0x08;
	/*Measurement configuration MEAS_CFG Address=0x08*/

	const uint8_t MEAS_CFG_value = 0x07;
	/* MEAS_CFG_value=0x07
		MEAS_CTRL[2:0]=111
		Background Mode:
		Continuous pressure and temperature measurement
	 *  */
	uint8_t data_write[2] = {MEAS_CFG,MEAS_CFG_value};

  spl0601_init();

    /*spl0601_start_continuous(uint8_t mode);
     *     spl0601_write(HW_ADR, 0x08, 0x07);*/
	temp = I2C_WriteByte(I2C0, 0x77, data_write[0], data_write[1]);

	delay_30ms();
	return  temp;

}
extern uint32_t persure;
void SPL06_001_Value(void)
{
	delay_30ms();
		
	char Pdata[16];
//	const uint16_t Second_Digit=0x028F;
//	uint8_t digit[6];
	spl0601_get_raw_temp();
	spl0601_get_raw_pressure();

	persure = spl0601_get_temperature();


  if(persure<0x80000000)
	{
		printf(" ");	
	}
	else
	{
		printf("-");		
		persure = 0xFFFFFFFF-persure+1;
	}
//	for(i=0;i<5;i++)
//	{
//		digit[i] = CharConvert(persure,Second_Digit,i);
//	}

//	for(i = 4; i>2; )
//	{
//		if(digit[i] == '0')
//		{
//			digit[i] = ' ';
//			i--;
//		}
//		else
//		{
//			break;
//		}
//	}

//	printf("%d%d.%d", digit[3], digit[2], digit[1]);
//#if 0
//	printf("%d", digit[0]);
//#endif
//	printf("C\r\n");
//	printf("temp = %d\r\n", persure);

	persure = spl0601_get_pressure();

//	for(i = 0; i<6; i++)
//	{
//		digit[i] = CharConvert(persure, 1, i);
//	}

//	for(i = 5; i>2; )
//	{
//		if(digit[i] == '0')
//		{
//			digit[i] = ' ';
//			i--;
//		}
//		else
//		{
//			break;
//		}
//	}

//	printf(", ");

//	for(i = 5; i>1; i--)
//	{
//		printf("%d", digit[i]);
//	}

//#if 0
//	printf(".%d%d\r\n", digit[1], digit[0]);
//#endif
//	printf("%d\r\n", digit[1], (uint8_t*)" hPa");
	printf("pressure = %d\r\n", persure);
	sprintf(Pdata, "%6.0f", (float)persure);
	OLED_ShowStr(24,3,(uint8_t*)Pdata);
}



