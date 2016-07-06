/*
 * net_method.c
 *
 *  Created on: 2016-7-4
 *      Author: Administrator
 *      网络操作的一些接口函数
 */


#include "net.h"
#include "debug.h"

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
#ifdef DEBUG_DRIVE_PILING
	TRACE_INFO("Drive Piling :%s-%s-%d \r\n", __FILE__, __func__, __LINE__);
	return EXIT_SUCCESS;
#else

#endif
}

err_t nicProcessPacket( NetInterface * Inet, uint8_t *frame, int len)
{
#ifdef DEBUG_DRIVE_PILING
	TRACE_INFO("Drive Piling :%s-%s-%d \r\n", __FILE__, __func__, __LINE__);
	return EXIT_SUCCESS;
#else

#endif
}

int netBufferGetLength( NetBuffer *net_buf)
{
#ifdef DEBUG_DRIVE_PILING
	TRACE_INFO("Drive Piling :%s-%s-%d \r\n", __FILE__, __func__, __LINE__);
	return EXIT_SUCCESS;
#else

#endif
}



