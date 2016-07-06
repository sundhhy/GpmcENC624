/*
 * debug.h
 *
 *  Created on: 2016-7-1
 *      Author: Administrator
 */

#ifndef DEBUG_H_
#define DEBUG_H_

#define	DEBUG_DRIVE_PILING				//�����ܹ����ԣ��������Ӳ������������д�׮
#define DEBUG_SWITCH
#include <stdio.h>

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


#endif /* DEBUG_H_ */
