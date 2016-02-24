/*
 * Copyright (c) 2015 Michael van der Westhuizen <michael@smart-africa.com>
 * Copyright (c) 2011 Picochip Ltd., Jamie Iles
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * PLL clock and device tree mapping for the Intel picoXcell px3xx SoC
 *
 * Based on arch/arm/mach-picoxcell/clk.c from the linux-2.6-ji tree
 * found at git://github.com/jamieiles/linux-2.6-ji.git.
 */
#define pr_fmt(fmt) "pc3x3-pll: " fmt

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
#include <linux/slab.h>
#include <asm/delay.h>

#define PX3XX_PLL_CLKF_REG_OFFS		0x00
#define PX3XX_PLL_FREQ_SENSE_REG_OFFS	0x04
#define PX3XX_PLL_FREQ_SENSE_VALID	(1 << 29)
#define PX3XX_PLL_FREQ_SENSE_ACTIVE	(1 << 30)
#define PX3XX_PLL_FREQ_SENSE_START	(1 << 31)
#define PX3XX_PLL_FREQ_SENSE_FREQ_MASK	0x3FF
#define PX3XX_PLL_STEP			5000000

struct clk_pc3x3_pll {
	struct clk_hw   hw;
	void __iomem    *reg;
	unsigned long   min_freq;
	unsigned long   max_freq;
};

#define to_clk_pc3x3_pll(clk) container_of(clk, struct clk_pc3x3_pll, hw)

static unsigned long pc3x3_pll_recalc_rate(struct clk_hw *clk,
					   unsigned long parent_rate)
{
	unsigned int mhz = 0;
	unsigned long sense_val;
	struct clk_pc3x3_pll *pll = to_clk_pc3x3_pll(clk);

	while (0 == mhz) {
		do {
			writel(PX3XX_PLL_FREQ_SENSE_START,
			       pll->reg + PX3XX_PLL_FREQ_SENSE_REG_OFFS);

			/* Wait for the frequency sense to complete. */
			do {
				sense_val =
					readl(pll->reg +
					      PX3XX_PLL_FREQ_SENSE_REG_OFFS);
			} while ((sense_val & PX3XX_PLL_FREQ_SENSE_ACTIVE));
		} while (!(sense_val & PX3XX_PLL_FREQ_SENSE_VALID));

		/* The frequency sense returns the frequency in MHz. */
		mhz = (sense_val & PX3XX_PLL_FREQ_SENSE_FREQ_MASK);
	}

	return mhz * 1000000;
}

static long pc3x3_pll_round_rate(struct clk_hw *clk, unsigned long rate,
				 unsigned long *parent_rate)
{
	long ret;
	unsigned long offset;
	struct clk_pc3x3_pll *pll = to_clk_pc3x3_pll(clk);

	rate = clamp(rate, pll->min_freq, pll->max_freq);
	offset = rate % PX3XX_PLL_STEP;
	rate -= offset;

	if (offset > PX3XX_PLL_STEP - offset)
		ret = rate + PX3XX_PLL_STEP;
	else
		ret = rate;

	return ret;
}

static void pc3x3_pll_set(struct clk_hw *clk, unsigned long rate)
{
	struct clk_pc3x3_pll *pll = to_clk_pc3x3_pll(clk);
	unsigned long clkf = ((rate / 1000000) / 5) - 1;

	writel(clkf, pll->reg + PX3XX_PLL_CLKF_REG_OFFS);
	udelay(2);
}

static int pc3x3_pll_set_rate(struct clk_hw *clk, unsigned long target,
			      unsigned long parent_rate)
{
	unsigned long current_khz;

	target = pc3x3_pll_round_rate(clk, target, &parent_rate);
	parent_rate = target;
	target /= 1000;

	pr_debug("set cpu clock rate to %luKHz\n", target);

	/*
	 * We can only reliably step by 20% at a time. We may need to
	 * do this in several iterations.
	 */
	while ((current_khz =
		pc3x3_pll_recalc_rate(clk, parent_rate) / 1000) != target) {
		unsigned long next_step, next_target;

		if (target < current_khz) {
			next_step = current_khz - ((4 * current_khz) / 5);
			next_target = current_khz -
				min(current_khz - target, next_step);
			next_target = roundup(next_target * 1000,
					      PX3XX_PLL_STEP);
		} else {
			next_step = ((6 * current_khz) / 5) - current_khz;
			next_target =
				min(target - current_khz, next_step) +
				current_khz;
			next_target = ((next_target * 1000) / PX3XX_PLL_STEP) *
				PX3XX_PLL_STEP;
		}

		pc3x3_pll_set(clk, next_target);
	}

	return 0;
}

static const struct clk_ops pc3x3_pll_ops = {
	.recalc_rate	= pc3x3_pll_recalc_rate,
	.round_rate	= pc3x3_pll_round_rate,
	.set_rate	= pc3x3_pll_set_rate,
};

static struct clk *clk_register_pc3x3_pll(struct device *dev,
					  const char *name,
					  const char *parent_name,
					  unsigned long flags,
					  void __iomem *reg,
					  unsigned long min,
					  unsigned long max)
{
	struct clk_pc3x3_pll *pll;
	struct clk *clk;
	struct clk_init_data init;

	pll = kzalloc(sizeof(struct clk_pc3x3_pll), GFP_KERNEL);
	if (!pll) {
		pr_err("%s: could not allocate pll\n", __func__);
		return ERR_PTR(-ENOMEM);
	}

	init.name = name;
	init.ops = &pc3x3_pll_ops;
	init.flags = flags; /* | CLK_IS_BASIC; */ /* XXX */
	init.parent_names = (parent_name ? &parent_name: NULL);
	init.num_parents = (parent_name ? 1 : 0);

	/* struct clk_pc3x3_pll assignments */
	pll->reg = reg;
	pll->min_freq = min;
	pll->max_freq = max;
	pll->hw.init = &init;

	clk = clk_register(dev, &pll->hw);
	if (IS_ERR(clk))
		kfree(pll);

	return clk;
}


static void of_pc3x3_pll_setup(struct device_node *node)
{
	struct clk *clk;
	const char *clk_name = node->name;
	const char *parent_clk_name;
	void __iomem *reg;
	u32 min_freq;
	u32 max_freq;
	u8 clk_flags = 0;

	if (of_property_read_u32(node, "picochip,min-freq", &min_freq)) {
		pr_err("no picochip,min-freq for %s\n", node->full_name);
		return;
	}

	if (of_property_read_u32(node, "picochip,max-freq", &max_freq)) {
		pr_err("no picochip,max-freq for %s\n", node->full_name);
		return;
	}

	if (of_property_read_bool(node, "picochip,clk-no-disable"))
		clk_flags |= CLK_IGNORE_UNUSED;

	reg = of_iomap(node, 0);
	if (!reg) {
		pr_err("unable to map reg for %s\n", node->full_name);
		return;
	}

	parent_clk_name = of_clk_get_parent_name(node, 0);
	if (!parent_clk_name)
		clk_flags |= CLK_IS_ROOT;

	of_property_read_string(node, "clock-output-names", &clk_name);

	clk = clk_register_pc3x3_pll(NULL, clk_name, parent_clk_name,
				     clk_flags, reg, min_freq, max_freq);

	if (IS_ERR(clk)) {
		pr_err("failed to register clock %s for %s\n",
		       clk_name, node->full_name);
		iounmap(reg);
		return;
	}

	if (of_clk_add_provider(node, of_clk_src_simple_get, clk)) {
		pr_err("failed to add OF provider for clock %s (%s)\n",
		       clk_name, node->full_name);
		clk_unregister(clk);
		iounmap(reg);
		return;
	}
}
CLK_OF_DECLARE(picochip_pc3x3_pll, "picochip,pc3x3-pll", of_pc3x3_pll_setup);
