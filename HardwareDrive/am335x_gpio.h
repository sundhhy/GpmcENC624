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
#include "am335x.h"

#define		LOW_LEVELDETECT 	0
#define		HIG_LEVELDETECT 	1
#define		RISINGDETECT 		2
#define		FALLINGDETECT 		3

#define 	GPIO_DETECT(n)   ( GPIO_LEVELDETECT0 + (n * 4))
#define 	GPIO_IRQSTATUS_SET(n)   (GPIO_IRQSTATUS_SET_0 + (n * 4))
#define 	GPIO_GPIO_IRQSTATUS(n)   (GPIO_IRQSTATUS_0 + (n * 4))

CLASS(Drive_Gpio)
{

//	IMPLEMENTS(IExternIntr);

	err_t 	(*init)(Drive_Gpio *);
	err_t	( *enableIrq)( Drive_Gpio *);
	err_t	( *disableIrq)( Drive_Gpio *);
	err_t	( *deatory)( Drive_Gpio *);

	bool	( *irq_handle)( void *);
	void	*irq_handle_arg;
	int					irq_id;
	uintptr_t			gpio_vbase;
	gpio_cfg			*config;








};


typedef enum {
	//for drives management
	ERROR_GPIO_BEGIN = ERROR_BEGIN(DRIVE_GPIO),

	gpio_init_mapio_fail,





}err_gpio_t;

const struct sigevent *gpioExtInteIsr (void *area, int id);
//extern struct sigevent	Gpio_event;
#endif /* AM335X_GPIO_H_ */
