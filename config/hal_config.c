/*
 * hal_config.c
 *
 *  Created on: 2016-7-1
 *      Author: Administrator
 */

#include "hal_config.h"

struct hal_enc_cfg Hal_enc_cfg = {
		ENC_INTERFACE_PSP,
		{ &Enc624_extern_intr0, &Enc624_extern_intr1},
};
