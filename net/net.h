/*
 * net_method.h
 *
 *  Created on: 2016-7-4
 *      Author: Administrator
 */

#ifndef NET_H_
#define NET_H_
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include "error_head.h"
#include <sys/types.h>
#include "lw_oopc.h"
#include "am335x_gpmc.h"
#include "am335x_gpio.h"

#define MAC_LEN		6
#define RX_EVENT	1
#define TX_EVENT	2

typedef struct
{
	char addr[6];
}MacAddr;

typedef struct
{
	uint16_t w[3];
}MacAddr_u16;

typedef struct
{
	uint_t		chunkCount;

}NetBuffer;

typedef struct
{
	char					name[32];

	uint8_t					*ethFrame;
	void 					*nicContext;

	unsigned int			macFilterSize;
	MacAddr					*macFilter;

	MacAddr_u16				macAddr;

	Drive_Gpmc				*busDriver;
	Drive_Gpio				*extIntDriver;

	bool					speed100;
	bool					fullDuplex;
	bool					linkState;
	uint8_t					instance;

	//todo ����ĳ�Ա��������������ʱ���ģ����ڲ���ȷ����16-07-04
	int 					nicRxEvent;
	int						nicTxEvent;

	//debug info
	uint32_t				rx_event_handle_count;
	uint32_t				tx_event_handle_count;

}NetInterface;

err_t macCompAddr( MacAddr_u16 *mac1, MacAddr_u16 *mac2);
err_t nicNotifyLinkChange( NetInterface *Inet );
err_t nicProcessPacket( NetInterface * Inet, uint8_t *frame, int len);
int netBufferGetLength( const NetBuffer *net_buf);
#endif /* NET_METHOD_H_ */