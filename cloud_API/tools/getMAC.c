#include "fsl_sim.h"
#include "tools.h"
#include "stdlib.h"
char *getMAC(void)
{
	char *MAC_ADDR;
	MAC_ADDR = malloc(10);
	MAC_ADDR[0] = ((SIM->UIDH) >> 24)& 0xFF;
	MAC_ADDR[1] = ((SIM->UIDH) >> 16)& 0xFF;
	MAC_ADDR[2] = ((SIM->UIDL) >> 24)& 0xFF;
	MAC_ADDR[3] = ((SIM->UIDL) >> 16)& 0xFF;
	MAC_ADDR[4] = ((SIM->UIDL) >> 8)& 0xFF;
	MAC_ADDR[5] = ((SIM->UIDL) >> 0)& 0xFF;
//	printf("mac0 = %02x mac1 = %02x mac2 = %02x mac3 = %02x mac4 = %02x mac5 = %02x\n\r",MAC_ADDR[0],MAC_ADDR[1],MAC_ADDR[2],MAC_ADDR[3],MAC_ADDR[4],MAC_ADDR[5]);
//	printf("H = %x ,L = %x ,ML = %x ,MH = %x \n\r",SIM->UIDH,SIM->UIDL,SIM->UIDML,SIM->UIDMH);
	return MAC_ADDR;
}
