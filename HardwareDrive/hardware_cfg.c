/*
 * hardware_cfg.c
 *
 *  Created on: 2016-6-30
 *      Author: Administrator
 *      硬件的配置数据
 */

#include "hardware_cfg.h"
#include "am335x_gpmc.h"

gpmc_chip_cfg gpmc_cfg_c2 ={
		2,
		DEVICE_SIZE_8,
		0,
		0x19000000,
		0x1000000,		//16M

};

gpmc_chip_cfg gpmc_cfg_c3 ={
		3,
		DEVICE_SIZE_8,
		0,
		0x1a000000,
		0x1000000,		//16M

};
