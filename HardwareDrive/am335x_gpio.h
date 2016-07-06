/*
 * am335x_gpio.h
 *
 *  Created on: 2016-7-4
 *      Author: Administrator
 */

#ifndef AM335X_GPIO_H_
#define AM335X_GPIO_H_
#include <stdint.h>
#include <arm/omap3530.h>
#include <arm/omap2420.h>
#include "lw_oopc.h"
#include "error_head.h"
#include "interface.h"
#include "hardware_cfg.h"


CLASS(Drive_Gpio)
{

//	IMPLEMENTS(IExternIntr);

	err_t 	(*init)(Drive_Gpio * );
	err_t	( *enableIrq)( Drive_Gpio *);
	err_t	( *disableIrq)( Drive_Gpio *);

	uintptr_t			gpio_vbase;
	gpio_cfg			*config;







};


typedef enum {
	//for drives management
	ERROR_GPIO_BEGIN = ERROR_BEGIN(DRIVE_GPIO),







}err_gpio_t;

#endif /* AM335X_GPIO_H_ */
