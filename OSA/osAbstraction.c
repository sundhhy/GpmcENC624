/*
 * osAbstraction.c
 *
 *  Created on: 2016-7-4
 *      Author: Administrator
 *      提供操作系统的服务支持
 */
#include "osAbstraction.h"
#include "debug.h"
#include "enc624j600.h"
#include "net.h"
static int RX_event_offset = ( int )&( ( (NetInterface *)0)->nicRxEvent);
static int TX_event_offset = ( int )&( ( (NetInterface *)0)->nicTxEvent);

err_t osSetEvent( void *event)
{
	NetInterface	*i_net;
	int offset = 0;

	int *this_event = (int *)event;
#ifdef DEBUG_OSA
	TRACE_INFO("Drive Piling :%s-%s-%d \r\n", __FILE__, __func__, __LINE__);
#endif

	if( *this_event == RX_EVENT)
	{
		offset = RX_event_offset;
		Dubug_info.event_count[0]++;
	}
	else if( *this_event == TX_EVENT)
	{
		offset = TX_event_offset;
		Dubug_info.event_count[1]++;
	}
	else
	{
		return EXIT_FAILURE;
	}

	i_net = ( NetInterface *) ( (int)event - offset);
	enc624j600Driver.EventHandler( i_net);
	return EXIT_SUCCESS;



}

int osSetEventFromIsr( void *event)
{
#ifdef DEBUG_OSA
	TRACE_INFO("Drive Piling :%s-%s-%d \r\n", __FILE__, __func__, __LINE__);
	return EXIT_SUCCESS;
#else
	NetInterface	*i_net;
	int offset = 0;

	int *this_event = (int *)event;
	if( *this_event == RX_EVENT)
	{
		offset = RX_event_offset;

	}
	else if( *this_event == TX_EVENT)
	{
		offset = TX_event_offset;

	}
	else
	{
		return EXIT_FAILURE;
	}

	i_net = ( NetInterface *) ( (int)event - offset);
	enc624j600Driver.EventHandler( i_net);
	return EXIT_SUCCESS;

#endif
}

