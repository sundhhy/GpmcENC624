/*
 * hal_config.h
 *
 *  Created on: 2016-7-1
 *      Author: Administrator
 */

#ifndef HAL_CONFIG_H_
#define HAL_CONFIG_H_
#include <stdint.h>

#define ENC_INTERFACE_PSP 0

struct hal_enc_cfg {
	char interface_type;
};

extern struct hal_enc_cfg Hal_enc_cfg;
#endif /* HAL_CONFIG_H_ */
