/*
 * Copyright (c) 2015, Michael van der Westhuizen <michael@smart-africa.com>
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */
#ifndef __PICOXCELL_SOC_H
#define __PICOXCELL_SOC_H

#include <linux/sys_soc.h>

#if defined(CONFIG_SOC_PICOXCELL)
struct soc_device * __init picoxcell_soc_init(void);
#else
struct soc_device * __init picoxcell_soc_init(void) { return NULL; }
#endif

#endif
