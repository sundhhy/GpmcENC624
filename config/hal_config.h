/*
 * hal_config.h
 *
 *  Created on: 2016-7-1
 *      Author: Administrator
 */

#ifndef HAL_CONFIG_H_
#define HAL_CONFIG_H_
#include <stdint.h>
#include "hardware_cfg.h"

#define ENC_INTERFACE_PSP 0
#define NET_INSTANCE_NUM		2 ///< ÍøÂçÐ¾Æ¬µÄÊýÁ¿
#define	PRE_NET_NAME			"eth"
#define NET_NAME_LEN			10

struct hal_enc_cfg {
	int 		interface_type;
	gpio_cfg	*extInt_cfg[2];
};

extern struct hal_enc_cfg Hal_enc_cfg;
#endif /* HAL_CONFIG_H_ */
