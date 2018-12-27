#include "tools.h"
#include "stdint.h"

void delay_30ms(void)//30ms
{
    volatile uint32_t i = 0U;
    for (i = 0U; i < 320000U; ++i)
    {
        __asm("NOP"); /* delay */
    }
}

void delay_s(void)
{
	  volatile uint32_t i = 0U;
    for (i = 0U; i < 33U; i++)
    {
       delay_30ms(); /* delay */
    }
}

void delay_xs(int a)
{
	  volatile uint32_t i = 0U;
    for (i = 0U; i < 33U*a; i++)
    {
       delay_30ms(); /* delay */
    }	
}
