/*
 * Copyright (c) 2015 Michael van der Westhuizen <michael@smart-africa.com>
 * Copyright (c) 2011 Picochip Ltd., Jamie Iles
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * Gated clock device tree mapping for the Intel picoXcell px3xx SoC
 *
 * Based on arch/arm/mach-picoxcell/clk.c from the linux-2.6-ji tree
 * found at git://github.com/jamieiles/linux-2.6-ji.git.
 */
#define pr_fmt(fmt) "pc3x3-pclk: " fmt

#include <linux/clk.h>
#include <linux/clk-provider.h>
#include <linux/module.h>
#include <linux/io.h>
#include <linux/device.h>
#include <linux/err.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_device.h>
#include <linux/platform_device.h>

static DEFINE_SPINLOCK(pc3x3_pclk_gate_lock);

static struct of_device_id gated_clk_ids[] = {
	{ .compatible = "picochip,pc3x3-gated-clk" },
	{ }
};

static void of_pc3x3_pclk_setup(struct device_node *node)
{
	int ret;
	struct clk *clk;
	const char *clk_name = node->name;
	const char *parent_clk_name;
	u32 rate;
	u32 accuracy = 0;
	/* child gate clock variables */
	void __iomem *reg;
	struct device_node *np;
	u32 disable_bit;
	const char *child_clk_name;
	u8 clk_flags = 0;
	u8 clk_gate_flags = CLK_GATE_SET_TO_DISABLE;

	of_property_read_u32(node, "clock-accuracy", &accuracy);

	of_property_read_string(node, "clock-output-names", &clk_name);

	if (of_property_read_u32(node, "clock-frequency", &rate)) {
		pr_err("No clk-frequency for pc3x3 perhipheral clock %s\n",
		       clk_name);
		return;
	}

	parent_clk_name = of_clk_get_parent_name(node, 0);
	if (!parent_clk_name)
		clk_flags |= CLK_IS_ROOT;

	clk = clk_register_fixed_rate_with_accuracy(NULL, clk_name,
						    parent_clk_name,
						    clk_flags, rate,
						    accuracy);
	if (IS_ERR(clk)) {
		pr_err("Failed to register pc3x3 perhipheral clock %s\n",
		       clk_name);
		return;
	}

	ret = of_clk_add_provider(node, of_clk_src_simple_get, clk);
	if (ret) {
		pr_err("Failed to add OF clock provider for %s\n",
		       clk_name);
		clk_unregister(clk);
		return;
	}

	reg = of_iomap(node, 0);
	if (!reg) {
		pr_err("Failed to map clock muxing registers for %s\n",
		       clk_name);
		of_clk_del_provider(node);
		clk_unregister(clk);
		return;
	}

	for_each_child_of_node(node, np) {
		clk_flags = 0;

		if (!of_match_node(gated_clk_ids, np))
			continue;

		child_clk_name = np->name;
		of_property_read_string(np, "clock-output-names", &child_clk_name);

		if (of_property_read_u32(np, "picochip,clk-disable-bit",
					 &disable_bit)) {
			pr_err("no picochip,clk-disable-bit for %s\n",
			       child_clk_name);
			continue;
		}

		if (of_property_read_bool(np, "picochip,clk-no-disable"))
			clk_flags |= CLK_IGNORE_UNUSED;

		clk = clk_register_gate(NULL, child_clk_name, clk_name, clk_flags,
					reg, disable_bit, clk_gate_flags,
					&pc3x3_pclk_gate_lock);
		if (IS_ERR(clk)) {
			pr_err("Failed to register pc3x3 gated clock %s\n",
			       child_clk_name);
			continue;
		}

		ret = of_clk_add_provider(np, of_clk_src_simple_get, clk);
		if (ret) {
			pr_err("Failed to add OF clock provider for %s\n",
			       child_clk_name);
			clk_unregister(clk);
			continue;
		}
	}
}
CLK_OF_DECLARE(picoxcell_pc3x3_pclk, "picochip,pc3x3-pclk", of_pc3x3_pclk_setup);
