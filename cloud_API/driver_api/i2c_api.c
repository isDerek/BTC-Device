#include "fsl_i2c.h"
#include "K64_api.h"
#include "pin_mux.h"
#include "board.h"
/* I2C */
#define ACCEL_I2C_CLK_SRC I2C0_CLK_SRC
#define ACCEL_I2C_CLK_FREQ CLOCK_GetFreq(I2C0_CLK_SRC)
#define I2C_RELEASE_SDA_PORT PORTE
#define I2C_RELEASE_SCL_PORT PORTE
#define I2C_RELEASE_SDA_GPIO GPIOE
#define I2C_RELEASE_SDA_PIN 25U
#define I2C_RELEASE_SCL_GPIO GPIOE
#define I2C_RELEASE_SCL_PIN 24U
#define I2C_RELEASE_BUS_COUNT 100U
#define I2C_BAUDRATE 100000U
i2c_master_config_t masterConfig;
i2c_master_handle_t g_m_handle;
extern i2c_master_handle_t g_m_handle;
volatile bool completionFlag = false;
volatile bool nakFlag = false;

void i2c_init(void){
		
		uint32_t sourceClock = 0;
		BOARD_I2C_ReleaseBus();
		BOARD_I2C_ConfigurePins();	
    I2C_MasterTransferCreateHandle(BOARD_ACCEL_I2C_BASEADDR, &g_m_handle, i2c_master_callback, NULL);
    I2C_MasterGetDefaultConfig(&masterConfig);
    masterConfig.baudRate_Bps = I2C_BAUDRATE;
    sourceClock = CLOCK_GetFreq(ACCEL_I2C_CLK_SRC);
    I2C_MasterInit(BOARD_ACCEL_I2C_BASEADDR, &masterConfig, sourceClock);
}
