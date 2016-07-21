/*
 * netif.c
 *
 *  Created on: 2016-7-18
 *      Author: Administrator
 *      实现lwip的网络接口管理功能，把ip部分全部去除了。
 */
#include "netif.h"
#include "debug.h"
#include <assert.h>
#include <stdlib.h>
#include <time.h>
#include "net.h"
#include <string.h>
#include "enc624j600.h"
#include <sys/neutrino.h>
#include "etharp.h"
#include "def.h"
#include <unistd.h>

static uint16_t Attempts = 0;		//尝试次数

extern MacAddr_u16	MAC_UNSPECIFIED_ADDR;
connect_info	Eth_Cnnect_info[CONNECT_INFO_NUM];

static err_t create_low_level_netif( struct netif * netif);
static err_t destory_low_level_netif( struct netif * netif);

err_t netif_linkoutput(struct netif *netif, struct pbuf *p);
err_t chitic_layer_output(struct netif *netif, struct pbuf *p, int handle);
static void* isr_handle_thread(void *arg);


void netif_init(struct netif * netif)
{
	int ret;
	int i = 0;
	static char fist_run = 1;
	assert( netif != NULL);
	NetInterface *inet;
	if( fist_run)
	{
		for( i = 0; i < CONNECT_INFO_NUM; i++)
			Eth_Cnnect_info[i].status = CON_STATUS_IDLE;
	}
	create_low_level_netif( netif);

	inet = (NetInterface *)netif->ll_netif;
	netif->linkoutput = netif_linkoutput;
	netif->upperlayer_output = chitic_layer_output;
	netif->flags |= NETIF_FLAG_ETHARP;
	sem_init( &netif->Transmit_Done_sem, 0, 0);
	ret = pthread_create ( &inet->tid, NULL, isr_handle_thread, netif);
	CHECK_ERROR( ret, " pthread_create()");


}

void netif_remove(struct netif * netif)
{
	NetInterface *inet = (NetInterface *)netif->ll_netif;;
	pthread_cancel( inet->tid);
	pthread_join( inet->tid, NULL);
}

//返回值 -1 失败，其他则是连接信息的索引
int netif_connect( struct netif * netif, u8_t* hwaddr)
{
	err_t ret ;
	int count = 5;
	static uint8_t step = 0;
	static uint8_t i = 0;

	struct pbuf *p;
	switch( step)
	{
		case 0:		//send chitic arp packet
			if( hwaddr == NULL)
					return ERR_BAD_PARAMETER;
			p = pbuf_alloc( PBUF_RAW, 64, PBUF_POOL);
			for( i = 0; i < CONNECT_INFO_NUM; i++)
			{
				if( Eth_Cnnect_info[i].status == CON_STATUS_IDLE)
					break;
			}
			if( i == CONNECT_INFO_NUM)
			{
				pbuf_free( p);
				return ERR_UNAVAILABLE;
			}

			Eth_Cnnect_info[i].status = CON_STATUS_PENDING;
			ETHADDR16_COPY( Eth_Cnnect_info[i].target_hwaddr, hwaddr);
			while( count)
			{
				ret = chitic_arp_output( netif, p, (struct eth_addr *)hwaddr);
				if( ret == 0)
					break;
				count --;
			}
			pbuf_free( p);
			if( ret != ERR_OK)
			{

				printf(" tx busy !\n");
				Eth_Cnnect_info[i].status = CON_STATUS_IDLE;
				return ERR_UNAVAILABLE;
			}
			step ++;
			break;
		case 1:		//check
			Attempts ++;
			if( Eth_Cnnect_info[i].status == CON_STATUS_ESTABLISH)
			{
				printf(" Connect ");
				printf("dest:%02x:%02x:%02x:%02x:%02x:%02x successed\n",
						 (unsigned)Eth_Cnnect_info[i].target_hwaddr[0], (unsigned)Eth_Cnnect_info[i].target_hwaddr[1], (unsigned)Eth_Cnnect_info[i].target_hwaddr[2],
						 (unsigned)Eth_Cnnect_info[i].target_hwaddr[3], (unsigned)Eth_Cnnect_info[i].target_hwaddr[4], (unsigned)Eth_Cnnect_info[i].target_hwaddr[5]);
//				enc624j600Driver.SendPacket( netif->ll_netif, &send_buffer, 0);
				Attempts = 0;
				step = 0;
				return i;
			}
			delay(10);
			if( Attempts > 100)
				step ++;
			break;
		case 3:	//超时
			printf(" can't recv arp reply !\n");
			Attempts = 0;
			step = 0;
			Eth_Cnnect_info[i].status = CON_STATUS_IDLE;
			return ERR_UNAVAILABLE;
	}







	return ERR_UNKOWN;
}

int netif_disconnect( struct netif * netif, int handle)
{
	Eth_Cnnect_info[handle].status = CON_STATUS_IDLE;
	return ERR_OK;
}

err_t netif_linkoutput(struct netif *netif, struct pbuf *p)
{
	NetBuffer		send_buffer;
	NetInterface *inet = (NetInterface *)netif->ll_netif;
	struct timespec tm;

//	if( sem_trywait( &netif->Transmit_Done_sem) == -1)
//		return ERR_UNAVAILABLE;

	clock_gettime(CLOCK_REALTIME, &tm);
	tm.tv_sec += 1;		//1s

	if( sem_timedwait( &netif->Transmit_Done_sem, &tm ) == -1)
	{
		printf(" wait sem fail \n");
		return ERR_UNAVAILABLE;
	}


	if( !inet->linkState )
	{
		printf(" net unconnect \n");
		return ERR_UNINITIALIZED;
	}
	printf("send ... \n");
	send_buffer.data = p->payload;
	send_buffer.len = p->len;
	return enc624j600Driver.SendPacket( netif->ll_netif, &send_buffer, 0);
//	sem_wait( &netif->Transmit_Done_sem);

//	return sem_timedwait( &netif->Transmit_Done_sem, &tm );

}

err_t chitic_layer_output(struct netif *netif, struct pbuf *p, int handle)
{
	struct eth_hdr *ethhdr ;
	u8_t *dst_hwaddr;
	if( handle < 0 || handle > CONNECT_INFO_NUM)
		return -1;

	if( Eth_Cnnect_info[ handle].status != CON_STATUS_ESTABLISH)
		return -1;
	dst_hwaddr = (u8_t *)Eth_Cnnect_info[ handle].target_hwaddr;
	/* make room for Ethernet header - should not fail */
	if (pbuf_header(p, sizeof(struct eth_hdr)) != 0) {
		/* bail out */
		LWIP_DEBUGF(ETHARP_DEBUG | LWIP_DBG_TRACE | LWIP_DBG_LEVEL_SERIOUS,
		  ("etharp_output: could not allocate room for header.\n"));

		return ERR_BUF;
	}
	ethhdr = (struct eth_hdr *)p->payload;


	ETHADDR16_COPY(&ethhdr->dest, dst_hwaddr);
	ETHADDR16_COPY(&ethhdr->src, netif->hwaddr);
	ethhdr->type = htons(ETHTYPE_CHITIC);


	return netif->linkoutput( netif, p);

}

static void cleanup(void *arg)
{
	struct netif *p_netif = (struct netif *)arg;

	NetInterface *inet = ( NetInterface *)p_netif->ll_netif;
	printf("net drive %s thread exit \n", inet->name );
	enc624j600Driver.destory( inet);
	destory_low_level_netif( arg);

}
static void* isr_handle_thread(void *arg)
{
	struct netif *p_netif = (struct netif *)arg;
	NetInterface *inet = ( NetInterface *)p_netif->ll_netif;
	err_t	ret = 0;
	struct pbuf *p_rxbuf;
	ret = enc624j600Driver.Init( inet);
	if( ret)
	{
		printf(" init %s fail!", inet->name);
		return NULL;
	}

	inet->hl_netif = p_netif;

	while(1)
	{
		enc624j600Driver.EnableIrq( inet);
		pthread_cleanup_push(cleanup, arg);

		InterruptWait_r(NULL, NULL);
		enc624j600Driver.DisableIrq( inet);
		if( inet->isr_status & ISR_RECV_PACKET)
		{
			atomic_clr( &inet->isr_status, ISR_RECV_PACKET);
			p_rxbuf = pbuf_alloc( PBUF_RAW, ETH_MTU, PBUF_POOL);
			if( p_rxbuf != NULL)
			{
				inet->ethFrame = p_rxbuf->payload;
				inet->rxpbuf = p_rxbuf;
				enc624j600Driver.EventHandler( inet);
			}
			else
			{
				TRACE_INFO("pbuf_alloc fail \r\n");

			}
			pbuf_free(p_rxbuf);

		}
		if(  inet->isr_status & ISR_TRAN_COMPLETE)
		{
			atomic_clr( &inet->isr_status, ISR_TRAN_COMPLETE);
			enc624j600Driver.EventHandler( inet);
			sem_post( &p_netif->Transmit_Done_sem);

		}
		if(  inet->isr_status & ISR_LINK_STATUS_CHG)
		{
			atomic_clr( &inet->isr_status, ISR_LINK_STATUS_CHG);
			enc624j600Driver.EventHandler( inet);
			if( inet->linkState)
			{
//				TRACE_INFO("sem_post  %d \r\n", inet->instance);
				sem_post( &p_netif->Transmit_Done_sem);
			}

			if( p_netif->link_callback)
				p_netif->link_callback( p_netif);

		}
		if(  inet->isr_status & ISR_ERROR)
		{
			atomic_clr( &inet->isr_status, ISR_ERROR);
			printf(" ISR_ERROR happened \n");
		}

		pthread_cleanup_pop(0);
	}





}




static err_t create_low_level_netif( struct netif * netif)
{
	NetInterface	*p_netif;
	uint32_t		seed;
	netif->ll_netif = malloc( sizeof(NetInterface));

	if( netif->ll_netif == NULL)
		goto err1;

	p_netif = ( NetInterface	*)netif->ll_netif;
	memset( p_netif, 0, sizeof(NetInterface));
	p_netif->nicContext = malloc( sizeof(u16_t));
	if( p_netif->nicContext == NULL)
			goto err2;
	p_netif->instance = netif->num;
	//未指定mac值就设置成随机值
	if(macCompAddr(&netif->hwaddr, &MAC_UNSPECIFIED_ADDR) == 0)
	{
		seed = (int) time( NULL );
		srand( seed++ );
		p_netif->macAddr.w[0] = rand()%0xffff;		//0 ~ 65535
		srand(  seed++ );
		p_netif->macAddr.w[1] = rand()%0xffff;		//0 ~ 65535
		srand(  seed++ );
		p_netif->macAddr.w[2] = rand()%0xffff;		//0 ~ 65535

		memcpy( netif->hwaddr, p_netif->macAddr.w, NETIF_MAX_HWADDR_LEN);
	}

	p_netif->nicRxEvent = RX_EVENT;
	p_netif->nicTxEvent = TX_EVENT;

	if( netif->mtu)
		enc624j600Driver.mtu = netif->mtu;
	else
		netif->mtu = enc624j600Driver.mtu;

	sprintf( p_netif->name, "%s%d", PRE_NET_NAME,p_netif->instance);


	return EXIT_SUCCESS;


err2:
	free( netif->ll_netif);
err1:
	return EXIT_FAILURE;
}


static err_t destory_low_level_netif( struct netif * netif)
{

	NetInterface	*p_netif = netif->ll_netif;

	free( p_netif->nicContext);
	free( p_netif);

	return EXIT_SUCCESS;


}

