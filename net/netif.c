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

//发送队列成员
static struct pbuf *Tx_pbuf[NET_INSTANCE_NUM] = {NULL};
static pthread_cond_t Send_pbuf_cond[NET_INSTANCE_NUM] = { PTHREAD_COND_INITIALIZER, PTHREAD_COND_INITIALIZER};
static pthread_mutex_t TxPbuf_mutex[NET_INSTANCE_NUM] = {  PTHREAD_MUTEX_INITIALIZER,\
															 PTHREAD_MUTEX_INITIALIZER,	};
//static pthread_mutex_t Send_mutex[NET_INSTANCE_NUM] = {  PTHREAD_MUTEX_INITIALIZER,\
															 PTHREAD_MUTEX_INITIALIZER,	};
extern MacAddr_u16	MAC_UNSPECIFIED_ADDR;
connect_info	Eth_Cnnect_info[CONNECT_INFO_NUM];

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
		for( i = 0; i < CONNECT_INFO_NUM; i++)
			Eth_Cnnect_info[i].status = CON_STATUS_IDLE;
	}
	create_low_level_netif( netif);

	netif->reply_pbuf = pbuf_alloc( PBUF_RAW,netif->mtu, PBUF_POOL);
	inet = (NetInterface *)netif->ll_netif;
	netif->linkoutput = netif_linkoutput;
	netif->upperlayer_output = chitic_layer_output;
	netif->flags |= NETIF_FLAG_ETHARP;
//	sem_init( &netif->Transmit_Done_sem, 0, 1);

//	pthread_mutex_init( &netif->mutex_send, NULL);
//	pthread_mutex_lock( &netif->mutex_send);

	ret = pthread_create ( &inet->isr_tid, NULL, isr_handle_thread, netif);
	CHECK_ERROR( ret, " pthread_create()");
	ret = pthread_create ( &inet->send_tid, NULL, send_pack_thread, netif);
	CHECK_ERROR( ret, " pthread_create()");
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
int netif_connect( struct netif * netif, u8_t* hwaddr)
{
	err_t ret ;
	static uint8_t step = 0;
	static uint8_t i = 0;

	struct pbuf *p;
//	printf("%s %s %d \n", netif->name, __func__, __LINE__);
	switch( step)
	{
		case 0:		//send chitic arp packet
			if( hwaddr == NULL)
					return ERR_BAD_PARAMETER;

			for( i = 0; i < CONNECT_INFO_NUM; i++)
			{
				if( Eth_Cnnect_info[i].status == CON_STATUS_IDLE)
					break;
			}
			if( i == CONNECT_INFO_NUM)
			{
				return ERR_UNAVAILABLE;
			}

			p = pbuf_alloc( PBUF_RAW, 64, PBUF_POOL);
			if( p == NULL)
			{
				printf("pbuf alloc fail:%s,%d \n", __func__, __LINE__);
				return ERR_UNAVAILABLE;
			}
			Eth_Cnnect_info[i].status = CON_STATUS_PENDING;
			ETHADDR16_COPY( Eth_Cnnect_info[i].target_hwaddr, hwaddr);


			ret = chitic_arp_output( netif, p, (struct eth_addr *)hwaddr);
			if( ret != ERR_OK)
			{
//				printf("invoking pbuf_free at :%s %s %d \n", __FILE__, __func__, __LINE__);

				pbuf_free( p);
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
				printf("dest:%02x:%02x:%02x:%02x:%02x:%02x successed \r\n",
						 (unsigned)Eth_Cnnect_info[i].target_hwaddr[0], (unsigned)Eth_Cnnect_info[i].target_hwaddr[1], (unsigned)Eth_Cnnect_info[i].target_hwaddr[2],
						 (unsigned)Eth_Cnnect_info[i].target_hwaddr[3], (unsigned)Eth_Cnnect_info[i].target_hwaddr[4], (unsigned)Eth_Cnnect_info[i].target_hwaddr[5]);
				Attempts = 0;
				step = 0;
				return i;
			}
			delay(10);
			if( Attempts > 100)
				step ++;
			break;
		case 2:	//超时
//			printf(" can't recv arp reply !\n");
			Attempts = 0;
			step = 0;
			Eth_Cnnect_info[i].status = CON_STATUS_IDLE;
			return ERR_TIMEOUT;
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
	NetInterface *inet = (NetInterface *)netif->ll_netif;
	int instance = inet->instance;
	struct pbuf *iterator;
	if( !inet->linkState)
		return ERR_UNINITIALIZED;

	pthread_mutex_lock( &TxPbuf_mutex[instance] );

//	printf("pthread_mutex_lock  TxPbuf_mutex[%d] at : %s %d \n",instance, __func__, __LINE__);
//	printf(" %s set tx pbuf \n",inet->name);
	//这是第一个要加入发送队列的pbuf
	if( Tx_pbuf[ inet->instance] == NULL)
	{
		Tx_pbuf[ inet->instance] = p;
		goto exit_output;
	}
	//从尾部加入新发送的pbuf
	iterator = Tx_pbuf[ inet->instance];
	while ( iterator->next != NULL)
	{
		iterator = iterator->next;
//		if( iterator->ref == 0)
//				printf(" %s error pbuf %p \n",__func__, iterator );
	}

	iterator->next = p;

exit_output:
	pthread_cond_signal( &Send_pbuf_cond[ instance]);
	pthread_mutex_unlock( &TxPbuf_mutex[instance] );

//	if( p->ref == 0)
//	{
//		printf(" %s %d error pbuf %p \n",__func__,__LINE__, p);
//	}

//	printf("pthread_mutex_unlock  TxPbuf_mutex[%d] at : %s %d \n",instance, __func__, __LINE__);

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
//	static uint32_t	last_seq[2] = {0};
//	uint32_t	*pu32;

	while(1)
	{

		pthread_mutex_lock( &TxPbuf_mutex[instance] );

		while( Tx_pbuf[ instance] == NULL)
			pthread_cond_wait( &Send_pbuf_cond[ instance], &TxPbuf_mutex[instance]);
		//从头部取出待发的数据,把全部的数据发完，或者enc624发送失败时退出
		while( Tx_pbuf[ instance] != NULL)
		{
			if( !inet->linkState)
				break;
			if( inet->isr_status & ISR_PROCESSING)
			{
//				printf("the %s  ISR_PROCESSING... \n", inet->name);
				break;
			}
			pbuf_tmp = Tx_pbuf[ instance];

			send_buffer.data = pbuf_tmp->payload;
			send_buffer.len = pbuf_tmp->len;
			ret = enc624j600Driver.SendPacket( inet, &send_buffer, 0);

			if( ret == ERR_OK)
			{
//				if( send_buffer.len > 64)
//				{
//					pu32 = (uint32_t *)( send_buffer.data +18);
//					*pu32 = PP_NTOHL(*pu32);
//					if( *pu32 == 7560)
//					{
//						printf( "send seq %d \n", *pu32 );
//					}
////					if( last_seq[instance] + 1 != *pu32)
////					{
////
////						printf("send err seq %d+1!= %d \n", last_seq[instance], *pu32);
////
////					}
////					last_seq[instance] = *pu32;
//				}

				Tx_pbuf[ instance] = pbuf_tmp->next;
				pbuf_free( pbuf_tmp);

			}
			else
			{
				//此次发送失败，保留数据到下一次发送
//				printf("%s send time out \n",  inet->name);
				delay_us = 100;
//				pthread_mutex_lock( &Send_mutex[instance]);
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
	if( handle < 0 || handle > CONNECT_INFO_NUM)
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
			inet->rxpbuf = inet->rxpbuf_head;
			inet->ethFrame = inet->rxpbuf->payload;
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
//			enc624j600Driver.EventHandler( inet);
//			TRACE_INFO("net%d tx complete \r\n", inet->instance);
//			pthread_mutex_unlock( &Send_mutex[ inet->instance]);
//			sem_post( &p_netif->Transmit_Done_sem);

		}
		if(  inet->isr_status & ISR_LINK_STATUS_CHG)
		{
			atomic_clr( &inet->isr_status, ISR_LINK_STATUS_CHG);
//			printf("%s %s link status  %d \r\n", __func__, inet->name, inet->linkState);
			enc624j600Driver.EventHandler( inet);
//			printf("%s %s link status  %d \r\n", __func__, inet->name, inet->linkState);
			if( inet->linkState)
			{
				p_netif->state |= NETIF_FLAG_LINK_UP;

//				pthread_mutex_unlock( &Send_mutex[ inet->instance]);
//				printf( "net%d  linkState up \n", inet->instance);
//				sem_post( &p_netif->Transmit_Done_sem);
			}
			else
			{
				p_netif->state &= ~NETIF_FLAG_LINK_UP;
			}

//			if( p_netif->link_callback)
//				p_netif->link_callback( p_netif);

		}
		if(  inet->isr_status & ISR_ERROR)
		{
			atomic_clr( &inet->isr_status, ISR_ERROR);
			enc624j600Driver.EventHandler( inet);
			printf(" ISR_ERROR happened \n");
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
	//接受缓存配置成与Enc624的接受缓存一样大
	p_netif->rxpbuf_head = \
			pbuf_alloc( PBUF_RAW, ENC624J600_RX_BUFFER_STOP - ENC624J600_RX_BUFFER_START, PBUF_POOL);
//	printf("eth%d rxpbuf_head = %p \n", p_netif->instance, p_netif->rxpbuf_head);


	//未指定mac值就设置成随机值
	if(macCompAddr(&netif->hwaddr, &MAC_UNSPECIFIED_ADDR) == 0)
	{
		seed = (int) time( NULL ) + netif->num * 4;
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
	{
		netif->mtu = enc624j600Driver.mtu;
		netif->mtu = enc624j600Driver.mtu;
	}

	sprintf( p_netif->name, "%s%d", PRE_NET_NAME,p_netif->instance);
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

	pbuf_free( p_netif->rxpbuf);
	free( p_netif->nicContext);
	free( p_netif);

	return EXIT_SUCCESS;


}

