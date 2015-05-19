/*
 * Copyright (c) 2011 Picochip Ltd., Jamie Iles
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * All enquiries to support@picochip.com
 */
#include <linux/delay.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_platform.h>
#include <linux/reboot.h>

#include <asm/mach/arch.h>
#include <asm/mach/map.h>

#include "soc.h"

static void __init picoxcell_dt_init(void)
{
	struct soc_device *soc;
	struct device *soc_dev = NULL;
	struct platform_device_info devinfo = { .name = "cpufreq-dt", };

	soc = picoxcell_soc_init();
	if (soc)
		soc_dev = soc_device_to_device(soc);

	of_platform_populate(NULL, of_default_bus_match_table, NULL, soc_dev);
	devinfo.parent = soc_dev;
	platform_device_register_full(&devinfo);
}

static const char *picoxcell_dt_match[] = {
	"picochip,pc3x2",
	"picochip,pc3x3",
	NULL
};

DT_MACHINE_START(PICOXCELL, "Intel/Picochip picoXcell")
	.init_machine   = picoxcell_dt_init,
	.dt_compat	= picoxcell_dt_match,
MACHINE_END
