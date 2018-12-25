#include "fsl_gpio.h"
#include "board.h"
#include "fsl_port.h"
#include "K64_api.h"
#include "globalParams.h"

#define SENSORSW1PORT PORTC
#define SENSORSW1GPIO GPIOC
#define SENSORSW1PIN 3

#define SENSORSW2PORT PORTC
#define SENSORSW2GPIO GPIOC
#define SENSORSW2PIN 12

#define BOARD_SW1_IRQ PORTC_IRQn
#define BOARD_SW1_IRQ_HANDLER PORTC_IRQHandler

#define BOARD_SW2_IRQ PORTC_IRQn
#define BOARD_SW2_IRQ_HANDLER PORTC_IRQHandler

bool sw1PressBtn = false;
bool sw2PressBtn = false;
int sw1Status = 0;
int sw2Status = 0;
int swTimer = 0;
void BOARD_SW1_IRQ_HANDLER(void)
{
	uint32_t PortCMask;
	int sw1Button,sw2Button;
	PortCMask = GPIO_GetPinsInterruptFlags(SENSORSW1GPIO);
	sw1Button = PortCMask&(0x1<<SENSORSW1PIN);
	sw2Button = PortCMask&(0x1<<SENSORSW2PIN);
	printf("PortCMask = %x\n\r",PortCMask);
	GPIO_ClearPinsInterruptFlags(SENSORSW1GPIO, 1U << SENSORSW1PIN);
	GPIO_ClearPinsInterruptFlags(SENSORSW2GPIO, 1U << SENSORSW2PIN);
	if(sw1Button){
		sw1PressBtn = true;
		printf(" sw1 is pressed \r\n");
	}
	if(sw2Button){
		sw2PressBtn = true;
		printf(" sw2 is pressed \r\n");
	}
    /* Add for ARM errata 838869, affects Cortex-M4, Cortex-M4F Store immediate overlapping
      exception return operation might vector to incorrect interrupt */
#if defined __CORTEX_M && (__CORTEX_M == 4U)
    __DSB();
#endif
}

void gpio_init(void)
{
	gpio_pin_config_t sensor_sw_config = {
		kGPIO_DigitalInput, 0,
	};
    /* Init input switch GPIO. */
    PORT_SetPinInterruptConfig(SENSORSW1PORT, SENSORSW1PIN, kPORT_InterruptFallingEdge);
    EnableIRQ(BOARD_SW1_IRQ);
    GPIO_PinInit(SENSORSW1GPIO, SENSORSW1PIN, &sensor_sw_config);
	
    PORT_SetPinInterruptConfig(SENSORSW2PORT, SENSORSW2PIN, kPORT_InterruptFallingEdge);
    EnableIRQ(BOARD_SW2_IRQ);
    GPIO_PinInit(SENSORSW2GPIO, SENSORSW2PIN, &sensor_sw_config);
	

		LED_GREEN_INIT(1);

}

