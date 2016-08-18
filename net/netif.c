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
#include <errno.h>

static uint16_t Attempts = 0;		//尝试次数
static uint8_t	Hardware_ready[ NET_INSTANCE_NUM] = {0};	///< 硬件初始化完成标志
//发送队列成员
static struct pbuf *Tx_pbuf[NET_INSTANCE_NUM] = {NULL};
static pthread_cond_t Send_pbuf_cond[NET_INSTANCE_NUM] = { PTHREAD_COND_INITIALIZER, PTHREAD_COND_INITIALIZER};
static pthread_mutex_t TxPbuf_mutex[NET_INSTANCE_NUM] = {  PTHREAD_MUTEX_INITIALIZER,\
															 PTHREAD_MUTEX_INITIALIZER,	};
//static pthread_mutex_t Send_mutex[NET_INSTANCE_NUM] = {  PTHREAD_MUTEX_INITIALIZER,\
															 PTHREAD_MUTEX_INITIALIZER,	};
extern MacAddr_u16	MAC_UNSPECIFIED_ADDR;
connect_info	Eth_Cnnect_info[ARP_CACHE_NUM];

static err_t create_low_level_netif( struct netif * netif);
static err_t destory_low_level_netif( struct netif * netif);

err_t netif_linkoutput(struct netif *netif, struct pbuf *p);
err_t chitic_layer_output(struct netif *netif, struct pbuf *p, int handle);
static void* isr_handle_thread(void *arg);
static void* send_pack_thread(void *arg);		//处理发送缓存的线程


void netif_init(struct netif * netif)
{
	int ret;
	int i = 0;
	static char fist_run = 1;
	assert( netif != NULL);
	NetInterface *inet;
	if( fist_run)
	{
		for( i = 0; i < ARP_CACHE_NUM; i++)
			Eth_Cnnect_info[i].status = CON_STATUS_IDLE;
	}
	create_low_level_netif( netif);

	netif->reply_pbuf = pbuf_alloc( PBUF_RAW,netif->mtu, PBUF_TX_POOL);
	inet = (NetInterface *)netif->ll_netif;
	netif->linkoutput = netif_linkoutput;
	netif->upperlayer_output = chitic_layer_output;
	ret = pthread_create ( &inet->isr_tid, NULL, isr_handle_thread, netif);
	CHECK_ERROR( ret, " pthread_create()");
	ret = pthread_create ( &inet->send_tid, NULL, send_pack_thread, netif);
	CHECK_ERROR( ret, " pthread_create()");

	while( Hardware_ready[netif->num] == 0)
		delay(10);

}

void netif_remove(struct netif * netif)
{
	NetInterface *inet = (NetInterface *)netif->ll_netif;
	pthread_cancel( inet->send_tid);
	pthread_join( inet->send_tid, NULL);
	pthread_cancel( inet->isr_tid);
	pthread_join( inet->isr_tid, NULL);
}

void netif_restart(struct netif * netif)
{
	NetInterface *inet = ( NetInterface *)netif->ll_netif;
	enc624j600Driver.restart( inet);
}

//返回值 -1 失败，其他则是连接信息的索引
int netif_connect( struct netif * netif, u16_t d_id)
{
	err_t ret ;
	static uint8_t step = 0;
	int 	i = 0;
	int 	least_hits = 0;
	int		least_idx = 0;
	struct pbuf *p;
//	printf("%s %s %d \n", netif->name, __func__, __LINE__);
	switch( step)
	{
		case 0:		//send chitic arp packet
			for( i = 0; i < ARP_CACHE_NUM; i++)
			{
				if( Eth_Cnnect_info[i].status == CON_STATUS_IDLE || Eth_Cnnect_info[i].netid == d_id)
					break;
			}
			//找到使用率最低的那个连接，并替换掉
			if( i == ARP_CACHE_NUM)
			{
				least_hits = Eth_Cnnect_info[0].hits;
				for( i = 1; i < ARP_CACHE_NUM; i++)
				{
					if( Eth_Cnnect_info[i].hits < least_hits)
					{
						least_hits = Eth_Cnnect_info[i].hits;
						least_idx = i;
					}
				}

				i = least_idx;

			}

			p = pbuf_alloc( PBUF_RAW, 64, PBUF_TX_POOL);
			if( p == NULL)
			{
				printf("pbuf alloc fail:%s,%d \n", __func__, __LINE__);
				return ERR_UNAVAILABLE;
			}
			Eth_Cnnect_info[i].status = CON_STATUS_PENDING;
			ETHADDR16_COPY( Eth_Cnnect_info[i].target_hwaddr, &ethbroadcast);
			Eth_Cnnect_info[i].netid = d_id;

			ret = chitic_arp_output( netif, p, Eth_Cnnect_info + i);
			if( ret != ERR_OK)
			{
				Dubug_info.pbuf_free_local = 4;
				pbuf_free( p);
//				printf("[%s] connect send chitic arp fail \n", netif->name);
				Eth_Cnnect_info[i].status = CON_STATUS_IDLE;
				return ERR_UNAVAILABLE;
			}
			step ++;
			break;
		case 1:		//check
			Attempts ++;
			for( i = 0; i < ARP_CACHE_NUM; i++)
			{
				if(  Eth_Cnnect_info[i].status == CON_STATUS_ESTABLISH && \
						Eth_Cnnect_info[i].netid == d_id)
				{
					printf(" Connect ");
					printf("dest:%02x:%02x:%02x:%02x:%02x:%02x successed \r\n",
							 (unsigned)Eth_Cnnect_info[i].target_hwaddr[0], (unsigned)Eth_Cnnect_info[i].target_hwaddr[1], (unsigned)Eth_Cnnect_info[i].target_hwaddr[2],
							 (unsigned)Eth_Cnnect_info[i].target_hwaddr[3], (unsigned)Eth_Cnnect_info[i].target_hwaddr[4], (unsigned)Eth_Cnnect_info[i].target_hwaddr[5]);
					Attempts = 0;
					step = 0;
					return i;
				}
			}

			delay(20);
			if( Attempts > 100)
				step ++;
			break;
		case 2:	//超时
			printf(" can't recv arp reply from %d !\n", d_id);
			Attempts = 0;
			step = 0;
			for( i = 0; i < ARP_CACHE_NUM; i++)
			{
				if( Eth_Cnnect_info[i].netid == d_id)
					Eth_Cnnect_info[i].status = CON_STATUS_IDLE;
			}
			return ERR_TIMEOUT;
	}







	return ERR_UNKOWN;
}

int netif_disconnect( struct netif * netif, int idx)
{
	Eth_Cnnect_info[idx].status = CON_STATUS_IDLE;
	return ERR_OK;
}

err_t netif_linkoutput(struct netif *netif, struct pbuf *p)
{
	NetInterface *inet = (NetInterface *)netif->ll_netif;
	int instance = inet->instance;
	err_t ret = 0;
	struct pbuf *p_iterator, *p_free;


	if( !inet->linkState)
	{
//		printf("[%s] %s can't send packet, because of linkstate is down \n", inet->name, __func__);
		return ERR_UNINITIALIZED;
	}


	pthread_mutex_lock( &TxPbuf_mutex[instance] );

	//刷新缓存区时会如此调用
	if( p == NULL)
	{
		goto exit_output;

	}
	ret = insert_node_to_listtail( (void **)&Tx_pbuf[ instance], p);


exit_output:
	pthread_cond_signal( &Send_pbuf_cond[ instance]);
	pthread_mutex_unlock( &TxPbuf_mutex[instance] );
	return ERR_OK;

}
static void* send_pack_thread(void *arg)
{
	struct netif *p_netif = (struct netif *)arg;
	NetInterface *inet = ( NetInterface *)p_netif->ll_netif;
	int instance = inet->instance;
	NetBuffer		send_buffer;
	err_t 	ret;
	int 		delay_us = 0;
	struct pbuf *pbuf_tmp;
//	pthread_condattr_t attr;
//	struct timespec to;


	/* Set up the condvar attributes to use CLOCK_MONOTONIC. */

//	pthread_condattr_init( &attr);
//	pthread_condattr_setclock( &attr, CLOCK_MONOTONIC);
//	pthread_cond_init( &Send_pbuf_cond[ instance], &attr);


	while(1)
	{

		pthread_mutex_lock( &TxPbuf_mutex[instance] );

//		clock_gettime(CLOCK_MONOTONIC, &to);
//		to.tv_sec += 5;
//
//
//		pthread_cond_timedwait(&Send_pbuf_cond[ instance],  &TxPbuf_mutex[instance], &to);
		while( Tx_pbuf[ instance] == NULL)
			pthread_cond_wait( &Send_pbuf_cond[ instance], &TxPbuf_mutex[instance]);
		//从头部取出待发的数据,把全部的数据发完，或者enc624发送失败时退出
		while( Tx_pbuf[ instance] != NULL)
		{
			if( inet->isr_status & ISR_PROCESSING)
			{
//				printf("the %s  ISR_PROCESSING... \n", inet->name);
				delay_us = 100;
				break;
			}
			pbuf_tmp = Tx_pbuf[ instance];

//			if( pbuf_tmp == p_netif->reply_pbuf)
//			{
//				printf("%s send reply_pbuf \n",__func__);
//			}
			send_buffer.data = pbuf_tmp->payload;
			send_buffer.len = pbuf_tmp->len;
			ret = enc624j600Driver.SendPacket( inet, &send_buffer, 0);

			if( ret == ERR_OK)
			{
				Tx_pbuf[ instance] = pbuf_tmp->next;
				Dubug_info.pbuf_free_local = 5;
				pbuf_free( pbuf_tmp);


			}
			else
			{
				//此次发送失败，保留数据到下一次发送
//				printf("%s send fail \n",__func__);
				delay_us = 100;
				break;
			}

		} //while(1)
		pthread_mutex_unlock( &TxPbuf_mutex[instance] );
		if( delay_us)
		{
			usleep(delay_us);
			delay_us = 0;
		}
	}			//while(1)
	return NULL;
}
err_t chitic_layer_output(struct netif *netif, struct pbuf *p, int handle)
{
	struct eth_hdr *ethhdr ;
	u8_t *dst_hwaddr;

	//内存不足，要求刷新缓存区
	if( p == NULL)
		return netif->linkoutput( netif, p);
	if( handle < 0 || handle > ARP_CACHE_NUM)
		return -1;

	if( Eth_Cnnect_info[ handle].status != CON_STATUS_ESTABLISH)
		return -1;
	dst_hwaddr = (u8_t *)Eth_Cnnect_info[ handle].target_hwaddr;
	/* make room for Ethernet header - should not fail */
	if (pbuf_header(p, sizeof(struct eth_hdr)) != 0) {
		/* bail out */
		printf("etharp_output: could not allocate room for header.\n");

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


	ret = enc624j600Driver.Init( inet);
	if( ret)
	{
		printf(" init %s fail!", inet->name);
		return NULL;
	}

	enc624j600Driver.EnableIrq( inet);
	Hardware_ready[p_netif->num] = 1;
	while(1)
	{
		enc624j600Driver.EnableIrq( inet);
		pthread_cleanup_push(cleanup, arg);
		atomic_clr( &inet->isr_status, ISR_PROCESSING);

		InterruptWait_r(NULL, NULL);

		enc624j600Driver.DisableIrq( inet);
		atomic_set( &inet->isr_status, ISR_PROCESSING);

//		TRACE_INFO(" %s isr happend status %04x \r\n",inet->name,  inet->isr_status);
		Dubug_info.irq_handle_count[ inet->instance] ++;
		if( inet->isr_status & ISR_RECV_PACKET)
		{

			atomic_clr( &inet->isr_status, ISR_RECV_PACKET);
//			TRACE_INFO("%s recv packet \r\n", inet->name);




			if( inet->rxpbuf == NULL)
			{
				inet->rxpbuf = pbuf_alloc( PBUF_RAW, p_netif->mtu, PBUF_RX_POOL);

				///< 没有内存接收新数据，从保留的待处理数据中回收
				if( inet->rxpbuf == NULL)
				{
					if( inet->rxUnprocessed)
					{
						inet->rxpbuf = inet->rxUnprocessed;
						inet->rxUnprocessed = inet->rxUnprocessed->next;
						Dubug_info.pbuf_free_local = 6;
						pbuf_free(inet->rxpbuf);

					}
					else
					{
						///< 当有块板子快速发送时应用数据而本机还没有进入处理流程时会出现这个情况
						///< 这种情况下，所有的接收pbuf都在接收处理队列中去了
						p_netif->input(NULL, p_netif);			//释放部分缓存
					}
					inet->rxpbuf = pbuf_alloc( PBUF_RAW, p_netif->mtu, PBUF_RX_POOL);	//再次申请内存

				}


			}

			if( inet->rxpbuf)
			{
				inet->rxpbuf->flags = PBUFFLAG_TRASH;
				inet->ethFrame = inet->rxpbuf->payload;
			}
			else
			{
				inet->ethFrame = NULL;
			}
			enc624j600Driver.EventHandler( inet);

		}
		if( inet->isr_status & ISR_RECV_ABORT)
		{
			atomic_clr( &inet->isr_status, ISR_RECV_ABORT);
			enc624j600Driver.EventHandler( inet);

		}
		if(  inet->isr_status & ISR_TRAN_COMPLETE)
		{
			atomic_clr( &inet->isr_status, ISR_TRAN_COMPLETE);
			enc624j600Driver.EventHandler( inet);
		}
		if(  inet->isr_status & ISR_LINK_STATUS_CHG)
		{
			atomic_clr( &inet->isr_status, ISR_LINK_STATUS_CHG);
			enc624j600Driver.EventHandler( inet);
			if( p_netif->link_callback)
				p_netif->link_callback( p_netif);

		}
		if(  inet->isr_status & ISR_NONE)
		{
			atomic_clr( &inet->isr_status, ISR_NONE);
			enc624j600Driver.EventHandler( inet);
//			enc624j600_print_reg(inet,  ENC624J600_REG_ESTAT);
//			printf(" ISR_ERROR happened \n");
		}

		pthread_cleanup_pop(0);
//		delay(1);
	}		//while(1)





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

	p_netif->hl_netif = netif;
	p_netif->nicContext = malloc( sizeof(u16_t));
	if( p_netif->nicContext == NULL)
			goto err2;
	p_netif->instance = netif->num;

	p_netif->macAddr = (MacAddr_u16	*)netif->hwaddr;
	p_netif->name = netif->name;

	//未指定mac值就设置成随机值
	if(macCompAddr(netif->hwaddr, &MAC_UNSPECIFIED_ADDR) == 0)
	{
		seed = (int) time( NULL ) + netif->num * 4;
		srand( seed++ );
		p_netif->macAddr->w[0] = rand()%0xffff;		//0 ~ 65535
		srand(  seed++ );
		p_netif->macAddr->w[1] = rand()%0xffff;		//0 ~ 65535
		srand(  seed++ );
		p_netif->macAddr->w[2] = rand()%0xffff;		//0 ~ 65535

//		memcpy( netif->hwaddr, p_netif->macAddr.w, NETIF_MAX_HWADDR_LEN);
	}

	p_netif->nicRxEvent = RX_EVENT;
	p_netif->nicTxEvent = TX_EVENT;

	if( netif->mtu)
		enc624j600Driver.mtu = netif->mtu;
	else
	{
		netif->mtu = enc624j600Driver.mtu;
		netif->mtu = enc624j600Driver.mtu;
	}

	netif->name =  p_netif->name;

	return EXIT_SUCCESS;


err2:
	free( netif->ll_netif);
err1:
	return EXIT_FAILURE;
}


static err_t destory_low_level_netif( struct netif * netif)
{

	NetInterface	*p_netif = netif->ll_netif;
//	printf("invoking pbuf_free at :%s %s %d \n", __FILE__, __func__, __LINE__);

	free( p_netif->nicContext);
	free( p_netif);

	return EXIT_SUCCESS;


}

