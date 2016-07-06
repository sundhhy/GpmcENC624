/*
 * hardware_cfg.c
 *
 *  Created on: 2016-6-30
 *      Author: Administrator
 *      硬件的配置数据
 */

#include "hardware_cfg.h"
#include "am335x_gpmc.h"

gpmc_chip_cfg Gpmc_cfg_c2 ={
		0x19000000,
		0x1000000,		//16M
		0x8000,
		2,
		DEVICE_SIZE_8,
		NOR_FLASH_LIKE,
		NON_MULTIPLEXED,
//		0,
//		0,
		READ_SIGNAL_ACCESS,
		READ_ASYNCHRONOUS,
		WRITE_SIGNAL_ACCESS,
		WRITE_ASYNCHRONOUS,
		false,

};

gpmc_chip_cfg Gpmc_cfg_c3 ={
		0x1a000000,
		0x1000000,		//16M
		0x8000,
		3,
		DEVICE_SIZE_8,
		NOR_FLASH_LIKE,
		NON_MULTIPLEXED,
//		0,
//		0,
		READ_SIGNAL_ACCESS,
		READ_ASYNCHRONOUS,
		WRITE_SIGNAL_ACCESS,
		WRITE_ASYNCHRONOUS,
		false,


};

gpio_cfg Enc624_extern_intr ={


};


