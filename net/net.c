/*
 * net_method.c
 *
 *  Created on: 2016-7-4
 *      Author: Administrator
 *      网络操作的一些接口函数
 */


#include "net.h"
#include "debug.h"
#include "pbuf.h"
#include <time.h>
#include "netif.h"
#include "etharp.h"
//#define DEBUG_NET
const static int  NETIF_INET_OFFSWT =  ( int)&( ( ( struct netif *)0)->ll_netif);

err_t macCompAddr( void *mac1, void *mac2)
{
	int i;
	uint8_t  *addr1 = (uint8_t *)mac1;
	uint8_t  *addr2 = (uint8_t *)mac2;

	for( i = 0; i < MAC_LEN; i ++)
	{
		if( addr1[i] != addr2[i] )
			break;
	}

	if( i == MAC_LEN)
		return 0;
	else
		return 1;
}

err_t nicNotifyLinkChange( NetInterface *Inet )
{
#ifdef DEBUG_NET
	TRACE_INFO("Drive Piling :%s-%s-%d \r\n", __FILE__, __func__, __LINE__);

	return EXIT_SUCCESS;
#else
	atomic_set( &Inet->isr_status, ISR_LINK_STATUS_CHG);
	return EXIT_SUCCESS;
#endif
}

err_t nicProcessPacket( NetInterface * Inet, uint8_t *frame, int len)
{
#ifdef DEBUG_NET
	int i;
	struct pbuf *p_buf = ( struct pbuf *) ( Inet->rxpbuf);
	uint8_t		*data = (uint8_t *) p_buf->payload;
	TRACE_INFO("%s Recv %d datas  ", Inet->name,  p_buf->len);

	for( i = 0; i < p_buf->len; i ++)
	{
		if( i % 16 == 0)
			TRACE_INFO("\r\n");
		TRACE_INFO("0x%02x ", data[i]);
	}
	TRACE_INFO("\r\n");
	return EXIT_SUCCESS;
#else

	struct pbuf *p_buf = ( struct pbuf *) ( Inet->rxpbuf);
	struct netif *pnet = ( struct netif *) ( Inet->hl_netif);

	if( len)
	{
		p_buf->flags = PBUFFLAG_UNPROCESS;
		p_buf->len = len;
		ethernet_input( p_buf, pnet);

		///< 本内存的数据还需要处理，则分配新的接收缓存用于接受
		///< 已经处理好的缓存就用于接收后面的数据
		if( p_buf->flags != PBUFFLAG_TRASH)
		{
			Inet->rxpbuf = pbuf_alloc( PBUF_RAW, pnet->mtu, PBUF_RX_POOL);
			if( Inet->rxpbuf)
			{
				Inet->rxpbuf->flags = PBUFFLAG_TRASH;
				Inet->ethFrame = Inet->rxpbuf->payload;
			}
			else
			{
				Inet->ethFrame = NULL;
			}
		}

	}
	else
	{
		///< 从网络芯片中已经无法读取更多的数据时，不需要继续接收了.
		///< 回收已经处理掉的数据缓存
		if( p_buf->flags == PBUFFLAG_TRASH)
			pbuf_free( p_buf);
	}
	return EXIT_SUCCESS;
#endif
}

int netBufferGetLength( const NetBuffer *net_buf)
{
#ifdef DEBUG_NET
	TRACE_INFO("Drive Piling :%s-%s-%d \r\n", __FILE__, __func__, __LINE__);
	return EXIT_SUCCESS;
#else


	return net_buf->len;

#endif
}






