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
#include <pthread.h>
#include <atomic.h>


#define MAC_LEN		6
#define RX_EVENT	1
#define TX_EVENT	2



#define		ISR_TX_EVENT ( 1 << 0)
#define		ISR_RX_EVENT ( 1 << 1)
#define		ISR_LINK_STATUS_CHG ( 1 << 2)
#define		ISR_RECV_PACKET ( 1 << 3)

#define		ISR_ERROR ( 1 << 31)
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

	//todo 下面的成员的数据类型是临时填充的，现在并不确定。16-07-04
	int 					nicRxEvent;
	int						nicTxEvent;

	pthread_t				tid;
	volatile uint32_t		isr_status;

}NetInterface;

err_t macCompAddr( MacAddr_u16 *mac1, MacAddr_u16 *mac2);
err_t nicNotifyLinkChange( NetInterface *Inet );
err_t nicProcessPacket( NetInterface * Inet, uint8_t *frame, int len);
int netBufferGetLength( const NetBuffer *net_buf);
#endif /* NET_METHOD_H_ */
