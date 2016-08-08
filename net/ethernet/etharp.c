/*
 * etharp.c
 *
 *  Created on: 2016-7-19
 *      Author: Administrator
 *      实现arp协议，剥离了涉及ip协议部分。
 */

#include "etharp.h"
#include "stats.h"
#include "debug.h"
#include "def.h"
#include <string.h>
#include "net.h"
struct stats_ lwip_stats;

const struct eth_addr ethbroadcast = {{0xff,0xff,0xff,0xff,0xff,0xff}};
const struct eth_addr ethzero = {{0,0,0,0,0,0}};

/** the time an ARP entry stays valid after its last update,
 *  for ARP_TMR_INTERVAL = 5000, this is
 *  (240 * 5) seconds = 20 minutes.
 */
#define ARP_MAXAGE 240
/** the time an ARP entry stays pending after first request,
 *  for ARP_TMR_INTERVAL = 5000, this is
 *  (2 * 5) seconds = 10 seconds.
 *
 *  @internal Keep this number at least 2, otherwise it might
 *  run out instantly if the timeout occurs directly after a request.
 */
#define ARP_MAXPENDING 2

#define HWTYPE_ETHERNET 1

/**
 * Responds to ARP requests to us. Upon ARP replies to us, add entry to cache
 * send out queued IP packets. Updates cache with snooped address pairs.
 *
 * Should be called for incoming ARP packets. The pbuf in the argument
 * is freed by this function.
 *
 * @param netif The lwIP network interface on which the ARP packet pbuf arrived.
 * @param ethaddr Ethernet address of netif.
 * @param p The ARP packet that arrived on netif. Is freed by this function.
 *
 * @return NULL
 *
 * @see pbuf_free()
 */
static void
etharp_arp_input(struct netif *netif, struct eth_addr *ethaddr, struct pbuf *p)
{
	struct etharp_hdr *hdr;
	struct chitic_etharp_hdr *chitic_hdr;
	struct eth_hdr *ethhdr;

	LWIP_ERROR("netif != NULL", (netif != NULL), return;);


	/* drop short ARP packets: we have to check for p->len instead of p->tot_len here
	     since a struct etharp_hdr is pointed to p->payload, so it musn't be chained! */
	if (p->len < SIZEOF_ETHARP_PACKET) {
		LWIP_DEBUGF(ETHARP_DEBUG | LWIP_DBG_TRACE | LWIP_DBG_LEVEL_WARNING,
		  ("etharp_arp_input: packet dropped, too short (%"S16_F"/%"S16_F")\n", p->tot_len,
		  (s16_t)SIZEOF_ETHARP_PACKET));
		ETHARP_STATS_INC(etharp.lenerr);
		ETHARP_STATS_INC(etharp.drop);
		return;
	}

	ethhdr = (struct eth_hdr *)p->payload;
	hdr = (struct etharp_hdr *)((u8_t*)ethhdr + SIZEOF_ETH_HDR);

	 /* RFC 826 "Packet Reception": */
	if ((hdr->hwtype != PP_HTONS(HWTYPE_ETHERNET)) ||
	  (hdr->hwlen != ETHARP_HWADDR_LEN)
	  )  {
		LWIP_DEBUGF(ETHARP_DEBUG | LWIP_DBG_TRACE | LWIP_DBG_LEVEL_WARNING,
		  ("etharp_arp_input: packet dropped, wrong hw type, hwlen, proto, protolen or ethernet type (%"U16_F"/%"U16_F"/%"U16_F"/%"U16_F")\n",
		  hdr->hwtype, hdr->hwlen, hdr->proto, hdr->protolen));
		ETHARP_STATS_INC(etharp.proterr);
		ETHARP_STATS_INC(etharp.drop);
		return;
	}
	ETHARP_STATS_INC(etharp.recv);

	//自定义的协议。对于请求报文，目标id与本网络接口id一直，将发送方和接收方对换位置，然后应答
	//对于应答报文，对比接收方MAC与本机MAC，将发送方MAC加入
	if( hdr->proto == PP_HTONS(ETHTYPE_CHITIC))
	{
		chitic_hdr = (struct chitic_etharp_hdr *)((u8_t*)ethhdr + SIZEOF_ETH_HDR);
		switch (chitic_hdr->opcode)
		{
			case PP_HTONS(ARP_REQUEST):
				if( chitic_hdr->d_id == netif->myid)
				{
					struct pbuf *reply_pbuf = (struct pbuf *)netif->reply_pbuf;
					if( reply_pbuf == NULL)
					{
//						printf("reply_pbuf no buf: %s,%s,%d \n", __FILE__, __func__, __LINE__);
//						break;
						reply_pbuf = pbuf_alloc( PBUF_RAW,netif->mtu, PBUF_TX_POOL);
						netif->reply_pbuf = reply_pbuf;

					}
					//引用计算加1，防止在发送后，被释放掉。
					reply_pbuf->ref ++;
					reply_pbuf->len = p->len;

					//交换目标地址和源地址
					ETHADDR16_COPY(&chitic_hdr->dhwaddr, &chitic_hdr->shwaddr);
					ETHADDR16_COPY(&chitic_hdr->shwaddr, netif->hwaddr);
					chitic_hdr->d_id = chitic_hdr->s_id;
					chitic_hdr->s_id = netif->myid;

					chitic_hdr->opcode = htons(ARP_REPLY);
					memcpy( reply_pbuf->payload, p->payload, p->len);
					/* return ARP reply */
					if( netif->linkoutput(netif, reply_pbuf))
						reply_pbuf->ref --;

					p->flags = PBUFFLAG_TRASH;
				}
				break;
			case PP_HTONS(ARP_REPLY):
				if( chitic_hdr->d_id == netif->myid)
				{
					int i = 0;
					for( i = 0 ; i < ARP_CACHE_NUM; i++)
					{
						if( Eth_Cnnect_info[i].netid == chitic_hdr->s_id)
						{
							ETHADDR16_COPY( Eth_Cnnect_info[i].target_hwaddr, &chitic_hdr->shwaddr);
							Eth_Cnnect_info[i].status = CON_STATUS_ESTABLISH;
							break;

						}
					}
				}
				p->flags = PBUFFLAG_TRASH;
				break;
			 default:
			    LWIP_DEBUGF(ETHARP_DEBUG | LWIP_DBG_TRACE, ("etharp_arp_input: ARP unknown opcode type %"S16_F"\n", htons(hdr->opcode)));
			    ETHARP_STATS_INC(etharp.err);
			    break;

		}
	}
}

err_t
chitic_arp_output(struct netif *netif, struct pbuf *q, connect_info *cnnt_info)
{
  struct chitic_etharp_hdr *chitic_hdr;
  struct eth_hdr *ethhdr ;



  ethhdr = (struct eth_hdr *)q->payload;
  chitic_hdr = (struct chitic_etharp_hdr *)((u8_t*)ethhdr + SIZEOF_ETH_HDR);

  ETHADDR16_COPY(&ethhdr->dest, &ethbroadcast);
  ETHADDR16_COPY(&ethhdr->src, netif->hwaddr);
  ethhdr->type = htons(ETHTYPE_ARP);

  chitic_hdr->hwtype = htons(HWTYPE_ETHERNET);
  chitic_hdr->proto = htons(ETHTYPE_CHITIC);
  chitic_hdr->hwlen = ETHARP_HWADDR_LEN;
  chitic_hdr->protolen = 2;
  chitic_hdr->opcode = htons(ARP_REQUEST);
  ETHADDR16_COPY(&chitic_hdr->shwaddr, netif->hwaddr);
  chitic_hdr->s_id = netif->myid;
  ETHADDR16_COPY(&chitic_hdr->dhwaddr, &ethzero);
  chitic_hdr->d_id = cnnt_info->netid;
  q->len = sizeof( *ethhdr)  + sizeof( *chitic_hdr);
  /* continuation for multicast/broadcast destinations */
  /* obtain source Ethernet address of the given interface */
  /* send packet directly on the link */
  return netif->linkoutput( netif, q);
}


/*
 *
 * 处理接受的数据
 *
 */
err_t ethernet_input(struct pbuf *p, struct netif *netif)
{
	struct eth_hdr* ethhdr;
	u16_t type;

	ethhdr = (struct eth_hdr *)p->payload;
	 if (p->len <= SIZEOF_ETH_HDR) {
	    /* a packet with only an ethernet header (or less) is not valid for us */
	    ETHARP_STATS_INC(etharp.proterr);
	    ETHARP_STATS_INC(etharp.drop);
	    goto free_and_return;
	  }


//
//	 printf("%s input: dest:%02x:%02x:%02x:%02x:%02x:%02x, src:%02x:%02x:%02x:%02x:%02x:%02x, type:%x\n",
//			 netif->name,
//	     (unsigned)ethhdr->dest.addr[0], (unsigned)ethhdr->dest.addr[1], (unsigned)ethhdr->dest.addr[2],
//	     (unsigned)ethhdr->dest.addr[3], (unsigned)ethhdr->dest.addr[4], (unsigned)ethhdr->dest.addr[5],
//	     (unsigned)ethhdr->src.addr[0], (unsigned)ethhdr->src.addr[1], (unsigned)ethhdr->src.addr[2],
//	     (unsigned)ethhdr->src.addr[3], (unsigned)ethhdr->src.addr[4], (unsigned)ethhdr->src.addr[5],
//	     (unsigned)htons(ethhdr->type));

	  type = ethhdr->type;



	  switch( type)
	  {
		case PP_HTONS(ETHTYPE_ARP):

			/* pass p to ARP module */
			etharp_arp_input(netif, (struct eth_addr*)(netif->hwaddr), p);
			break;
		case PP_HTONS(ETHTYPE_CHITIC):
			if( netif->input)
			{
				netif->input( p, netif);
			}
			break;
		default:
			p->flags = PBUFFLAG_TRASH;
			break;
	  }

free_and_return:
//	  pbuf_free(p);
	  return ERR_OK;
}
