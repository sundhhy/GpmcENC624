/*
 * osAbstraction.c
 *
 *  Created on: 2016-7-4
 *      Author: Administrator
 *      �ṩ����ϵͳ�ķ���֧��
 */
#include "osAbstraction.h"
#include "debug.h"
err_t osSetEvent( void *event)
{

#ifdef DEBUG_DRIVE_PILING
	TRACE_INFO("Drive Piling :%s-%s-%d \r\n", __FILE__, __func__, __LINE__);
	return EXIT_SUCCESS;
#else

#endif
}

int osSetEventFromIsr( void *event)
{
#ifdef DEBUG_DRIVE_PILING
	TRACE_INFO("Drive Piling :%s-%s-%d \r\n", __FILE__, __func__, __LINE__);
	return EXIT_SUCCESS;
#else

#endif
}

