/*
 * hardware_cfg.h
 *
 *  Created on: 2016-6-30
 *      Author: Administrator
 *      硬件驱动的配置项结构体定义
 */

#ifndef HARDWARE_CFG_H_
#define HARDWARE_CFG_H_
#include <stdint.h>
typedef struct gpmc_config {
	char		chip_instance;
	char		device_size;
	char		device_type;
	char 		use_dma;
	uint32_t	base_address;
	uint32_t	mask_address;
}gpmc_chip_cfg;

#endif /* HARDWARE_CFG_H_ */
