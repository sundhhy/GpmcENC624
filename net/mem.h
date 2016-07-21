/*
 * mem.h
 *
 *  Created on: 2016-7-14
 *      Author: Administrator
 */

#ifndef MEM_H_
#define MEM_H_
#include <stdint.h>
#include "net_config.h"

typedef uint32_t mem_size_t;

#define LWIP_MEM_ALIGN_SIZE(size) (((size) + MEM_ALIGNMENT - 1) & ~(MEM_ALIGNMENT-1))
#define LWIP_MEM_ALIGN(addr) ((void *)(((mem_ptr_t)(addr) + MEM_ALIGNMENT - 1) & ~(mem_ptr_t)(MEM_ALIGNMENT-1)))
#endif /* MEM_H_ */
