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

	//len > 0时，说明可能还会有后继数据
	if( len)
	{
		p_buf->flags = PBUFFLAG_UNPROCESS;
		p_buf->len = len;
		ethernet_input( p_buf, pnet);

		///< 内存的数据还需要处理，则分配新的接收缓存用于接受
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
		///< 内存中的数据已经处理好，就直接继续用于接受新数据

	}
//	else
//	{
//		///< 从网络芯片中已经无法读取更多的数据时，不需要继续接收了.
//		///< 回收已经处理掉的数据缓存
//		if( p_buf->flags != PBUFFLAG_TRASH)
//		{
//			Dubug_info.pbuf_free_local = 3;
//			pbuf_free( p_buf);
//			Inet->rxpbuf = NULL;
//			Inet->ethFrame = NULL;
//		}
//
//	}
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


/**
 * @brief 向指定的头链表尾部插入一个节点.
 *
 * @details 当头链表是空的时候，将头链表指向新节点，否则将新节点添加到尾部。如果新节点已经在链表中存在了，就放弃插入该节点
 *
 * @param[in]	listhead 链表的头指针.
 * @param[in]	newnode	带插入的节点
 * @retval	ERR_OK	成功
 * @retval	ERR_BAD_PARAMETER	新节点已经存在于链表中
 * @warning	操作的链表节点对象的第一个元素必须是next，否则会出现不可预知的错误
 * @warning	当新节点不是单节点时（newnode->next 不为NULL），函数不会去依次查找newnod这个链表其他节点是否已经存在在带插入的链表中
 * @par 标识符
 * 		保留
 * @par 其它
 * 		无
 * @par 修改日志
 * 		sundh于2016-08-17创建
 */
err_t insert_node_to_listtail(void **listhead, void *newnode)
{
	list_node	*p_itreator = *listhead;

	if( newnode == NULL)
		return ERR_BAD_PARAMETER;

	if( *listhead == NULL)
	{
		*listhead = newnode;
		return ERR_OK;
	}

	while( p_itreator->next != NULL)
	{
		if( p_itreator == newnode)
			return ERR_BAD_PARAMETER;
		p_itreator = p_itreator->next;
		if( p_itreator == p_itreator->next)
			return ERR_CATASTROPHIC_ERR;
	}

	p_itreator->next = newnode;
	if( p_itreator == newnode)
	{
		printf("%s insert the same: %p - %p \n",p_itreator ,newnode);
	}
	return ERR_OK;

}


