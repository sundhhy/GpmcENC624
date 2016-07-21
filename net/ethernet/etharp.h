/*
 * etharp.h
 *
 *  Created on: 2016-7-19
 *      Author: Administrator
 */

#ifndef ETHARP_H_
#define ETHARP_H_
#include "net.h"
#include "netif.h"
#include <stdint.h>


#define SIZEOF_ETH_HDR (14 + ETH_PAD_SIZE)
#define SIZEOF_ETHARP_HDR 28
#define SIZEOF_ETHARP_PACKET (SIZEOF_ETH_HDR + SIZEOF_ETHARP_HDR)

/** 5 seconds period */
#define ARP_TMR_INTERVAL 5000

#define ETHTYPE_ARP       0x0806U
#define ETHTYPE_IP        0x0800U
#define ETHTYPE_VLAN      0x8100U
#define ETHTYPE_PPPOEDISC 0x8863U  /* PPP Over Ethernet Discovery Stage */
#define ETHTYPE_PPPOE     0x8864U  /* PPP Over Ethernet Session Stage */

#define ETHTYPE_CHITIC    0xfffeU	/* 自定义的一种协议 */


/** ARP message types (opcodes) */
#define ARP_REQUEST 1
#define ARP_REPLY   2

struct eth_hdr {
	MacAddr	dest;
	MacAddr	src;
	uint16_t	type;
};
struct eth_addr {
  u8_t addr[ETHARP_HWADDR_LEN];
};
struct ip_addr2 {
  u16_t addrw[2];
} ;

/** the ARP message, see RFC 826 ("Packet format") */
struct etharp_hdr {
  u16_t hwtype;
  u16_t proto;
  u8_t  hwlen;
  u8_t  protolen;
  u16_t opcode;
  struct eth_addr shwaddr;
  struct ip_addr2 sipaddr;
  struct eth_addr dhwaddr;
  struct ip_addr2 dipaddr;
} ;

struct chitic_etharp_hdr {
  u16_t hwtype;
  u16_t proto;
  u8_t  hwlen;
  u8_t  protolen;
  u16_t opcode;
  struct eth_addr shwaddr;
  struct eth_addr dhwaddr;
} ;



err_t ethernet_input(struct pbuf *p, struct netif *netif);
err_t
chitic_arp_output(struct netif *netif, struct pbuf *q, struct eth_addr *target_hwaddr);
#endif /* ETHARP_H_ */
