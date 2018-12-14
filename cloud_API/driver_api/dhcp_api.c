#include "lwip/opt.h"
#if LWIP_IPV4 && LWIP_DHCP && LWIP_NETCONN
#include "lwip/dhcp.h"
#include "lwip/ip_addr.h"
#include "lwip/sys.h"
#include "tcpecho/tcpecho.h"
#include "lwip/tcpip.h"
#include "netif/ethernet.h"
#include "ethernetif.h"

#include "K64_api.h"
#include "board.h"
/* MAC address configuration. *///be 45 4f 62 e7 20
#define configMAC_ADDR {0xbe, 0x45, 0x4f, 0x62, 0xe7, 0x20}
/* Address of PHY interface. */
#define EXAMPLE_PHY_ADDRESS BOARD_ENET0_PHY_ADDRESS
/* System clock name. */
#define EXAMPLE_CLOCK_NAME kCLOCK_CoreSysClk

#define serverIP_ADDR0 119
#define serverIP_ADDR1 23
#define serverIP_ADDR2 18
#define serverIP_ADDR3 135

static struct netif fsl_netif0;
ip4_addr_t fsl_netif0_ipaddr, fsl_netif0_netmask, fsl_netif0_gw;
ip_addr_t server_ipaddr;	

void Network_Init(void)
{
	MPU_Type *base = MPU;
	/* Disable MPU. */
	base->CESR &= ~MPU_CESR_VLD_MASK;
  ip4_addr_t fsl_netif0_ipaddr, fsl_netif0_netmask, fsl_netif0_gw;
  ethernetif_config_t fsl_enet_config0 = {
        .phyAddress = EXAMPLE_PHY_ADDRESS,
        .clockName = EXAMPLE_CLOCK_NAME,
        .macAddress = configMAC_ADDR,
  };
	IP4_ADDR(&fsl_netif0_ipaddr, 0U, 0U, 0U, 0U);
	IP4_ADDR(&fsl_netif0_netmask, 0U, 0U, 0U, 0U);
	IP4_ADDR(&fsl_netif0_gw, 0U, 0U, 0U, 0U);
	IP4_ADDR(&server_ipaddr, serverIP_ADDR0, serverIP_ADDR1, serverIP_ADDR2, serverIP_ADDR3);
	tcpip_init(NULL, NULL);

	netif_add(&fsl_netif0, &fsl_netif0_ipaddr, &fsl_netif0_netmask, &fsl_netif0_gw,
              &fsl_enet_config0, ethernetif0_init, tcpip_input);
	netif_set_default(&fsl_netif0);
  netif_set_up(&fsl_netif0);
	
	dhcp_start(&fsl_netif0);
}
#endif
