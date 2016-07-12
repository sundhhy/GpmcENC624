/*
 * hardware_cfg.c
 *
 *  Created on: 2016-6-30
 *      Author: Administrator
 *      硬件的配置数据
 */

#include "hardware_cfg.h"
#include "am335x_gpmc.h"
#include "am335x_gpio.h"
#include "pin_mux.h"


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


//		group;
//		number;
//		intr_type;
//		intr_line;
//		debou_time
//		instance
//		pin_ctrl_off
gpio_cfg Enc624_extern_intr0 ={
		3,
		20,
		FALLINGDETECT,
		1,
		10,
		0,
		conf_mcasp0_axr1,

};
gpio_cfg Enc624_extern_intr1 ={
		0,
		7,
		FALLINGDETECT,
		1,
		10,
		1,
		conf_ecap0_in_pwm0_out,
};


