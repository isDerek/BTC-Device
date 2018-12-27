/*
 * Copyright (c) 2001-2004 Swedish Institute of Computer Science.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT
 * SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT
 * OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY
 * OF SUCH DAMAGE.
 *
 * This file is part of the lwIP TCP/IP stack.
 *
 * Author: Adam Dunkels <adam@sics.se>
 *
 */


#include "lwip/opt.h"
#include "lwip/def.h"
#include "lwip/mem.h"
#include "lwip/pbuf.h"
#include "lwip/stats.h"
#include "lwip/snmp.h"
#include "lwip/ethip6.h"
#include "netif/etharp.h"
#include "netif/ppp/pppoe.h"
#include "lwip/igmp.h"
#include "lwip/mld6.h"

#if USE_RTOS && defined(FSL_RTOS_FREE_RTOS)
#include "FreeRTOS.h"
#include "event_groups.h"
#endif

#include "ethernetif.h"

#include "fsl_enet.h"
#include "fsl_phy.h"

/*******************************************************************************
 * Definitions
 ******************************************************************************/
/* Define those to better describe your network interface. */
#define IFNAME0 'e'
#define IFNAME1 'n'

#define ENET_ALIGN(x) ((unsigned int)((x) + ((ENET_BUFF_ALIGNMENT)-1)) & (unsigned int)(~(unsigned int)((ENET_BUFF_ALIGNMENT)-1)))

#if defined(FSL_SDK_ENABLE_DRIVER_CACHE_CONTROL) && FSL_SDK_ENABLE_DRIVER_CACHE_CONTROL
    #if defined(FSL_FEATURE_L2CACHE_LINESIZE_BYTE) \
        && ((!defined(FSL_SDK_DISBLE_L2CACHE_PRESENT)) || (FSL_SDK_DISBLE_L2CACHE_PRESENT == 0))
        #if defined(FSL_FEATURE_L1DCACHE_LINESIZE_BYTE)
            #define FSL_CACHE_LINESIZE_MAX MAX(FSL_FEATURE_L1DCACHE_LINESIZE_BYTE, FSL_FEATURE_L2CACHE_LINESIZE_BYTE)
            #define FSL_ENET_BUFF_ALIGNMENT MAX(ENET_BUFF_ALIGNMENT, FSL_CACHE_LINESIZE_MAX)
        #else
            #define FSL_ENET_BUFF_ALIGNMENT MAX(ENET_BUFF_ALIGNMENT, FSL_FEATURE_L2CACHE_LINESIZE_BYTE)
        #endif
    #elif defined(FSL_FEATURE_L1DCACHE_LINESIZE_BYTE)
        #define FSL_ENET_BUFF_ALIGNMENT MAX(ENET_BUFF_ALIGNMENT, FSL_FEATURE_L1DCACHE_LINESIZE_BYTE)
    #else
        #define FSL_ENET_BUFF_ALIGNMENT ENET_BUFF_ALIGNMENT
    #endif
#else
    #define FSL_ENET_BUFF_ALIGNMENT ENET_BUFF_ALIGNMENT
#endif

#if defined(FSL_FEATURE_SOC_LPC_ENET_COUNT) && (FSL_FEATURE_SOC_LPC_ENET_COUNT > 0)
#define kENET_RxEvent kENET_RxIntEvent
#define kENET_TxEvent kENET_TxIntEvent
#endif

#define ENET_RING_NUM 1U

typedef uint8_t rx_buffer_t[SDK_SIZEALIGN(ENET_RXBUFF_SIZE, FSL_ENET_BUFF_ALIGNMENT)];
typedef uint8_t tx_buffer_t[SDK_SIZEALIGN(ENET_TXBUFF_SIZE, FSL_ENET_BUFF_ALIGNMENT)];

/**
 * Helper struct to hold private data used to operate your ethernet interface.
 */
struct ethernetif
{
    ENET_Type *base;
#if (defined(FSL_FEATURE_SOC_ENET_COUNT) && (FSL_FEATURE_SOC_ENET_COUNT > 0)) || \
    (USE_RTOS && defined(FSL_RTOS_FREE_RTOS))
    enet_handle_t handle;
#endif
#if USE_RTOS && defined(FSL_RTOS_FREE_RTOS)
    EventGroupHandle_t enetTransmitAccessEvent;
    EventBits_t txFlag;
#endif
    enet_rx_bd_struct_t *RxBuffDescrip;
    enet_tx_bd_struct_t *TxBuffDescrip;
    rx_buffer_t *RxDataBuff;
    tx_buffer_t *TxDataBuff;
#if defined(FSL_FEATURE_SOC_LPC_ENET_COUNT) && (FSL_FEATURE_SOC_LPC_ENET_COUNT > 0)
    uint8_t txIdx;
#if !(USE_RTOS && defined(FSL_RTOS_FREE_RTOS))
    uint8_t rxIdx;
#endif
#endif
};

/*******************************************************************************
 * Code
 ******************************************************************************/
#if USE_RTOS && defined(FSL_RTOS_FREE_RTOS)
void ethernet_callback(ENET_Type *base, enet_handle_t *handle, enet_event_t event, void *param)
{
    struct netif *netif = (struct netif *)param;
    struct ethernetif *ethernetif = netif->state;

    switch (event)
    {
        case kENET_RxEvent:
            ethernetif_input(netif);
            break;
        case kENET_TxEvent:
        {
            portBASE_TYPE taskToWake = pdFALSE;

            if (__get_IPSR())
            {
                xEventGroupSetBitsFromISR(ethernetif->enetTransmitAccessEvent, ethernetif->txFlag, &taskToWake);
                if (pdTRUE == taskToWake)
                {
                    taskYIELD();
                }
            }
            else
            {
                xEventGroupSetBits(ethernetif->enetTransmitAccessEvent, ethernetif->txFlag);
            }
        }
        break;
        default:
            break;
    }
}
#endif

#if LWIP_IPV4 && LWIP_IGMP
static err_t ethernetif_igmp_mac_filter(struct netif *netif, const ip4_addr_t *group, u8_t action) {
  struct ethernetif *ethernetif = netif->state;
  uint8_t multicastMacAddr[6];
  err_t result;  

  multicastMacAddr[0] = 0x01U;
  multicastMacAddr[1] = 0x00U;
  multicastMacAddr[2] = 0x5EU;
  multicastMacAddr[3] = (group->addr>>8) & 0x7FU;
  multicastMacAddr[4] = (group->addr>>16)& 0xFFU;
  multicastMacAddr[5] = (group->addr>>24)& 0xFFU;

  switch (action) {
    case IGMP_ADD_MAC_FILTER: 
      /* Adds the ENET device to a multicast group.*/
      ENET_AddMulticastGroup(ethernetif->base, multicastMacAddr);
      result = ERR_OK;
      break;
    case IGMP_DEL_MAC_FILTER:
      /* Moves the ENET device from a multicast group.*/
    #if 0
      ENET_LeaveMulticastGroup(ethernetif->base, multicastMacAddr);
    #endif
      result = ERR_OK;
      break;
    default:
      result = ERR_IF;
      break;
  }

  return result;
}
#endif

#if LWIP_IPV6 && LWIP_IPV6_MLD
static err_t ethernetif_mld_mac_filter(struct netif *netif, const ip6_addr_t *group, u8_t action) {
  struct ethernetif *ethernetif = netif->state;
  uint8_t multicastMacAddr[6];
  err_t result;  

  multicastMacAddr[0] = 0x33U;
  multicastMacAddr[1] = 0x33U;
  multicastMacAddr[2] = (group->addr[3]) & 0xFFU;
  multicastMacAddr[3] = (group->addr[3]>>8) & 0xFFU;
  multicastMacAddr[4] = (group->addr[3]>>16)& 0xFFU;
  multicastMacAddr[5] = (group->addr[3]>>24)& 0xFFU;

  switch (action) {
    case MLD6_ADD_MAC_FILTER: 
      /* Adds the ENET device to a multicast group.*/
      ENET_AddMulticastGroup(ethernetif->base, multicastMacAddr);
      result = ERR_OK;
      break;
    case MLD6_DEL_MAC_FILTER:
      /* Moves the ENET device from a multicast group.*/
    #if 0
      ENET_LeaveMulticastGroup(ethernetif->base, multicastMacAddr);
    #endif
      result = ERR_OK;
      break;
    default:
      result = ERR_IF;
      break;
  }

  return result;
}
#endif

/**
 * Initializes ENET driver.
 */
#if defined(FSL_FEATURE_SOC_ENET_COUNT) && (FSL_FEATURE_SOC_ENET_COUNT > 0)
static void enet_init(struct netif *netif, struct ethernetif *ethernetif,
                      const ethernetif_config_t *ethernetifConfig)
{
    enet_config_t config;
    uint32_t sysClock;
    status_t status;
    bool link = false;
    phy_speed_t speed;
    phy_duplex_t duplex;
    uint32_t count = 0;
    enet_buffer_config_t buffCfg[ENET_RING_NUM];

    /* prepare the buffer configuration. */
    buffCfg[0].rxBdNumber = ENET_RXBD_NUM;                      /* Receive buffer descriptor number. */
    buffCfg[0].txBdNumber = ENET_TXBD_NUM;                      /* Transmit buffer descriptor number. */
    buffCfg[0].rxBuffSizeAlign = sizeof(rx_buffer_t);           /* Aligned receive data buffer size. */
    buffCfg[0].txBuffSizeAlign = sizeof(tx_buffer_t);           /* Aligned transmit data buffer size. */
    buffCfg[0].rxBdStartAddrAlign = &(ethernetif->RxBuffDescrip[0]); /* Aligned receive buffer descriptor start address. */
    buffCfg[0].txBdStartAddrAlign = &(ethernetif->TxBuffDescrip[0]); /* Aligned transmit buffer descriptor start address. */
    buffCfg[0].rxBufferAlign = &(ethernetif->RxDataBuff[0][0]); /* Receive data buffer start address. */
    buffCfg[0].txBufferAlign = &(ethernetif->TxDataBuff[0][0]); /* Transmit data buffer start address. */

    sysClock = CLOCK_GetFreq(ethernetifConfig->clockName);

    ENET_GetDefaultConfig(&config);
    config.ringNum = ENET_RING_NUM;

    status = PHY_Init(ethernetif->base, ethernetifConfig->phyAddress, sysClock);
    if (kStatus_Success != status)
    {
        LWIP_ASSERT("\r\nCannot initialize PHY.\r\n", 0);
    }

    while ((count < ENET_ATONEGOTIATION_TIMEOUT) && (!link))
    {
        PHY_GetLinkStatus(ethernetif->base, ethernetifConfig->phyAddress, &link);

        if (link)
        {
            /* Get the actual PHY link speed. */
            PHY_GetLinkSpeedDuplex(ethernetif->base, ethernetifConfig->phyAddress, &speed, &duplex);
            /* Change the MII speed and duplex for actual link status. */
            config.miiSpeed = (enet_mii_speed_t)speed;
            config.miiDuplex = (enet_mii_duplex_t)duplex;
        }

        count++;
    }

#if 0 /* Disable assert. If initial auto-negation is timeout, \ \
        the ENET set to default 100Mbs and full-duplex.*/
    if (count == ENET_ATONEGOTIATION_TIMEOUT)
    {
        LWIP_ASSERT("\r\nPHY Link down, please check the cable connection.\r\n", 0);
    }
#endif

#if USE_RTOS && defined(FSL_RTOS_FREE_RTOS)
    uint32_t instance;
    static ENET_Type *const enetBases[] = ENET_BASE_PTRS;
    static const IRQn_Type enetTxIrqId[] = ENET_Transmit_IRQS;
    /*! @brief Pointers to enet receive IRQ number for each instance. */
    static const IRQn_Type enetRxIrqId[] = ENET_Receive_IRQS;
#if defined(ENET_ENHANCEDBUFFERDESCRIPTOR_MODE) && ENET_ENHANCEDBUFFERDESCRIPTOR_MODE
    /*! @brief Pointers to enet timestamp IRQ number for each instance. */
    static const IRQn_Type enetTsIrqId[] = ENET_1588_Timer_IRQS;
#endif /* ENET_ENHANCEDBUFFERDESCRIPTOR_MODE */

    /* Create the Event for transmit busy release trigger. */
    ethernetif->enetTransmitAccessEvent = xEventGroupCreate();
    ethernetif->txFlag = 0x1;

    config.interrupt |= kENET_RxFrameInterrupt | kENET_TxFrameInterrupt | kENET_TxBufferInterrupt;

    for (instance = 0; instance < ARRAY_SIZE(enetBases); instance++)
    {
        if (enetBases[instance] == ethernetif->base)
        {
#ifdef __CA7_REV
            GIC_SetPriority(enetRxIrqId[instance], ENET_PRIORITY);
            GIC_SetPriority(enetTxIrqId[instance], ENET_PRIORITY);
#if defined(ENET_ENHANCEDBUFFERDESCRIPTOR_MODE) && ENET_ENHANCEDBUFFERDESCRIPTOR_MODE
            GIC_SetPriority(enetTsIrqId[instance], ENET_1588_PRIORITY);
#endif /* ENET_ENHANCEDBUFFERDESCRIPTOR_MODE */
#else
            NVIC_SetPriority(enetRxIrqId[instance], ENET_PRIORITY);
            NVIC_SetPriority(enetTxIrqId[instance], ENET_PRIORITY);
#if defined(ENET_ENHANCEDBUFFERDESCRIPTOR_MODE) && ENET_ENHANCEDBUFFERDESCRIPTOR_MODE
            NVIC_SetPriority(enetTsIrqId[instance], ENET_1588_PRIORITY);
#endif /* ENET_ENHANCEDBUFFERDESCRIPTOR_MODE */
#endif /* __CA7_REV */
            break;
        }
    }

    LWIP_ASSERT("Input Ethernet base error!", (instance != ARRAY_SIZE(enetBases)));
#endif /* USE_RTOS */

    /* Initialize the ENET module.*/
    ENET_Init(ethernetif->base, &ethernetif->handle, &config, &buffCfg[0], netif->hwaddr, sysClock);

#if USE_RTOS && defined(FSL_RTOS_FREE_RTOS)
    ENET_SetCallback(&ethernetif->handle, ethernet_callback, netif);
#endif

    ENET_ActiveRead(ethernetif->base);
}
#elif defined(FSL_FEATURE_SOC_LPC_ENET_COUNT) && (FSL_FEATURE_SOC_LPC_ENET_COUNT > 0)
static void enet_init(struct netif *netif, struct ethernetif *ethernetif,
                      const ethernetif_config_t *ethernetifConfig)
{
    enet_config_t config;
    status_t status;
    uint32_t sysClock;
    bool link = false;
    phy_speed_t speed;
    phy_duplex_t duplex;
    uint32_t count = 0;
    enet_buffer_config_t buffCfg[ENET_RING_NUM];
    uint32_t rxBufferStartAddr[ENET_RXBD_NUM];
    uint32_t i;

    /* calculate start addresses of all rx buffers */
    for (i = 0; i < ENET_RXBD_NUM; i++)
    {
        rxBufferStartAddr[i] = (uint32_t)&(ethernetif->RxDataBuff[i][0]);
    }

    /* prepare the buffer configuration. */
    buffCfg[0].rxRingLen = ENET_RXBD_NUM;                          /* The length of receive buffer descriptor ring. */
    buffCfg[0].txRingLen = ENET_TXBD_NUM;                          /* The length of transmit buffer descriptor ring. */
    buffCfg[0].txDescStartAddrAlign = get_tx_desc(ethernetif, 0U); /* Aligned transmit descriptor start address. */
    buffCfg[0].txDescTailAddrAlign = get_tx_desc(ethernetif, 0U);  /* Aligned transmit descriptor tail address. */
    buffCfg[0].rxDescStartAddrAlign = get_rx_desc(ethernetif, 0U); /* Aligned receive descriptor start address. */
    buffCfg[0].rxDescTailAddrAlign = get_rx_desc(ethernetif, ENET_RXBD_NUM); /* Aligned receive descriptor tail address. */
    buffCfg[0].rxBufferStartAddr = rxBufferStartAddr;              /* Start addresses of the rx buffers. */
    buffCfg[0].rxBuffSizeAlign = sizeof(rx_buffer_t);              /* Aligned receive data buffer size. */

    sysClock = CLOCK_GetFreq(ethernetifConfig->clockName);

    ENET_GetDefaultConfig(&config);
    config.multiqueueCfg = NULL;

    status = PHY_Init(ethernetif->base, ethernetifConfig->phyAddress, sysClock);
    if (kStatus_Success != status)
    {
        LWIP_ASSERT("\r\nCannot initialize PHY.\r\n", 0);
    }

    while ((count < ENET_ATONEGOTIATION_TIMEOUT) && (!link))
    {
        PHY_GetLinkStatus(ethernetif->base, ethernetifConfig->phyAddress, &link);
        if (link)
        {
            /* Get the actual PHY link speed. */
            PHY_GetLinkSpeedDuplex(ethernetif->base, ethernetifConfig->phyAddress, &speed, &duplex);
            /* Change the MII speed and duplex for actual link status. */
            config.miiSpeed = (enet_mii_speed_t)speed;
            config.miiDuplex = (enet_mii_duplex_t)duplex;
        }

        count++;
    }

#if 0 /* Disable assert. If initial auto-negation is timeout, \ \
        the ENET set to default 100Mbs and full-duplex.*/
    if (count == ENET_ATONEGOTIATION_TIMEOUT)
    {
        LWIP_ASSERT("\r\nPHY Link down, please check the cable connection.\r\n", 0);
    }
#endif

#if USE_RTOS && defined(FSL_RTOS_FREE_RTOS)
    /* Create the Event for transmit busy release trigger. */
    ethernetif->enetTransmitAccessEvent = xEventGroupCreate();
    ethernetif->txFlag = 0x1;

    NVIC_SetPriority(ETHERNET_IRQn, ENET_PRIORITY);
#else
    ethernetif->rxIdx = 0U;
#endif /* USE_RTOS */

    ethernetif->txIdx = 0U;

    ENET_Init(ethernetif->base, &config, netif->hwaddr, sysClock);

#if defined(LPC54018_SERIES)
    /* Workaround for receive issue on lpc54018 */
    ethernetif->base->MAC_FRAME_FILTER |= ENET_MAC_FRAME_FILTER_RA_MASK;
#endif

/* Create the handler. */
#if USE_RTOS && defined(FSL_RTOS_FREE_RTOS)
    ENET_EnableInterrupts(ethernetif->base, kENET_DmaTx | kENET_DmaRx);
    ENET_CreateHandler(ethernetif->base, &ethernetif->handle, &config, &buffCfg[0], ethernet_callback, netif);
#endif

    ENET_DescriptorInit(ethernetif->base, &config, &buffCfg[0]);

    /* Active TX/RX. */
    ENET_StartRxTx(ethernetif->base, 1, 1);
}
#endif /* FSL_FEATURE_SOC_*_ENET_COUNT */

/**
 * In this function, the hardware should be initialized.
 * Called from ethernetif_init().
 *
 * @param netif the already initialized lwip network interface structure
 *        for this ethernetif
 */
static void low_level_init(struct netif *netif, const uint8_t enetIdx,
                           const ethernetif_config_t *ethernetifConfig)
{
    struct ethernetif *ethernetif = netif->state;

    /* set MAC hardware address length */
    netif->hwaddr_len = ETH_HWADDR_LEN;

    /* set MAC hardware address */
    memcpy(netif->hwaddr, ethernetifConfig->macAddress, NETIF_MAX_HWADDR_LEN);
	
    /* maximum transfer unit */
    netif->mtu = 1500; /* TODO: define a config */

    /* device capabilities */
    /* don't set NETIF_FLAG_ETHARP if this device is not an ethernet one */
    netif->flags |= NETIF_FLAG_BROADCAST | NETIF_FLAG_ETHARP | NETIF_FLAG_LINK_UP;

    /* ENET driver initialization.*/
    enet_init(netif, ethernetif, ethernetifConfig);

#if LWIP_IPV6 && LWIP_IPV6_MLD
    /*
     * For hardware/netifs that implement MAC filtering.
     * All-nodes link-local is handled by default, so we must let the hardware know
     * to allow multicast packets in.
     * Should set mld_mac_filter previously. */
    if (netif->mld_mac_filter != NULL)
    {
        ip6_addr_t ip6_allnodes_ll;
        ip6_addr_set_allnodes_linklocal(&ip6_allnodes_ll);
        netif->mld_mac_filter(netif, &ip6_allnodes_ll, NETIF_ADD_MAC_FILTER);
    }
#endif /* LWIP_IPV6 && LWIP_IPV6_MLD */
}
/**
 * This function should do the actual transmission of the packet. The packet is
 * contained in the pbuf that is passed to the function. This pbuf
 * might be chained.
 *
 * @param netif the lwip network interface structure for this ethernetif
 * @param p the MAC packet to send (e.g. IP packet including MAC addresses and type)
 * @return ERR_OK if the packet could be sent
 *         an err_t value if the packet couldn't be sent
 *
 * @note Returning ERR_MEM here if a DMA queue of your MAC is full can lead to
 *       strange results. You might consider waiting for space in the DMA queue
 *       to become available since the stack doesn't retry to send a packet
 *       dropped because of memory failure (except for the TCP timers).
 */

static err_t
low_level_output(struct netif *netif, struct pbuf *p)
{
  struct ethernetif *ethernetif = netif->state;
  struct pbuf *q;
  static unsigned char ucBuffer[ENET_FRAME_MAX_FRAMELEN];
  unsigned char *pucBuffer = ucBuffer;
  unsigned char *pucChar;

  LWIP_ASSERT("Output packet buffer empty", p);

  /* Initiate transfer. */
  
#if ETH_PAD_SIZE
  pbuf_header(p, -ETH_PAD_SIZE); /* drop the padding word */
#endif
  
  if (p->len == p->tot_len)
  {
    /* No pbuf chain, don't have to copy -> faster. */
    pucBuffer = (unsigned char *)p->payload;
  }
  else
  {
    /* pbuf chain, copy into contiguous ucBuffer. */
    if (p->tot_len >= ENET_FRAME_MAX_FRAMELEN)
    {
      return ERR_BUF;
    }
    else
    {
      pucChar = ucBuffer;

      for (q = p; q != NULL; q = q->next)
      {
        /* Send the data from the pbuf to the interface, one pbuf at a
        time. The size of the data in each pbuf is kept in the ->len
        variable. */
        /* send data from(q->payload, q->len); */
        if (q == p)
        {
          memcpy(pucChar, q->payload, q->len);
          pucChar += q->len;
        }
        else
        {
          memcpy(pucChar, q->payload, q->len);
          pucChar += q->len;
        }
      }
    }
  }

  /* Send frame. */
#if USE_RTOS && defined(FSL_RTOS_FREE_RTOS)
  {
    status_t result;

    do
    {
      result = ENET_SendFrame(ethernetif->base, &ethernetif->handle, pucBuffer, p->tot_len);

      if (result == kStatus_ENET_TxFrameBusy){
            xEventGroupWaitBits(ethernetif->enetTransmitAccessEvent, ethernetif->txFlag, pdTRUE, (BaseType_t) false, portMAX_DELAY);
      }

    } while (result == kStatus_ENET_TxFrameBusy);
  }
#else
  {
    uint32_t counter;

    for (counter = ENET_TIMEOUT; counter != 0U; counter--){
      if (ENET_SendFrame(ethernetif->base, &ethernetif->handle, pucBuffer, p->tot_len) != kStatus_ENET_TxFrameBusy){
            break;
      }
    }
  }
#endif

  MIB2_STATS_NETIF_ADD(netif, ifoutoctets, p->tot_len);
  if (((u8_t*)p->payload)[0] & 1) {
    /* broadcast or multicast packet*/
    MIB2_STATS_NETIF_INC(netif, ifoutnucastpkts);
  } else {
    /* unicast packet */
    MIB2_STATS_NETIF_INC(netif, ifoutucastpkts);
  }
  /* increase ifoutdiscards or ifouterrors on error */

#if ETH_PAD_SIZE
  pbuf_header(p, ETH_PAD_SIZE); /* reclaim the padding word */
#endif

  LINK_STATS_INC(link.xmit);

  return ERR_OK;
}

/**
 * Should allocate a pbuf and transfer the bytes of the incoming
 * packet from the interface into the pbuf.
 *
 * @param netif the lwip network interface structure for this ethernetif
 * @return a pbuf filled with the received packet (including MAC header)
 *         NULL on memory error
 */
static struct pbuf *
low_level_input(struct netif *netif)
{
  struct ethernetif *ethernetif = netif->state;
  struct pbuf *p = NULL;
  uint32_t len;
  status_t status;

  /* Obtain the size of the packet and put it into the "len"
     variable. */
  status = ENET_GetRxFrameSize(&ethernetif->handle, &len); 

  if(kStatus_ENET_RxFrameEmpty != status)
  {
    /* Call ENET_ReadFrame when there is a received frame. */
    if (len != 0)
    {

    #if ETH_PAD_SIZE
      len += ETH_PAD_SIZE; /* allow room for Ethernet padding */
    #endif

      /* We allocate a pbuf chain of pbufs from the pool. */
      p = pbuf_alloc(PBUF_RAW, len, PBUF_POOL);

      if (p != NULL) {

      #if ETH_PAD_SIZE
        pbuf_header(p, -ETH_PAD_SIZE); /* drop the padding word */
      #endif

        ENET_ReadFrame(ethernetif->base, &ethernetif->handle, p->payload, p->len);


      MIB2_STATS_NETIF_ADD(netif, ifinoctets, p->tot_len);
      if (((u8_t*)p->payload)[0] & 1) {
        /* broadcast or multicast packet*/
        MIB2_STATS_NETIF_INC(netif, ifinnucastpkts);
      } else {
        /* unicast packet*/
        MIB2_STATS_NETIF_INC(netif, ifinucastpkts);
      }
      #if ETH_PAD_SIZE
        pbuf_header(p, ETH_PAD_SIZE); /* reclaim the padding word */
      #endif

        LINK_STATS_INC(link.recv);
      } else {
        /* drop packet*/
        ENET_ReadFrame(ethernetif->base, &ethernetif->handle, NULL, 0);
        LWIP_DEBUGF(NETIF_DEBUG, ("ethernetif_input: Fail to allocate new memory space\n"));

        LINK_STATS_INC(link.memerr);
        LINK_STATS_INC(link.drop);
        MIB2_STATS_NETIF_INC(netif, ifindiscards);
      }
    }
    else
    {
      /* Update the received buffer when error happened. */
      if (status == kStatus_ENET_RxFrameError)
      {
      #if 0 /* Error statisctics */
        enet_data_error_stats_t eErrStatic;
        /* Get the error information of the received g_frame. */
        ENET_GetRxErrBeforeReadFrame(&ethernetif->handle, &eErrStatic);
      #endif

        /* Update the receive buffer. */
        ENET_ReadFrame(ethernetif->base, &ethernetif->handle, NULL, 0);
        LWIP_DEBUGF(NETIF_DEBUG, ("ethernetif_input: RxFrameError\n"));

        LINK_STATS_INC(link.drop);
        MIB2_STATS_NETIF_INC(netif, ifindiscards);
      }
    }

  }
  return p;
}

/**
 * This function should be called when a packet is ready to be read
 * from the interface. It uses the function low_level_input() that
 * should handle the actual reception of bytes from the network
 * interface. Then the type of the received packet is determined and
 * the appropriate input function is called.
 *
 * @param netif the lwip network interface structure for this ethernetif
 */
void
ethernetif_input(struct netif *netif)
{
  struct pbuf *p;

  LWIP_ASSERT("netif != NULL", (netif != NULL));

  /* move received packet into a new pbuf */
  while((p = low_level_input(netif)) != NULL)
  {
    /* pass all packets to ethernet_input, which decides what packets it supports */
    if (netif->input(p, netif) != ERR_OK) { 
      LWIP_DEBUGF(NETIF_DEBUG, ("ethernetif_input: IP input error\n"));
      pbuf_free(p);
      p = NULL;
    }
  }
}

static ENET_Type *get_enet_base(const uint8_t enetIdx)
{
    ENET_Type* enets[] = ENET_BASE_PTRS;
    int arrayIdx;
    int enetCount;

    for (arrayIdx = 0, enetCount = 0; arrayIdx < ARRAY_SIZE(enets); arrayIdx++)
    {
        if (enets[arrayIdx] != 0U)    /* process only defined positions */
        {                             /* (some SOC headers count ENETs from 1 instead of 0) */
            if (enetCount == enetIdx)
            {
                return enets[arrayIdx];
            }
            enetCount++;
        }
    }

    return NULL;
}

/**
 * Should be called at the beginning of the program to set up the
 * network interface. It calls the function low_level_init() to do the
 * actual setup of the hardware.
 *
 * This function should be passed as a parameter to netif_add().
 *
 * @param netif the lwip network interface structure for this ethernetif
 * @return ERR_OK if the loopif is initialized
 *         ERR_MEM if private data couldn't be allocated
 *         any other err_t on error
 */
static err_t ethernetif_init(struct netif *netif, struct ethernetif *ethernetif,
                             const uint8_t enetIdx,
                             const ethernetif_config_t *ethernetifConfig)
{
    LWIP_ASSERT("netif != NULL", (netif != NULL));
    LWIP_ASSERT("ethernetifConfig != NULL", (ethernetifConfig != NULL));

#if LWIP_NETIF_HOSTNAME
    /* Initialize interface hostname */
    netif->hostname = "lwip";
#endif /* LWIP_NETIF_HOSTNAME */

    /*
     * Initialize the snmp variables and counters inside the struct netif.
     * The last argument should be replaced with your link speed, in units
     * of bits per second.
     */
    MIB2_INIT_NETIF(netif, snmp_ifType_ethernet_csmacd, LINK_SPEED_OF_YOUR_NETIF_IN_BPS);

    netif->state = ethernetif;
    netif->name[0] = IFNAME0;
    netif->name[1] = IFNAME1;
/* We directly use etharp_output() here to save a function call.
 * You can instead declare your own function an call etharp_output()
 * from it if you have to do some checks before sending (e.g. if link
 * is available...) */
#if LWIP_IPV4
    netif->output = etharp_output;
#endif
#if LWIP_IPV6
    netif->output_ip6 = ethip6_output;
#endif /* LWIP_IPV6 */
    netif->linkoutput = low_level_output;

#if LWIP_IPV4 && LWIP_IGMP
    netif_set_igmp_mac_filter(netif, ethernetif_igmp_mac_filter);
    netif->flags |= NETIF_FLAG_IGMP;
#endif
#if LWIP_IPV6 && LWIP_IPV6_MLD
    netif_set_mld_mac_filter(netif, ethernetif_mld_mac_filter);
    netif->flags |= NETIF_FLAG_MLD6;
#endif

    /* Init ethernetif parameters.*/
    ethernetif->base = get_enet_base(enetIdx);
    LWIP_ASSERT("ethernetif->base != NULL", (ethernetif->base != NULL));

    /* initialize the hardware */
    low_level_init(netif, enetIdx, ethernetifConfig);

    return ERR_OK;
}


/**
 * Should be called at the beginning of the program to set up the
 * first network interface. It calls the function low_level_init() to do the
 * actual setup of the hardware.
 *
 * This function should be passed as a parameter to netif_add().
 *
 * @param netif the lwip network interface structure for this ethernetif
 * @return ERR_OK if the loopif is initialized
 *         ERR_MEM if private data couldn't be allocated
 *         any other err_t on error
 */
err_t ethernetif0_init(struct netif *netif)
{
    static struct ethernetif ethernetif_0;
    AT_NONCACHEABLE_SECTION_ALIGN(static enet_rx_bd_struct_t rxBuffDescrip_0[ENET_RXBD_NUM], FSL_ENET_BUFF_ALIGNMENT);
    AT_NONCACHEABLE_SECTION_ALIGN(static enet_tx_bd_struct_t txBuffDescrip_0[ENET_TXBD_NUM], FSL_ENET_BUFF_ALIGNMENT);
    SDK_ALIGN(static rx_buffer_t rxDataBuff_0[ENET_RXBD_NUM], FSL_ENET_BUFF_ALIGNMENT);
    SDK_ALIGN(static tx_buffer_t txDataBuff_0[ENET_TXBD_NUM], FSL_ENET_BUFF_ALIGNMENT);

    ethernetif_0.RxBuffDescrip = &(rxBuffDescrip_0[0]);
    ethernetif_0.TxBuffDescrip = &(txBuffDescrip_0[0]);
    ethernetif_0.RxDataBuff = &(rxDataBuff_0[0]);
    ethernetif_0.TxDataBuff = &(txDataBuff_0[0]);

    return ethernetif_init(netif, &ethernetif_0, 0U, (ethernetif_config_t *)netif->state);
}

#if (defined(FSL_FEATURE_SOC_ENET_COUNT) && (FSL_FEATURE_SOC_ENET_COUNT > 1)) \
 || (defined(FSL_FEATURE_SOC_LPC_ENET_COUNT) && (FSL_FEATURE_SOC_LPC_ENET_COUNT > 1))
/**
 * Should be called at the beginning of the program to set up the
 * second network interface. It calls the function low_level_init() to do the
 * actual setup of the hardware.
 *
 * This function should be passed as a parameter to netif_add().
 *
 * @param netif the lwip network interface structure for this ethernetif
 * @return ERR_OK if the loopif is initialized
 *         ERR_MEM if private data couldn't be allocated
 *         any other err_t on error
 */
err_t ethernetif1_init(struct netif *netif)
{
    static struct ethernetif ethernetif_1;
    AT_NONCACHEABLE_SECTION_ALIGN(static enet_rx_bd_struct_t rxBuffDescrip_1[ENET_RXBD_NUM], FSL_ENET_BUFF_ALIGNMENT);
    AT_NONCACHEABLE_SECTION_ALIGN(static enet_tx_bd_struct_t txBuffDescrip_1[ENET_TXBD_NUM], FSL_ENET_BUFF_ALIGNMENT);
    SDK_ALIGN(static rx_buffer_t rxDataBuff_1[ENET_RXBD_NUM], FSL_ENET_BUFF_ALIGNMENT);
    SDK_ALIGN(static tx_buffer_t txDataBuff_1[ENET_TXBD_NUM], FSL_ENET_BUFF_ALIGNMENT);

    ethernetif_1.RxBuffDescrip = &(rxBuffDescrip_1[0]);
    ethernetif_1.TxBuffDescrip = &(txBuffDescrip_1[0]);
    ethernetif_1.RxDataBuff = &(rxDataBuff_1[0]);
    ethernetif_1.TxDataBuff = &(txDataBuff_1[0]);

    return ethernetif_init(netif, &ethernetif_1, 1U, (ethernetif_config_t *)netif->state);
}
#endif /* FSL_FEATURE_SOC_*_ENET_COUNT */
