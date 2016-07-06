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
#include <stdbool.h>
typedef struct gpmc_config {
	uint32_t	base_address;
	uint32_t	mask_address;
	uint32_t	enc624_addr_len;
	// GPMC_CONFIG1_n
	uint8_t		chip_instance;
	char		device_size;
	char		device_type;
	char		mux_add_data;
//	char		gpmc_fclk_divider;
//	char		clk_activation_time;
	char		read_multiple;
	char		read_type;
	char		write_multiple;
	char		write_type;
	bool 		use_dma;

//	char		rese[2];



}gpmc_chip_cfg;

typedef struct gpio_config {

}gpio_cfg;



extern gpio_cfg Enc624_extern_intr;
extern gpmc_chip_cfg Gpmc_cfg_c2;
extern gpmc_chip_cfg Gpmc_cfg_c3;
#endif /* HARDWARE_CFG_H_ */
