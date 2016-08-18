/*
 * net_method.c
 *
 *  Created on: 2016-7-4
 *      Author: Administrator
 *      ���������һЩ�ӿں���
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

	//len > 0ʱ��˵�����ܻ����к������
	if( len)
	{
		p_buf->flags = PBUFFLAG_UNPROCESS;
		p_buf->len = len;
		ethernet_input( p_buf, pnet);

		///< �ڴ�����ݻ���Ҫ����������µĽ��ջ������ڽ���
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
		///< �ڴ��е������Ѿ�����ã���ֱ�Ӽ������ڽ���������

	}
//	else
//	{
//		///< ������оƬ���Ѿ��޷���ȡ���������ʱ������Ҫ����������.
//		///< �����Ѿ�����������ݻ���
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
 * @brief ��ָ����ͷ����β������һ���ڵ�.
 *
 * @details ��ͷ�����ǿյ�ʱ�򣬽�ͷ����ָ���½ڵ㣬�����½ڵ���ӵ�β��������½ڵ��Ѿ��������д����ˣ��ͷ�������ýڵ�
 *
 * @param[in]	listhead �����ͷָ��.
 * @param[in]	newnode	������Ľڵ�
 * @retval	ERR_OK	�ɹ�
 * @retval	ERR_BAD_PARAMETER	�½ڵ��Ѿ�������������
 * @warning	����������ڵ����ĵ�һ��Ԫ�ر�����next���������ֲ���Ԥ֪�Ĵ���
 * @warning	���½ڵ㲻�ǵ��ڵ�ʱ��newnode->next ��ΪNULL������������ȥ���β���newnod������������ڵ��Ƿ��Ѿ������ڴ������������
 * @par ��ʶ��
 * 		����
 * @par ����
 * 		��
 * @par �޸���־
 * 		sundh��2016-08-17����
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


