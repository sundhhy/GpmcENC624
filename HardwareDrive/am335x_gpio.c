/*
 * am335x_gpio.c
 *
 *  Created on: 2016-7-4
 *      Author: Administrator
 */

#include "am335x_gpio.h"
#include "debug.h"

static err_t gpio_init(Drive_Gpio *t)
{
	Drive_Gpio 		*cthis = ( Drive_Gpio *)t ;
#ifdef DEBUG_DRIVE_PILING
	TRACE_INFO("Drive Piling :%s-%s-%d \r\n", __FILE__, __func__, __LINE__);
	return EXIT_SUCCESS;
#else

#endif
}

static err_t gpio_enableIrq(Drive_Gpio *t)
{
	Drive_Gpio 		*cthis = ( Drive_Gpio *)t ;
#ifdef DEBUG_DRIVE_PILING
	TRACE_INFO("Drive Piling :%s-%s-%d \r\n", __FILE__, __func__, __LINE__);
	return EXIT_SUCCESS;
#else

#endif
}

static err_t gpio_disableIrq(Drive_Gpio *t)
{
	Drive_Gpio 		*cthis = ( Drive_Gpio *)t ;
#ifdef DEBUG_DRIVE_PILING
	TRACE_INFO("Drive Piling :%s-%s-%d \r\n", __FILE__, __func__, __LINE__);
	return EXIT_SUCCESS;
#else

#endif
}


CTOR(Drive_Gpio)

FUNCTION_SETTING(init, gpio_init);
FUNCTION_SETTING(enableIrq, gpio_enableIrq);
FUNCTION_SETTING(disableIrq, gpio_disableIrq);
END_CTOR
