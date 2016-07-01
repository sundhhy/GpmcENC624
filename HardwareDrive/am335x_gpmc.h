/*
 * am335x_gpmc.h
 *
 *  Created on: 2016-6-30
 *      Author: Administrator
 */

#ifndef AM335X_GPMC_H_
#define AM335X_GPMC_H_
#include <stdint.h>
#include <arm/omap3530.h>
#include <arm/omap2420.h>
#include "lw_oopc.h"
#include "error_head.h"

#define DEVICE_SIZE_8	0

#define DRIVE_GPMC	0x01

CLASS(Dev_Gpmc)
{

	uintptr_t			gpmc_vbase;
	gpmc_chip_cfg		*config;
	uint16_t			cs_regoffset;


	int (*init)(void* t, gpmc_chip_cfg *cfg);




};


typedef enum {
	//for drives management
	ERROR_MAIN = ERROR_BEGIN(DRIVE_GPMC),

	ERR_mapio_fail,






}err_gpmc_t;



#endif /* AM335X_GPMC_H_ */
