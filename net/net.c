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

//#define DEBUG_NET
const static int  PBUF_PLAYLOAD_OFFSWT =  ( int)&( ( ( struct pbuf *)0)->payload);

err_t macCompAddr( MacAddr_u16 *mac1, MacAddr_u16 *mac2)
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
	int	i = 0;
	TRACE_INFO("Recv %d datas  ", len);

	for( i = 0; i < len; i ++)
	{
		if( i % 8 == 0)
			TRACE_INFO("\r\n");
		TRACE_INFO("0x%02x ", frame[i]);
	}
	TRACE_INFO("\r\n");

	return EXIT_SUCCESS;
#else
//	atomic_set( &Inet->isr_status, ISR_RECV_PACKET);
	struct pbuf *p_buf = ( struct pbuf *) ( (int)frame - PBUF_PLAYLOAD_OFFSWT);
	int	i = 0;
	p_buf->len = len;
	TRACE_INFO("%s Recv %d datas  ", Inet->name, len);

	for( i = 0; i < len; i ++)
	{
		if( i % 16 == 0)
			TRACE_INFO("\r\n");
		TRACE_INFO("0x%02x ", frame[i]);
	}
	TRACE_INFO("\r\n");

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



