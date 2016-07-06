/*
 * interface.h
 *
 *  Created on: 2016-7-1
 *      Author: Administrator
 *      系统使用的接口定义
 */

#ifndef INTERFACE_H_
#define INTERFACE_H_
#include "lw_oopc.h"
#include "stdint.h"
#include "error_head.h"
#include <stdbool.h>
#include <stdlib.h>

//分离一个16位数据的高8位和低8位
#define LSB(data) ( data & 0xff)
#define MSB(data) ( ( data >> 8) & 0xff)

#define TRUE 		true
#define FALSE		false
#define MIN(a,b)	min(a,b)

INTERFACE(IBusDrive)
{



	err_t 	(*init)(IBusDrive * , void * );

	err_t	(*assertCs)(IBusDrive *);
	err_t	(*deassertCs)(IBusDrive *);

	err_t	(*write_u8)(IBusDrive *, uint8_t );
	uint8_t	(*read_u8)(IBusDrive *, uint8_t );
//	error_t (*close)(IDriver *);
//	error_t (*read)(IDriver *, uint8_t *, int );
//	error_t (*write)(IDriver *,uint8_t *, int );
//	error_t (*ioctl)(IDriver *,int ,...);
//	error_t (*test)(IDriver *);
//	void (*err_handle)(IDriver *,error_t );
};

INTERFACE(IExternIntr)
{



	err_t 	(*init)(IExternIntr * );
	err_t	( *enableIrq)( IExternIntr *);
	err_t	( *disableIrq)( IExternIntr *);

};


#endif /* INTERFACE_H_ */
