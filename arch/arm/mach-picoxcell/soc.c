/*
 * Copyright (c) 2015, Michael van der Westhuizen <michael@smart-africa.com>
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */
#include "soc.h"

#include <linux/mfd/syscon.h>
#include <linux/regmap.h>
#include <linux/slab.h>

struct soc_device * __init picoxcell_soc_init()
{
	struct soc_device *soc_dev;
	struct regmap *socdata = NULL;
	struct soc_device_attribute *soc_dev_attr = NULL;
	u32 val;
	int ret;

	socdata =
		syscon_regmap_lookup_by_compatible("picochip,soc-identifier");
	if (IS_ERR(socdata))
		goto out;

	soc_dev_attr = kzalloc(sizeof(*soc_dev_attr), GFP_KERNEL);
	if (!soc_dev_attr)
		goto out;

	ret = regmap_read(socdata, 0x0, &val);
	if (ret < 0)
		goto out;

	if (val == 0x8003)
		soc_dev_attr->machine = kasprintf(GFP_KERNEL, "pc302");
	else if (val == 0x8007)
		soc_dev_attr->machine = kasprintf(GFP_KERNEL, "pc312");
	else if (val == 0x20)
		soc_dev_attr->machine = kasprintf(GFP_KERNEL, "pc313");
	else if (val == 0x21)
		soc_dev_attr->machine = kasprintf(GFP_KERNEL, "pc323");
	else if (val == 0x22)
		soc_dev_attr->machine = kasprintf(GFP_KERNEL, "pc333");
	else if (val == 0x30)
		soc_dev_attr->machine = kasprintf(GFP_KERNEL, "pc3008");
	else if (val == 0x31)
		soc_dev_attr->machine = kasprintf(GFP_KERNEL, "pc3032");

	if (!soc_dev_attr->machine)
		goto out;

	soc_dev_attr->family = kasprintf(GFP_KERNEL, "picoXcell");
	if (!soc_dev_attr->family)
		goto out;

	ret = regmap_read(socdata, 0x4, &val);
	if (ret < 0)
		goto out;

	soc_dev_attr->revision = kasprintf(GFP_KERNEL, "0x%08x", val);
	if (!soc_dev_attr->revision)
		goto out;

	soc_dev = soc_device_register(soc_dev_attr);
	if (IS_ERR(soc_dev))
		goto out;

	dev_info(soc_device_to_device(soc_dev),
		 "Detected %s %s, revision %s\n",
		 soc_dev_attr->family, soc_dev_attr->machine,
		 soc_dev_attr->revision);

	regmap_exit(socdata);
	return soc_dev;

out:
	if (soc_dev_attr) {
		kfree(soc_dev_attr->revision);
		kfree(soc_dev_attr->family);
		kfree(soc_dev_attr->machine);
	}

	kfree(soc_dev_attr);
	regmap_exit(socdata);

	return NULL;
}
