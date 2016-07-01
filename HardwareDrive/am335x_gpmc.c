/*
 * am335x_gpmc.c
 *
 *  Created on: 2016-6-30
 *      Author: Administrator
 */

#include "am335x_gpmc.h"
#include "hardware_cfg.h"
#include "am335x.h"
#include <sys/mman.h>

const uint16_t	Gpmc_config_offset[] = {
		OMAP2420_GPMC_CS0, OMAP2420_GPMC_CS1, OMAP2420_GPMC_CS2, OMAP2420_GPMC_CS3, OMAP2420_GPMC_CS4, OMAP2420_GPMC_CS5,\
		OMAP2420_GPMC_CS6,OMAP2420_GPMC_CS7
};

err_t gpmc_init(void *t, void *arg)
{
	Dev_Gpmc *cthis = (Dev_Gpmc *)t;

	cthis->cs_regoffset = Gpmc_config_offset[ cthis->config->chip_instance];

	cthis->gpmc_vbase =  mmap_device_io(OMAP3530_GPMC_SIZE, GPMC_BASE);
	if (cthis->gpmc_vbase == MAP_DEVICE_FAILED)
	{
		return ERROR_T( ERR_mapio_fail);
	}





}
