/*
 * memp.h
 *
 *  Created on: 2016-7-14
 *      Author: Administrator
 */

#ifndef MEMP_H_
#define MEMP_H_
#include "net_config.h"

typedef enum {
#define LWIP_MEMPOOL(name,num,size,desc)  MEMP_##name,
//#define LWIP_PBUF_MEMPOOL(name,num,size,desc)  MEMP_##name,
#include "memp_std.h"
	MEMP_MAX,
} memp_t;

void
memp_init(void);

void *memp_malloc(memp_t type);
void
memp_free(memp_t type, void *mem);

#endif /* MEMP_H_ */
