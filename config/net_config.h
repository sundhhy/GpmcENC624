/*
 * mem_config.h
 *
 *  Created on: 2016-7-14
 *      Author: Administrator
 */


#ifndef MEM_CONFIG_H_
#define MEM_CONFIG_H_
#include <stdint.h>
#include "mem.h"

#define  CONNECT_INFO_NUM		10
#define NET_INSTANCE_NUM		2

#ifndef LITTLE_ENDIAN
#define LITTLE_ENDIAN 1234
#endif

#ifndef BYTE_ORDER
#define BYTE_ORDER LITTLE_ENDIAN
#endif


//stats
#define LWIP_STATS			1
#define ETHARP_STATS		1



typedef unsigned    char    u8_t;
typedef signed      char    s8_t;
typedef unsigned    short   u16_t;
typedef signed      short   s16_t;
typedef unsigned    int    u32_t;
typedef signed      int    s32_t;
typedef u32_t           mem_ptr_t;

/** flag for LWIP_DEBUGF to enable that debug message */
#define LWIP_DBG_ON            0x80U
/** flag for LWIP_DEBUGF to disable that debug message */
#define LWIP_DBG_OFF           0x00U

#ifndef PBUF_DEBUG
#define PBUF_DEBUG                      LWIP_DBG_OFF
#endif
#define LWIP_DBG_LEVEL_SERIOUS 0x02 /* memory allocation failures, ... */
#define LWIP_DBG_TRACE         0x40U

#define ETHARP_DEBUG                    LWIP_DBG_OFF


#define MEMP_NUM_RAW_PCB                4


#define MEM_ALIGNMENT                   4

#ifndef ETH_PAD_SIZE
#define ETH_PAD_SIZE                    0
#endif

#define LWIP_DBG_LEVEL_ALL     0x00
#define LWIP_DBG_LEVEL_OFF     LWIP_DBG_LEVEL_ALL /* compatibility define only */
#define LWIP_DBG_LEVEL_WARNING 0x01 /* bad checksums, dropped packets, ... */
#define LWIP_DBG_LEVEL_SERIOUS 0x02 /* memory allocation failures, ... */
#define LWIP_DBG_LEVEL_SEVERE  0x03
#define LWIP_DBG_MASK_LEVEL    0x03
#ifndef LWIP_DBG_MIN_LEVEL
#define LWIP_DBG_MIN_LEVEL              LWIP_DBG_LEVEL_ALL
#endif

#define MEMCPY(dst,src,len)             memcpy(dst,src,len)
#define SMEMCPY(dst,src,len)            memcpy(dst,src,len)
/*
   ----------------------------------
   ---------- Pbuf options ----------
   ----------------------------------
*/
/**
 * PBUF_LINK_HLEN: the number of bytes that should be allocated for a
 * link level header. The default is 14, the standard value for
 * Ethernet.
 */
#ifndef PBUF_LINK_HLEN
#define PBUF_LINK_HLEN                  (14 + ETH_PAD_SIZE)
#endif

/**
 * PBUF_POOL_BUFSIZE: the size of each pbuf in the pbuf pool. The default is
 * designed to accomodate single full size TCP frame in one pbuf, including
 * TCP_MSS, IP header, and link header.
 */
#ifndef PBUF_POOL_BUFSIZE
#define PBUF_POOL_BUFSIZE               LWIP_MEM_ALIGN_SIZE(1518)
#endif

/**
 * PBUF_POOL_SIZE: the number of buffers in the pbuf pool.
 */
#ifndef PBUF_POOL_SIZE
#define PBUF_POOL_SIZE                  16
#endif



#define LWIP_NETIF_LOOPBACK				0

#define	PRE_NET_NAME					"eth"

#endif /* MEM_CONFIG_H_ */
