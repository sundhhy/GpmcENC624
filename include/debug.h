/*
 * debug.h
 *
 *  Created on: 2016-7-1
 *      Author: Administrator
 */

#ifndef DEBUG_H_
#define DEBUG_H_

#define	DEBUG_DRIVE_LEVEL
#define DEBUG_SWITCH
#include <stdio.h>
#include <stdint.h>

#define NIC_TRACE_LEVEL			0
#define TRACE_LEVEL_DEBUG	1

#ifdef    DEBUG_SWITCH
#define TRACE_INFO(fmt,args...) printf(fmt, ##args)
#else
#define TRACE_INFO(fmt,args...) /*do nothing */
#endif

#define 	DEBUG_PRINT
#ifdef    DEBUG_PRINT
#define TRACE_DEBUG(fmt,args...) printf(fmt, ##args)
#else
#define TRACE_DEBUG(fmt,args...) /*do nothing */
#endif


typedef struct {
	uint32_t		irq_count[2];
	uint32_t		event_count[2];
	uint32_t		event_handle_count;
}Sys_deginfo;

extern Sys_deginfo		Dubug_info;
//#define DEBUG_ONLY_GPIO_INIT			//调试GPIO配置的时候出现SIGBUS的问题
#endif /* DEBUG_H_ */
