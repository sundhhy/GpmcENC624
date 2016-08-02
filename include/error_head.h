/*
 * error_head.h
 *
 *  Created on: 2016-7-1
 *      Author: Administrator
 *      程序的错误码定义
 */

#ifndef ERROR_HEAD_H_
#define ERROR_HEAD_H_

#ifndef __err_t_defined
typedef int err_t;
#define __err_t_defined
#endif
#include <stdlib.h>


/* Definitions for error constants. */

#define ERR_OK          0    /* No error, everything OK. */
#define ERR_MEM        -1    /* Out of memory error.     */
#define ERR_BUF        -2    /* Buffer error.            */
#define ERR_TIMEOUT    -3    /* Timeout.                 */
#define ERR_RTE        -4    /* Routing problem.         */
#define ERR_INPROGRESS -5    /* Operation in progress    */
#define ERR_VAL        -6    /* Illegal value.           */
#define ERR_WOULDBLOCK -7    /* Operation would block.   */
#define ERR_USE        -8    /* Address in use.          */
#define ERR_ISCONN     -9    /* Already connected.       */

#define ERR_IS_FATAL(e) ((e) < ERR_ISCONN)

#define ERR_ABRT       -10   /* Connection aborted.      */
#define ERR_RST        -11   /* Connection reset.        */
#define ERR_CLSD       -12   /* Connection closed.       */
#define ERR_CONN       -13   /* Not connected.           */

#define ERR_ARG        -14   /* Illegal argument.        */

#define ERR_IF         -15   /* Low-level netif error    */



#define ERR_UNKOWN     			-16    /* 未知错误      */
#define ERR_BAD_PARAMETER    	-17    /* 错误参数      */
#define ERR_ERROR_INDEX     	-18    /* 错误索引      */
#define ERR_UNINITIALIZED     	-19    /* 未初始化的变量或子系统      */
#define ERR_CATASTROPHIC_ERR	-20 /* 灾难性错误			*/
#define ERR_UNAVAILABLE			-21	  /* 不可获取的资源			*/
#define ERR_TIMEOUT				-22	  		/* 不可获取的资源			*/
#define ERR_BUSY     			-23    /*       */
/*
err_t :
MSB  ----------|----------- LSB
1  	|	模块标识  |	错误标识
1bit| 15b				| 16b


*/

#define MODULE_BITS	15
#define ERROR_BITS	16

#define ERROR_BIT		(1<<(MODULE_BITS + ERROR_BITS))
#define ERROR_BEGIN(_module_id)	((_module_id) << ERROR_BITS)
#define	ERROR_T(_module_error)	(ERROR_BIT | _module_error)

//获取错误标识
#define MODULE_ERROR(_error_t) ((_error_t) & ((1 << ERROR_BITS) - 1))
//获取模块标识
#define MODULE_ID(_error_t)		(((_error_t) & ~(ERROR_BIT)) >> ERROR_BITS)

#define INVERSE_ERROR(_error_t) ((_error_t) & ( ~(ERROR_BIT)))

//----------------------------------------------------------------------
//系统中的组件，两层结构
// 高4位表示层级 低四位表示层级中的序号   层级|序号
//----------------------------------------------------------------------
#define	APP_MAIN	0x00
#define HAL_ENC624	0x80
#define DRIVE_GPMC	0x90
#define DRIVE_GPIO	0x91
#define CHECK_ERROR( return_val, msg) { \
	if( return_val != 0) { \
		fprintf( stderr, "%s:%s \n", msg, strerror(return_val)); \
		exit(-1);	\
		}\
	}

#endif /* ERROR_HEAD_H_ */



