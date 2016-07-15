/*
 * memp_std.h
 *
 *  Created on: 2016-7-14
 *      Author: Administrator
 *
 *      用于为各种数据结构提供各种不同的定义
 */



#ifndef LWIP_PBUF_MEMPOOL
/* This treats "pbuf pools" just like any other pool.
 * Allocates buffers for a pbuf struct AND a payload size */
#define LWIP_PBUF_MEMPOOL(name, num, payload, desc) LWIP_MEMPOOL(name, num, (MEMP_ALIGN_SIZE(sizeof(struct pbuf)) + MEMP_ALIGN_SIZE(payload)), desc)
#endif /* LWIP_PBUF_MEMPOOL */

//LWIP_MEMPOOL(RAW_PCB,        MEMP_NUM_RAW_PCB,         sizeof(struct raw_pcb),        "RAW_PCB")





LWIP_PBUF_MEMPOOL(PBUF_POOL, PBUF_POOL_SIZE,           PBUF_POOL_BUFSIZE,             "PBUF_POOL")




#undef LWIP_MEMPOOL
#undef LWIP_PBUF_MEMPOOL
