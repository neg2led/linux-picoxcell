/*
 * Copyright (c) 2015, Michael van der Westhuizen <michael@smart-africa.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#include <linux/pinctrl/pinctrl.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/platform_device.h>
#include <linux/pinctrl/machine.h>
#include <linux/pinctrl/pinctrl.h>
#include <linux/pinctrl/pinconf-generic.h>
#include <linux/pinctrl/pinconf.h>
#include <linux/pinctrl/pinmux.h>
#include <linux/pm.h>
#include <linux/slab.h>
#include <linux/regmap.h>
#include <linux/mfd/syscon.h>
#include <linux/stringify.h>
#include <linux/io.h>

#include "pinctrl-utils.h"
#include "core.h"

#include <dt-bindings/pinctrl/pc3x3.h>

#define PC3X3_PAI_OFFSET	0x00
#define PC3X3_EBI_OFFSET	0x04
#define PC3X3_DECODE_OFFSET	0x08
#define PC3X3_MISC_OFFSET	0x0c

#define PASTE_HELPER(a,b) a ## b
#define PASTE(a,b) PASTE_HELPER(a,b)

#define NUM_CONFLICT_PINS	16

struct pc3x3_pmx {
	struct device *dev;
	struct pinctrl_dev *pctl;
	void __iomem *virtbase;
	struct regmap *scr;
	struct regmap *shd;
	u32 gpio_conflict_pins[NUM_CONFLICT_PINS];
};

/*
 S: Shared GPIO Muxing Register
 C: System Configuration Register
 P: PAI GPIO Control Register
 E: EBI GPIO Control Register
 D: Decode GPIO Control Register
 M: Misc GPIO Control Register

S  C  P  E  D  M  Destination    Name              GPIO Source    SD Source    GPIO/SD Destination      Main Function
 0                PC3X3_BALL_D4  arm_gpio[0]     - arm_gpio[0]  - sd_gpio[16]  arm_gpio[0]/sd_gpio[16]
 1                PC3X3_BALL_E4  arm_gpio[1]     - arm_gpio[1]  - sd_gpio[17]  arm_gpio[1]/sd_gpio[17]
 2                PC3X3_BALL_D3  arm_gpio[2]     - arm_gpio[2]  - sd_gpio[18]  arm_gpio[2]/sd_gpio[18]
 3                PC3X3_BALL_E3  arm_gpio[3]     - arm_gpio[3]  - sd_gpio[19]  arm_gpio[3]/sd_gpio[19]
 4        8       PC3X3_BALL_A3  ebi_addr[22]    - arm_gpio[4]  - sd_gpio[20]  arm_gpio[4]/sd_gpio[20]  ebi_addr[22]
 4     0          PC3X3_BALL_U7  pai_tx_data[0]  - arm_gpio[4]  - sd_gpio[20]  arm_gpio[4]/sd_gpio[20]  pai_tx_data[0]
 5        9       PC3X3_BALL_B3  ebi_addr[23]    - arm_gpio[5]  - sd_gpio[21]  arm_gpio[5]/sd_gpio[21]  ebi_addr[23]
 5     1          PC3X3_BALL_Y6  pai_tx_data[1]  - arm_gpio[5]  - sd_gpio[21]  arm_gpio[5]/sd_gpio[21]  pai_tx_data[1]
 6       10       PC3X3_BALL_C3  ebi_addr[24]    - arm_gpio[6]  - sd_gpio[22]  arm_gpio[6]/sd_gpio[22]  ebi_addr[24]
 6     2          PC3X3_BALL_W6  pai_tx_data[2]  - arm_gpio[6]  - sd_gpio[22]  arm_gpio[6]/sd_gpio[22]  pai_tx_data[2]
 7       11       PC3X3_BALL_B2  ebi_addr[25]    - arm_gpio[7]  - sd_gpio[23]  arm_gpio[7]/sd_gpio[23]  ebi_addr[25]
 7     3          PC3X3_BALL_V6  pai_tx_data[3]  - arm_gpio[7]  - sd_gpio[23]  arm_gpio[7]/sd_gpio[23]  pai_tx_data[3]
 8                PC3X3_BALL_N4  shd_gpio        - arm_gpio[8]  - sd_gpio[8]   arm_gpio[8]/sd_gpio[8]   shd_gpio
 9                PC3X3_BALL_P3  boot_mode[0]    - arm_gpio[9]  - sd_gpio[9]   arm_gpio[9]/sd_gpio[9]   boot_mode[0] (sampled at PoR only)
10                PC3X3_BALL_P2  boot_mode[1]    - arm_gpio[10] - sd_gpio[10]  arm_gpio[10]/sd_gpio[10] boot_mode[1] (sampled at PoR only)
11                PC3X3_BALL_P16 sdram_speed_sel - arm_gpio[11] - sd_gpio[11]  arm_gpio[11]/sd_gpio[11] sdram_speed_sel (sampled at PoR only)
12                PC3X3_BALL_P17 mii_rev_en      - arm_gpio[12] - sd_gpio[12]  arm_gpio[12]/sd_gpio[12] mii_rev_en (sampled at PoR only)
13                PC3X3_BALL_T20 mii_rmii_en     - arm_gpio[13] - sd_gpio[13]  arm_gpio[13]/sd_gpio[13] mii_rmii_en (sampled at PoR only)
14                PC3X3_BALL_T18 mii_speed_sel   - arm_gpio[14] - sd_gpio[14]  arm_gpio[14]/sd_gpio[14] mii_speed_sel (sampled at PoR only)
15       12       PC3X3_BALL_M1  ebi_addr[26]    - arm_gpio[15] - sd_gpio[15]  arm_gpio[15]/sd_gpio[15] ebi_addr[26]
16  7             PC3X3_BALL_P1  sd_gpio[0]/FSYN - arm_gpio[16] - sd_gpio[0]   arm_gpio[16]/sd_gpio[0]  FSYN
17                PC3X3_BALL_N1  sd_gpio[1]      - arm_gpio[17] - sd_gpio[1]   arm_gpio[17]/sd_gpio[1]
18                PC3X3_BALL_N2  sd_gpio[2]      - arm_gpio[18] - sd_gpio[2]   arm_gpio[18]/sd_gpio[2]
19                PC3X3_BALL_N3  sd_gpio[3]      - arm_gpio[19] - sd_gpio[3]   arm_gpio[19]/sd_gpio[3]
20        4       PC3X3_BALL_M2  ebi_addr[18]    - arm_gpio[20] - sd_gpio[4]   arm_gpio[20]/sd_gpio[4]  ebi_addr[18]
20     8          PC3X3_BALL_U12 pai_rx_data[0]  - arm_gpio[20] - sd_gpio[4]   arm_gpio[20]/sd_gpio[4]  pai_rx_data[0]
21        5       PC3X3_BALL_M3  ebi_addr[19]    - arm_gpio[21] - sd_gpio[5]   arm_gpio[21]/sd_gpio[5]  ebi_addr[19]
21     9          PC3X3_BALL_Y11 pai_rx_data[1]  - arm_gpio[21] - sd_gpio[5]   arm_gpio[21]/sd_gpio[5]  pai_rx_data[1]
22        6       PC3X3_BALL_M4  ebi_addr[20]    - arm_gpio[22] - sd_gpio[6]   arm_gpio[22]/sd_gpio[6]  ebi_addr[20]
22    10          PC3X3_BALL_W11 pai_rx_data[2]  - arm_gpio[22] - sd_gpio[6]   arm_gpio[22]/sd_gpio[6]  pai_rx_data[2]
23        7       PC3X3_BALL_L1  ebi_addr[21]    - arm_gpio[23] - sd_gpio[7]   arm_gpio[23]/sd_gpio[7]  ebi_addr[21]
23    11          PC3X3_BALL_V11 pai_rx_data[3]  - arm_gpio[23] - sd_gpio[7]   arm_gpio[23]/sd_gpio[7]  pai_rx_data[3]
       4          PC3X3_BALL_Y5  pai_tx_data[4]  - arm_gpio[24] -              arm_gpio[24]             pai_tx_data[4]
       5          PC3X3_BALL_W5  pai_tx_data[5]  - arm_gpio[25] -              arm_gpio[25]             pai_tx_data[5]
       6          PC3X3_BALL_Y4  pai_tx_data[6]  - arm_gpio[26] -              arm_gpio[26]             pai_tx_data[6]
       7          PC3X3_BALL_W4  pai_tx_data[7]  - arm_gpio[27] -              arm_gpio[27]             pai_tx_data[7]
      12          PC3X3_BALL_U11 pai_rx_data[4]  - arm_gpio[28] -              arm_gpio[28]             pai_rx_data[4]
      13          PC3X3_BALL_Y10 pai_rx_data[5]  - arm_gpio[29] -              arm_gpio[29]             pai_rx_data[5]
      14          PC3X3_BALL_W10 pai_rx_data[6]  - arm_gpio[30] -              arm_gpio[30]             pai_rx_data[6]
      15          PC3X3_BALL_V10 pai_rx_data[7]  - arm_gpio[31] -              arm_gpio[31]             pai_rx_data[7]
          0       PC3X3_BALL_L2  ebi_addr[14]    - arm_gpio[32] -              arm_gpio[32]             ebi_addr[14]
          1       PC3X3_BALL_L4  ebi_addr[15]    - arm_gpio[33] -              arm_gpio[33]             ebi_addr[15]
          2       PC3X3_BALL_K4  ebi_addr[16]    - arm_gpio[34] -              arm_gpio[34]             ebi_addr[16]
          3       PC3X3_BALL_K3  ebi_addr[17]    - arm_gpio[35] -              arm_gpio[35]             ebi_addr[17]
    8        0    PC3X3_BALL_F3  ebi_decode[0]_n - arm_gpio[36] -              arm_gpio[36]             ebi_decode[0]_n
    9        1    PC3X3_BALL_F2  ebi_decode[1]_n - arm_gpio[37] -              arm_gpio[37]             ebi_decode[1]_n
   10        2    PC3X3_BALL_E2  ebi_decode[2]_n - arm_gpio[38] -              arm_gpio[38]             ebi_decode[2]_n
   11        3    PC3X3_BALL_D1  ebi_decode[3]_n - arm_gpio[39] -              arm_gpio[39]             ebi_decode[3]_n
                0 PC3X3_BALL_C2  spi_clk         - arm_gpio[40] -              arm_gpio[40]             spi_clk
                0 PC3X3_BALL_C1  spi_data_in     - arm_gpio[41] -              arm_gpio[41]             spi_data_in
                0 PC3X3_BALL_D2  spi_data_out    - arm_gpio[42] -              arm_gpio[42]             spi_data_out
   13             PC3X3_BALL_W19 mii_tx_data[2]  - arm_gpio[43] -              arm_gpio[43]             mii_tx_data[2]
   13             PC3X3_BALL_V16 mii_tx_data[3]  - arm_gpio[44] -              arm_gpio[44]             mii_tx_data[3]
   13             PC3X3_BALL_U18 mii_rx_data[2]  - arm_gpio[45] -              arm_gpio[45]             mii_rx_data[2]
   13             PC3X3_BALL_U17 mii_rx_data[3]  - arm_gpio[46] -              arm_gpio[46]             mii_rx_data[3]
   13             PC3X3_BALL_R17 mii_col         - arm_gpio[47] -              arm_gpio[47]             mii_col
   13             PC3X3_BALL_U20 mii_crs         - arm_gpio[48] -              arm_gpio[48]             mii_crs
   13             PC3X3_BALL_W18 mii_tx_clk      - arm_gpio[49] -              arm_gpio[49]             mii_tx_clk
                1 PC3X3_BALL_Y12 max_tx_ctrl     - arm_gpio[50] -              arm_gpio[50]             max_tx_ctrl
                1 PC3X3_BALL_W15 max_clk_ref     - arm_gpio[51] -              arm_gpio[51]             max_clk_ref
                1 PC3X3_BALL_W12 max_trig_clk    - arm_gpio[52] -              arm_gpio[52]             max_trig_clk
         13       PC3X3_BALL_K1  ebi_clk         - arm_gpio[53] -              arm_gpio[53]             ebi_clk
                  PC3X3_BALL_R1  boot_error      - arm_gpio[54] -              arm_gpio[54]             boot_error
 */

#define MUX_CAP_SHD_GPIO	(1U << 31)
#define MUX_CAP_SYSCONF		(1U << 30)
#define MUX_CAP_PAI		(1U << 29)
#define MUX_CAP_PAI_EBI		(1U << 28)
#define MUX_CAP_EBI		(1U << 27)
#define MUX_CAP_EBI_PAI		(1U << 26)
#define MUX_CAP_DECODE		(1U << 25)
#define MUX_CAP_MISC		(1U << 24)
#define MUX_CAP_MII		(1U << 23)
#define MUX_CAP_FSYN		(1U << 22)
#define MUX_CAP_BERR		(1U << 21)
#define MUX_OPT_GROUP		(1U <<  1)
#define MUX_OPT_RO		(1U <<  0)

/* pin capabilities */
struct pc3x3_pin_config
{
	u32 capabilities;
	u32 shd_bit;
	u32 scr_bit;
	u32 pai_bit;
	u32 ebi_bit;
	u32 decode_bit;
	u32 misc_bit;
	u32 gpio_conflict;
};

#define PIN_HAS_DRV_DATA(P)					\
	((P) && (P)->drv_data)
#define PIN_TO_DRV_DATA(P)					\
	((const struct pc3x3_pin_config*)(P)->drv_data)
#define PIN_HAS_CAP(P, CAP)					\
	(PIN_TO_DRV_DATA((P))->capabilities & MUX_CAP_ ## CAP)
#define PIN_HAS_OPT(P, OPT)					\
	(PIN_TO_DRV_DATA((P))->capabilities & MUX_OPT_ ## CAP)
#define PIN_COULD_CONFLICT(P)					\
	(PIN_HAS_CAP((P), EBI_PAI) || PIN_HAS_CAP((P), PAI_EBI))
#define PIN_BIT(P, B)						\
	PIN_TO_DRV_DATA((P))-> B ## _bit

#define MPC_NAME(A) muxable_pin_config_##A

#define PC3X3_MUX_PIN(B, CAPS, SHD, SCR, PAI, EBI, DEC, MSC, C)	\
static const struct pc3x3_pin_config MPC_NAME(B) = {		\
	.capabilities	= CAPS,					\
	.shd_bit	= SHD,					\
	.scr_bit	= SCR,					\
	.pai_bit	= PAI,					\
	.ebi_bit	= EBI,					\
	.decode_bit	= DEC,					\
	.misc_bit	= MSC,					\
	.gpio_conflict	= C					\
}

#define CAPS_SHD		MUX_CAP_SHD_GPIO
#define CAPS_SHD_EP		(MUX_CAP_SHD_GPIO|MUX_CAP_EBI|MUX_CAP_EBI_PAI)
#define CAPS_SHD_PE		(MUX_CAP_SHD_GPIO|MUX_CAP_PAI|MUX_CAP_PAI_EBI)
#define CAPS_SHD_E		(MUX_CAP_SHD_GPIO|MUX_CAP_EBI)
#define CAPS_E			MUX_CAP_EBI
#define CAPS_P			MUX_CAP_PAI
#define CAPS_MII		(MUX_CAP_MII|MUX_CAP_SYSCONF|MUX_OPT_RO|MUX_OPT_GROUP)
#define CAPS_SHD_F		(MUX_CAP_SHD_GPIO|MUX_CAP_FSYN)
#define CAPS_DECODE		(MUX_CAP_SYSCONF|MUX_CAP_DECODE)
#define CAPS_MAXIM		(MUX_CAP_MISC|MUX_OPT_GROUP)
#define CAPS_SSI		(MUX_CAP_MISC|MUX_OPT_GROUP)
#define CAPS_BERR		(MUX_CAP_BERR|MUX_OPT_RO)

#define PIN_IS_CAP(P, CAP)					\
	(PIN_TO_DRV_DATA((P))->capabilities == CAPS_ ## CAP)

/*            Ball            Capabilities SHD SCR PAI EBI DEC MSC Conflict */
PC3X3_MUX_PIN(PC3X3_BALL_D4,  CAPS_SHD,     0,  0,  0,  0,  0,  0, 0);
PC3X3_MUX_PIN(PC3X3_BALL_E4,  CAPS_SHD,     1,  0,  0,  0,  0,  0, 0);
PC3X3_MUX_PIN(PC3X3_BALL_D3,  CAPS_SHD,     2,  0,  0,  0,  0,  0, 0);
PC3X3_MUX_PIN(PC3X3_BALL_E3,  CAPS_SHD,     3,  0,  0,  0,  0,  0, 0);
PC3X3_MUX_PIN(PC3X3_BALL_A3,  CAPS_SHD_EP,  4,  0,  0,  8,  0,  0, PC3X3_BALL_U7);
PC3X3_MUX_PIN(PC3X3_BALL_U7,  CAPS_SHD_PE,  4,  0,  0,  8,  0,  0, PC3X3_BALL_A3);
PC3X3_MUX_PIN(PC3X3_BALL_B3,  CAPS_SHD_EP,  5,  0,  1,  9,  0,  0, PC3X3_BALL_Y6);
PC3X3_MUX_PIN(PC3X3_BALL_Y6,  CAPS_SHD_PE,  5,  0,  1,  9,  0,  0, PC3X3_BALL_B3);
PC3X3_MUX_PIN(PC3X3_BALL_C3,  CAPS_SHD_EP,  6,  0,  2, 10,  0,  0, PC3X3_BALL_W6);
PC3X3_MUX_PIN(PC3X3_BALL_W6,  CAPS_SHD_PE,  6,  0,  2, 10,  0,  0, PC3X3_BALL_C3);
PC3X3_MUX_PIN(PC3X3_BALL_B2,  CAPS_SHD_EP,  7,  0,  3, 11,  0,  0, PC3X3_BALL_V6);
PC3X3_MUX_PIN(PC3X3_BALL_V6,  CAPS_SHD_PE,  7,  0,  3, 11,  0,  0, PC3X3_BALL_B2);
PC3X3_MUX_PIN(PC3X3_BALL_N4,  CAPS_SHD,     8,  0,  0,  0,  0,  0, 0);
PC3X3_MUX_PIN(PC3X3_BALL_P3,  CAPS_SHD,     9,  0,  0,  0,  0,  0, 0);
PC3X3_MUX_PIN(PC3X3_BALL_P2,  CAPS_SHD,    10,  0,  0,  0,  0,  0, 0);
PC3X3_MUX_PIN(PC3X3_BALL_P16, CAPS_SHD,    11,  0,  0,  0,  0,  0, 0);
PC3X3_MUX_PIN(PC3X3_BALL_P17, CAPS_SHD,    12,  0,  0,  0,  0,  0, 0);
PC3X3_MUX_PIN(PC3X3_BALL_T20, CAPS_SHD,    13,  0,  0,  0,  0,  0, 0);
PC3X3_MUX_PIN(PC3X3_BALL_T18, CAPS_SHD,    14,  0,  0,  0,  0,  0, 0);
PC3X3_MUX_PIN(PC3X3_BALL_M1,  CAPS_SHD_E,  15,  0,  0, 12,  0,  0, 0);
PC3X3_MUX_PIN(PC3X3_BALL_P1,  CAPS_SHD_F,  16,  7,  0,  0,  0,  0, 0);
PC3X3_MUX_PIN(PC3X3_BALL_N1,  CAPS_SHD,    17,  0,  0,  0,  0,  0, 0);
PC3X3_MUX_PIN(PC3X3_BALL_N2,  CAPS_SHD,    18,  0,  0,  0,  0,  0, 0);
PC3X3_MUX_PIN(PC3X3_BALL_N3,  CAPS_SHD,    19,  0,  0,  0,  0,  0, 0);
PC3X3_MUX_PIN(PC3X3_BALL_M2,  CAPS_SHD_EP, 20,  0,  8,  4,  0,  0, PC3X3_BALL_U12);
PC3X3_MUX_PIN(PC3X3_BALL_U12, CAPS_SHD_PE, 20,  0,  8,  4,  0,  0, PC3X3_BALL_M2);
PC3X3_MUX_PIN(PC3X3_BALL_M3,  CAPS_SHD_EP, 21,  0,  9,  5,  0,  0, PC3X3_BALL_Y11);
PC3X3_MUX_PIN(PC3X3_BALL_Y11, CAPS_SHD_PE, 21,  0,  9,  5,  0,  0, PC3X3_BALL_M3);
PC3X3_MUX_PIN(PC3X3_BALL_M4,  CAPS_SHD_EP, 22,  0, 10,  6,  0,  0, PC3X3_BALL_W11);
PC3X3_MUX_PIN(PC3X3_BALL_W11, CAPS_SHD_PE, 22,  0, 10,  6,  0,  0, PC3X3_BALL_M4);
PC3X3_MUX_PIN(PC3X3_BALL_L1,  CAPS_SHD_EP, 23,  0, 11,  7,  0,  0, PC3X3_BALL_V11);
PC3X3_MUX_PIN(PC3X3_BALL_V11, CAPS_SHD_PE, 23,  0, 11,  7,  0,  0, PC3X3_BALL_L1);
PC3X3_MUX_PIN(PC3X3_BALL_Y5,  CAPS_P,       0,  0,  4,  0,  0,  0, 0);
PC3X3_MUX_PIN(PC3X3_BALL_W5,  CAPS_P,       0,  0,  5,  0,  0,  0, 0);
PC3X3_MUX_PIN(PC3X3_BALL_Y4,  CAPS_P,       0,  0,  6,  0,  0,  0, 0);
PC3X3_MUX_PIN(PC3X3_BALL_W4,  CAPS_P,       0,  0,  7,  0,  0,  0, 0);
PC3X3_MUX_PIN(PC3X3_BALL_U11, CAPS_P,       0,  0, 12,  0,  0,  0, 0);
PC3X3_MUX_PIN(PC3X3_BALL_Y10, CAPS_P,       0,  0, 13,  0,  0,  0, 0);
PC3X3_MUX_PIN(PC3X3_BALL_W10, CAPS_P,       0,  0, 14,  0,  0,  0, 0);
PC3X3_MUX_PIN(PC3X3_BALL_V10, CAPS_P,       0,  0, 15,  0,  0,  0, 0);
PC3X3_MUX_PIN(PC3X3_BALL_L2,  CAPS_E,       0,  0,  0,  0,  0,  0, 0);
PC3X3_MUX_PIN(PC3X3_BALL_L4,  CAPS_E,       0,  0,  0,  1,  0,  0, 0);
PC3X3_MUX_PIN(PC3X3_BALL_K4,  CAPS_E,       0,  0,  0,  2,  0,  0, 0);
PC3X3_MUX_PIN(PC3X3_BALL_K3,  CAPS_E,       0,  0,  0,  3,  0,  0, 0);
PC3X3_MUX_PIN(PC3X3_BALL_F3,  CAPS_DECODE,  0,  8,  0,  0,  0,  0, 0);
PC3X3_MUX_PIN(PC3X3_BALL_F2,  CAPS_DECODE,  0,  9,  0,  0,  1,  0, 0);
PC3X3_MUX_PIN(PC3X3_BALL_E2,  CAPS_DECODE,  0, 10,  0,  0,  2,  0, 0);
PC3X3_MUX_PIN(PC3X3_BALL_D1,  CAPS_DECODE,  0, 11,  0,  0,  3,  0, 0);
PC3X3_MUX_PIN(PC3X3_BALL_C2,  CAPS_SSI,     0,  0,  0,  0,  0,  0, 0);
PC3X3_MUX_PIN(PC3X3_BALL_C1,  CAPS_SSI,     0,  0,  0,  0,  0,  0, 0);
PC3X3_MUX_PIN(PC3X3_BALL_D2,  CAPS_SSI,     0,  0,  0,  0,  0,  0, 0);
PC3X3_MUX_PIN(PC3X3_BALL_W19, CAPS_MII,     0, 13,  0,  0,  0,  0, 0);
PC3X3_MUX_PIN(PC3X3_BALL_V16, CAPS_MII,     0, 13,  0,  0,  0,  0, 0);
PC3X3_MUX_PIN(PC3X3_BALL_U18, CAPS_MII,     0, 13,  0,  0,  0,  0, 0);
PC3X3_MUX_PIN(PC3X3_BALL_U17, CAPS_MII,     0, 13,  0,  0,  0,  0, 0);
PC3X3_MUX_PIN(PC3X3_BALL_R17, CAPS_MII,     0, 13,  0,  0,  0,  0, 0);
PC3X3_MUX_PIN(PC3X3_BALL_U20, CAPS_MII,     0, 13,  0,  0,  0,  0, 0);
PC3X3_MUX_PIN(PC3X3_BALL_W18, CAPS_MII,     0, 13,  0,  0,  0,  0, 0);
PC3X3_MUX_PIN(PC3X3_BALL_Y12, CAPS_MAXIM,   0,  0,  0,  0,  0,  1, 0);
PC3X3_MUX_PIN(PC3X3_BALL_W15, CAPS_MAXIM,   0,  0,  0,  0,  0,  1, 0);
PC3X3_MUX_PIN(PC3X3_BALL_W12, CAPS_MAXIM,   0,  0,  0,  0,  0,  1, 0);
PC3X3_MUX_PIN(PC3X3_BALL_K1,  CAPS_E,       0,  0,  0, 13,  0,  0, 0);
PC3X3_MUX_PIN(PC3X3_BALL_R1,  CAPS_BERR,    0,  0,  0,  0,  0,  0, 0);

#undef PC3X3_MUX_PIN
#define PINCTRL_MUX_PIN(a, b) \
	{ .number = a, .name = b, .drv_data = (void *)&MPC_NAME(a) }

const struct pinctrl_pin_desc pc3x3_pins[] = {
	PINCTRL_MUX_PIN(PC3X3_BALL_D4, "arm_gpio[0]"),
	PINCTRL_MUX_PIN(PC3X3_BALL_E4, "arm_gpio[1]"),
	PINCTRL_MUX_PIN(PC3X3_BALL_D3, "arm_gpio[2]"),
	PINCTRL_MUX_PIN(PC3X3_BALL_E3, "arm_gpio[3]"),
	PINCTRL_MUX_PIN(PC3X3_BALL_A3, "ebi_addr[22]"),
	PINCTRL_MUX_PIN(PC3X3_BALL_U7, "pai_tx_data[0]"),
	PINCTRL_MUX_PIN(PC3X3_BALL_B3, "ebi_addr[23]"),
	PINCTRL_MUX_PIN(PC3X3_BALL_Y6, "pai_tx_data[1]"),
	PINCTRL_MUX_PIN(PC3X3_BALL_C3, "ebi_addr[24]"),
	PINCTRL_MUX_PIN(PC3X3_BALL_W6, "pai_tx_data[2]"),
	PINCTRL_MUX_PIN(PC3X3_BALL_B2, "ebi_addr[25]"),
	PINCTRL_MUX_PIN(PC3X3_BALL_V6, "pai_tx_data[3]"),
	PINCTRL_MUX_PIN(PC3X3_BALL_N4, "shd_gpio"),
	PINCTRL_MUX_PIN(PC3X3_BALL_P3, "boot_mode[0]"),
	PINCTRL_MUX_PIN(PC3X3_BALL_P2, "boot_mode[1]"),
	PINCTRL_MUX_PIN(PC3X3_BALL_P16, "sdram_speed_sel"),
	PINCTRL_MUX_PIN(PC3X3_BALL_P17, "mii_rev_en"),
	PINCTRL_MUX_PIN(PC3X3_BALL_T20, "mii_rmii_en"),
	PINCTRL_MUX_PIN(PC3X3_BALL_T18, "mii_speed_sel"),
	PINCTRL_MUX_PIN(PC3X3_BALL_M1, "ebi_addr[26]"),
	PINCTRL_MUX_PIN(PC3X3_BALL_P1, "arm_gpio[16]"),
	PINCTRL_MUX_PIN(PC3X3_BALL_N1, "arm_gpio[17]"),
	PINCTRL_MUX_PIN(PC3X3_BALL_N2, "arm_gpio[18]"),
	PINCTRL_MUX_PIN(PC3X3_BALL_N3, "arm_gpio[19]"),
	PINCTRL_MUX_PIN(PC3X3_BALL_M2, "ebi_addr[18]"),
	PINCTRL_MUX_PIN(PC3X3_BALL_U12, "pai_rx_data[0]"),
	PINCTRL_MUX_PIN(PC3X3_BALL_M3, "ebi_addr[19]"),
	PINCTRL_MUX_PIN(PC3X3_BALL_Y11, "pai_rx_data[1]"),
	PINCTRL_MUX_PIN(PC3X3_BALL_M4, "ebi_addr[20]"),
	PINCTRL_MUX_PIN(PC3X3_BALL_W11, "pai_rx_data[2]"),
	PINCTRL_MUX_PIN(PC3X3_BALL_L1, "ebi_addr[21]"),
	PINCTRL_MUX_PIN(PC3X3_BALL_V11, "pai_rx_data[3]"),
	PINCTRL_MUX_PIN(PC3X3_BALL_Y5, "pai_tx_data[4]"),
	PINCTRL_MUX_PIN(PC3X3_BALL_W5, "pai_tx_data[5]"),
	PINCTRL_MUX_PIN(PC3X3_BALL_Y4, "pai_tx_data[6]"),
	PINCTRL_MUX_PIN(PC3X3_BALL_W4, "pai_tx_data[7]"),
	PINCTRL_MUX_PIN(PC3X3_BALL_U11, "pai_rx_data[4]"),
	PINCTRL_MUX_PIN(PC3X3_BALL_Y10, "pai_rx_data[5]"),
	PINCTRL_MUX_PIN(PC3X3_BALL_W10, "pai_rx_data[6]"),
	PINCTRL_MUX_PIN(PC3X3_BALL_V10, "pai_rx_data[7]"),
	PINCTRL_MUX_PIN(PC3X3_BALL_L2, "ebi_addr[14]"),
	PINCTRL_MUX_PIN(PC3X3_BALL_L4, "ebi_addr[15]"),
	PINCTRL_MUX_PIN(PC3X3_BALL_K4, "ebi_addr[16]"),
	PINCTRL_MUX_PIN(PC3X3_BALL_K3, "ebi_addr[17]"),
	PINCTRL_MUX_PIN(PC3X3_BALL_F3, "ebi_decode[0]_n"),
	PINCTRL_MUX_PIN(PC3X3_BALL_F2, "ebi_decode[1]_n"),
	PINCTRL_MUX_PIN(PC3X3_BALL_E2, "ebi_decode[2]_n"),
	PINCTRL_MUX_PIN(PC3X3_BALL_D1, "ebi_decode[3]_n"),
	PINCTRL_MUX_PIN(PC3X3_BALL_C2, "spi_clk"),
	PINCTRL_MUX_PIN(PC3X3_BALL_C1, "spi_data_in"),
	PINCTRL_MUX_PIN(PC3X3_BALL_D2, "spi_data_out"),
	PINCTRL_MUX_PIN(PC3X3_BALL_W19, "mii_tx_data[2]"),
	PINCTRL_MUX_PIN(PC3X3_BALL_V16, "mii_tx_data[3]"),
	PINCTRL_MUX_PIN(PC3X3_BALL_U18, "mii_rx_data[2]"),
	PINCTRL_MUX_PIN(PC3X3_BALL_U17, "mii_rx_data[3]"),
	PINCTRL_MUX_PIN(PC3X3_BALL_R17, "mii_col"),
	PINCTRL_MUX_PIN(PC3X3_BALL_U20, "mii_crs"),
	PINCTRL_MUX_PIN(PC3X3_BALL_W18, "mii_tx_clk"),
	PINCTRL_MUX_PIN(PC3X3_BALL_Y12, "max_tx_ctrl"),
	PINCTRL_MUX_PIN(PC3X3_BALL_W15, "max_clk_ref"),
	PINCTRL_MUX_PIN(PC3X3_BALL_W12, "max_trig_clk"),
	PINCTRL_MUX_PIN(PC3X3_BALL_K1, "ebi_clk"),
	PINCTRL_MUX_PIN(PC3X3_BALL_R1, "boot_error"),
};

#undef MPC_NAME

struct pc3x3_group {
	const char *name;
	const unsigned int *pins;
	const unsigned num_pins;
	const bool group_mux;
};

#define PC3X3_GROUP_DEF(X, Y)			\
{						\
	.name = __stringify(X) "_grp",		\
	.pins = PASTE(X, _pins),		\
	.num_pins = ARRAY_SIZE(PASTE(X, _pins)),\
	.group_mux = Y,				\
}

#define PC3X3_GROUP_PINS(X)			\
static const unsigned int PASTE(X, _pins)[]

PC3X3_GROUP_PINS(shd_gpio0) = {PC3X3_BALL_D4};
PC3X3_GROUP_PINS(shd_gpio1) = {PC3X3_BALL_E4};
PC3X3_GROUP_PINS(shd_gpio2) = {PC3X3_BALL_D3};
PC3X3_GROUP_PINS(shd_gpio3) = {PC3X3_BALL_E3};
PC3X3_GROUP_PINS(ebi_addr_22) = {PC3X3_BALL_A3};
PC3X3_GROUP_PINS(pai_tx_data_0) = {PC3X3_BALL_U7};
PC3X3_GROUP_PINS(ebi_addr_23) = {PC3X3_BALL_B3};
PC3X3_GROUP_PINS(pai_tx_data_1) = {PC3X3_BALL_Y6};
PC3X3_GROUP_PINS(ebi_addr_24) = {PC3X3_BALL_C3};
PC3X3_GROUP_PINS(pai_tx_data_2) = {PC3X3_BALL_W6};
PC3X3_GROUP_PINS(ebi_addr_25) = {PC3X3_BALL_B2};
PC3X3_GROUP_PINS(pai_tx_data_3) = {PC3X3_BALL_V6};
PC3X3_GROUP_PINS(shd_gpio) = {PC3X3_BALL_N4};
PC3X3_GROUP_PINS(boot_mode_0) = {PC3X3_BALL_P3};
PC3X3_GROUP_PINS(boot_mode_1) = {PC3X3_BALL_P2};
PC3X3_GROUP_PINS(sdram_speed_sel) = {PC3X3_BALL_P16};
PC3X3_GROUP_PINS(mii_rev_en) = {PC3X3_BALL_P17};
PC3X3_GROUP_PINS(mii_rmii_en) = {PC3X3_BALL_T20};
PC3X3_GROUP_PINS(mii_speed_sel) = {PC3X3_BALL_T18};
PC3X3_GROUP_PINS(ebi_addr_26) = {PC3X3_BALL_M1};
PC3X3_GROUP_PINS(shd_gpio16) = {PC3X3_BALL_P1};
PC3X3_GROUP_PINS(shd_gpio17) = {PC3X3_BALL_N1};
PC3X3_GROUP_PINS(shd_gpio18) = {PC3X3_BALL_N2};
PC3X3_GROUP_PINS(shd_gpio19) = {PC3X3_BALL_N3};
PC3X3_GROUP_PINS(ebi_addr_18) = {PC3X3_BALL_M2};
PC3X3_GROUP_PINS(pai_rx_data_0) = {PC3X3_BALL_U12};
PC3X3_GROUP_PINS(ebi_addr_19) = {PC3X3_BALL_M3};
PC3X3_GROUP_PINS(pai_rx_data_1) = {PC3X3_BALL_Y11};
PC3X3_GROUP_PINS(ebi_addr_20) = {PC3X3_BALL_M4};
PC3X3_GROUP_PINS(pai_rx_data_2) = {PC3X3_BALL_W11};
PC3X3_GROUP_PINS(ebi_addr_21) = {PC3X3_BALL_L1};
PC3X3_GROUP_PINS(pai_rx_data_3) = {PC3X3_BALL_V11};
PC3X3_GROUP_PINS(pai_tx_data_4) = {PC3X3_BALL_Y5};
PC3X3_GROUP_PINS(pai_tx_data_5) = {PC3X3_BALL_W5};
PC3X3_GROUP_PINS(pai_tx_data_6) = {PC3X3_BALL_Y4};
PC3X3_GROUP_PINS(pai_tx_data_7) = {PC3X3_BALL_W4};
PC3X3_GROUP_PINS(pai_rx_data_4) = {PC3X3_BALL_U11};
PC3X3_GROUP_PINS(pai_rx_data_5) = {PC3X3_BALL_Y10};
PC3X3_GROUP_PINS(pai_rx_data_6) = {PC3X3_BALL_W10};
PC3X3_GROUP_PINS(pai_rx_data_7) = {PC3X3_BALL_V10};
PC3X3_GROUP_PINS(ebi_addr_14) = {PC3X3_BALL_L2};
PC3X3_GROUP_PINS(ebi_addr_15) = {PC3X3_BALL_L4};
PC3X3_GROUP_PINS(ebi_addr_16) = {PC3X3_BALL_K4};
PC3X3_GROUP_PINS(ebi_addr_17) = {PC3X3_BALL_K3};
PC3X3_GROUP_PINS(ebi_decode_0) = {PC3X3_BALL_F3};
PC3X3_GROUP_PINS(ebi_decode_1) = {PC3X3_BALL_F2};
PC3X3_GROUP_PINS(ebi_decode_2) = {PC3X3_BALL_E2};
PC3X3_GROUP_PINS(ebi_decode_3) = {PC3X3_BALL_D1};
PC3X3_GROUP_PINS(ssi) = {PC3X3_BALL_C2, PC3X3_BALL_C1, PC3X3_BALL_D2};
PC3X3_GROUP_PINS(mii) = {PC3X3_BALL_W19, PC3X3_BALL_V16, PC3X3_BALL_U18, PC3X3_BALL_U17, PC3X3_BALL_R17, PC3X3_BALL_U20, PC3X3_BALL_W18};
PC3X3_GROUP_PINS(maxphy) = {PC3X3_BALL_Y12, PC3X3_BALL_W15, PC3X3_BALL_W12};
PC3X3_GROUP_PINS(ebi_clk) = {PC3X3_BALL_K1};
PC3X3_GROUP_PINS(boot_error) = {PC3X3_BALL_R1};

static const struct pc3x3_group pc3x3_groups[] = {
	PC3X3_GROUP_DEF(shd_gpio0, false),
	PC3X3_GROUP_DEF(shd_gpio1, false),
	PC3X3_GROUP_DEF(shd_gpio2, false),
	PC3X3_GROUP_DEF(shd_gpio3, false),
	PC3X3_GROUP_DEF(ebi_addr_22, false),
	PC3X3_GROUP_DEF(pai_tx_data_0, false),
	PC3X3_GROUP_DEF(ebi_addr_23, false),
	PC3X3_GROUP_DEF(pai_tx_data_1, false),
	PC3X3_GROUP_DEF(ebi_addr_24, false),
	PC3X3_GROUP_DEF(pai_tx_data_2, false),
	PC3X3_GROUP_DEF(ebi_addr_25, false),
	PC3X3_GROUP_DEF(pai_tx_data_3, false),
	PC3X3_GROUP_DEF(shd_gpio, false),
	PC3X3_GROUP_DEF(boot_mode_0, false),
	PC3X3_GROUP_DEF(boot_mode_1, false),
	PC3X3_GROUP_DEF(sdram_speed_sel, false),
	PC3X3_GROUP_DEF(mii_rev_en, false),
	PC3X3_GROUP_DEF(mii_rmii_en, false),
	PC3X3_GROUP_DEF(mii_speed_sel, false),
	PC3X3_GROUP_DEF(ebi_addr_26, false),
	PC3X3_GROUP_DEF(shd_gpio16, false),
	PC3X3_GROUP_DEF(shd_gpio17, false),
	PC3X3_GROUP_DEF(shd_gpio18, false),
	PC3X3_GROUP_DEF(shd_gpio19, false),
	PC3X3_GROUP_DEF(ebi_addr_18, false),
	PC3X3_GROUP_DEF(pai_rx_data_0, false),
	PC3X3_GROUP_DEF(ebi_addr_19, false),
	PC3X3_GROUP_DEF(pai_rx_data_1, false),
	PC3X3_GROUP_DEF(ebi_addr_20, false),
	PC3X3_GROUP_DEF(pai_rx_data_2, false),
	PC3X3_GROUP_DEF(ebi_addr_21, false),
	PC3X3_GROUP_DEF(pai_rx_data_3, false),
	PC3X3_GROUP_DEF(pai_tx_data_4, false),
	PC3X3_GROUP_DEF(pai_tx_data_5, false),
	PC3X3_GROUP_DEF(pai_tx_data_6, false),
	PC3X3_GROUP_DEF(pai_tx_data_7, false),
	PC3X3_GROUP_DEF(pai_rx_data_4, false),
	PC3X3_GROUP_DEF(pai_rx_data_5, false),
	PC3X3_GROUP_DEF(pai_rx_data_6, false),
	PC3X3_GROUP_DEF(pai_rx_data_7, false),
	PC3X3_GROUP_DEF(ebi_addr_14, false),
	PC3X3_GROUP_DEF(ebi_addr_15, false),
	PC3X3_GROUP_DEF(ebi_addr_16, false),
	PC3X3_GROUP_DEF(ebi_addr_17, false),
	PC3X3_GROUP_DEF(ebi_decode_0, false),
	PC3X3_GROUP_DEF(ebi_decode_1, false),
	PC3X3_GROUP_DEF(ebi_decode_2, false),
	PC3X3_GROUP_DEF(ebi_decode_3, false),
	PC3X3_GROUP_DEF(ssi, true),
	PC3X3_GROUP_DEF(mii, true),
	PC3X3_GROUP_DEF(maxphy, true),
	PC3X3_GROUP_DEF(ebi_clk, false),
	PC3X3_GROUP_DEF(boot_error, false),
};

static int pc3x3_get_groups_count(struct pinctrl_dev *pctldev)
{
	return ARRAY_SIZE(pc3x3_groups);
}

static const char * pc3x3_get_group_name(struct pinctrl_dev *pctldev,
					 unsigned selector)
{
	return pc3x3_groups[selector].name;
}

static int pc3x3_get_group_pins(struct pinctrl_dev *pctldev, unsigned selector,
				const unsigned **pins,
				unsigned *num_pins)
{
	*pins = (unsigned *) pc3x3_groups[selector].pins;
	*num_pins = pc3x3_groups[selector].num_pins;
	return 0;
}

static struct pinctrl_ops pc3x3_pctrl_ops = {
	.get_groups_count = pc3x3_get_groups_count,
	.get_group_name = pc3x3_get_group_name,
	.get_group_pins = pc3x3_get_group_pins,
	.dt_node_to_map = pinconf_generic_dt_node_to_map_all,
	.dt_free_map = pinctrl_utils_dt_free_map,
};

#define PC3X3_GROUP_LIST(X)			\
static const char * const PASTE(X, _groups)[]

PC3X3_GROUP_LIST(ebi) = {
	"ebi_addr_14_grp", "ebi_addr_15_grp", "ebi_addr_16_grp",
	"ebi_addr_17_grp", "ebi_addr_18_grp", "ebi_addr_19_grp",
	"ebi_addr_20_grp", "ebi_addr_21_grp", "ebi_addr_22_grp",
	"ebi_addr_23_grp", "ebi_addr_24_grp", "ebi_addr_25_grp",
	"ebi_addr_26_grp", "ebi_clk_grp", "ebi_decode_0_grp",
	"ebi_decode_1_grp", "ebi_decode_2_grp", "ebi_decode_3_grp",
};

PC3X3_GROUP_LIST(pai) = {
	"pai_tx_data_0_grp", "pai_tx_data_1_grp", "pai_tx_data_2_grp",
	"pai_tx_data_3_grp", "pai_tx_data_4_grp", "pai_tx_data_5_grp",
	"pai_tx_data_6_grp", "pai_tx_data_7_grp", "pai_rx_data_0_grp",
	"pai_rx_data_1_grp", "pai_rx_data_2_grp", "pai_rx_data_3_grp",
	"pai_rx_data_4_grp", "pai_rx_data_5_grp", "pai_rx_data_6_grp",
	"pai_rx_data_7_grp",
};

PC3X3_GROUP_LIST(sdgpio) = {
	"shd_gpio0_grp", "shd_gpio1_grp", "shd_gpio2_grp", "shd_gpio3_grp",
	"ebi_addr_22_grp", "pai_tx_data_0_grp", "ebi_addr_23_grp",
	"pai_tx_data_1_grp", "ebi_addr_24_grp", "pai_tx_data_2_grp",
	"ebi_addr_25_grp", "pai_tx_data_3_grp", "shd_gpio_grp",
	"boot_mode_0_grp", "boot_mode_1_grp", "sdram_speed_sel_grp",
	"mii_rev_en_grp", "mii_rmii_en_grp", "mii_speed_sel_grp",
	"ebi_addr_26_grp", "shd_gpio16_grp", "shd_gpio17_grp",
	"shd_gpio18_grp", "shd_gpio19_grp", "ebi_addr_18_grp",
	"pai_rx_data_0_grp", "ebi_addr_19_grp", "pai_rx_data_1_grp",
	"ebi_addr_20_grp", "pai_rx_data_2_grp", "ebi_addr_21_grp",
	"pai_rx_data_3_grp",
};

PC3X3_GROUP_LIST(gpio) = {
	"shd_gpio0_grp", "shd_gpio1_grp", "shd_gpio2_grp", "shd_gpio3_grp",
	"ebi_addr_22_grp", "pai_tx_data_0_grp", "ebi_addr_23_grp",
	"pai_tx_data_1_grp", "ebi_addr_24_grp", "pai_tx_data_2_grp",
	"ebi_addr_25_grp", "pai_tx_data_3_grp", "shd_gpio_grp",
	"boot_mode_0_grp", "boot_mode_1_grp", "sdram_speed_sel_grp",
	"mii_rev_en_grp", "mii_rmii_en_grp", "mii_speed_sel_grp",
	"ebi_addr_26_grp", "shd_gpio16_grp", "shd_gpio17_grp",
	"shd_gpio18_grp", "shd_gpio19_grp", "ebi_addr_18_grp",
	"pai_rx_data_0_grp", "ebi_addr_19_grp", "pai_rx_data_1_grp",
	"ebi_addr_20_grp", "pai_rx_data_2_grp", "ebi_addr_21_grp",
	"pai_rx_data_3_grp", "pai_tx_data_4_grp", "pai_tx_data_5_grp",
	"pai_tx_data_6_grp", "pai_tx_data_7_grp", "pai_rx_data_4_grp",
	"pai_rx_data_5_grp", "pai_rx_data_6_grp", "pai_rx_data_7_grp",
	"ebi_addr_14_grp", "ebi_addr_15_grp", "ebi_addr_16_grp",
	"ebi_addr_17_grp", "ebi_decode_0_grp", "ebi_decode_1_grp",
	"ebi_decode_2_grp", "ebi_decode_3_grp", "ssi_grp", "mii_grp",
	"maxphy_grp", "ebi_clk_grp", "boot_error_grp",
};

PC3X3_GROUP_LIST(mii) = {
	"mii_grp",
};

PC3X3_GROUP_LIST(fsyn) = {
	"shd_gpio16_grp",
};

PC3X3_GROUP_LIST(ssi) = {
	"ssi_grp", "ebi_decode_0_grp", "ebi_decode_1_grp",
	"ebi_decode_2_grp", "ebi_decode_3_grp",
};

PC3X3_GROUP_LIST(maxphy) = {
	"maxphy_grp",
};

struct pc3x3_pmx_func {
	const char *name;
	const char * const *groups;
	const unsigned num_groups;
	const unsigned int fnc;
};

#define PC3X3_FUNCTION(X, Y) {				\
	.name = __stringify(X),				\
	.groups = PASTE(X, _groups),			\
	.num_groups = ARRAY_SIZE(PASTE(X, _groups)),	\
	.fnc = Y,					\
}

#define PC3X3_GPIO_SELECTOR	0
#define PC3X3_SDGPIO_SELECTOR	1
#define PC3X3_EBI_SELECTOR	2
#define PC3X3_PAI_SELECTOR	3
#define PC3X3_MII_SELECTOR	4
#define PC3X3_FSYN_SELECTOR	5
#define PC3X3_SSI_SELECTOR	6
#define PC3X3_MAXPHY_SELECTOR	7
#define PC3X3_FUNC_SELECTOR_MAX	PC3X3_MAXPHY_SELECTOR

#define IS_GPIO_SELECTOR(SEL)				\
	(SEL == PC3X3_GPIO_SELECTOR || SEL == PC3X3_SDGPIO_SELECTOR)

static const struct pc3x3_pmx_func pc3x3_functions[] = {
	PC3X3_FUNCTION(gpio,	PC3X3_GPIO_SELECTOR),
	PC3X3_FUNCTION(sdgpio,	PC3X3_SDGPIO_SELECTOR),
	PC3X3_FUNCTION(ebi,	PC3X3_EBI_SELECTOR),
	PC3X3_FUNCTION(pai,	PC3X3_PAI_SELECTOR),
	PC3X3_FUNCTION(mii,	PC3X3_MII_SELECTOR),
	PC3X3_FUNCTION(fsyn,	PC3X3_FSYN_SELECTOR),
	PC3X3_FUNCTION(ssi,	PC3X3_SSI_SELECTOR),
	PC3X3_FUNCTION(maxphy,	PC3X3_MAXPHY_SELECTOR),
};

static int pc3x3_get_functions_count(struct pinctrl_dev *pctldev)
{
	return ARRAY_SIZE(pc3x3_functions);
}

static const char *pc3x3_get_fname(struct pinctrl_dev *pctldev,
				   unsigned selector)
{
	return pc3x3_functions[selector].name;
}

static int pc3x3_get_groups(struct pinctrl_dev *pctldev, unsigned selector,
			    const char * const **groups,
			    unsigned * const num_groups)
{
	*groups = pc3x3_functions[selector].groups;
	*num_groups = pc3x3_functions[selector].num_groups;
	return 0;
}

static const struct pinctrl_pin_desc *pc3x3_check_group_mux(
					struct pinctrl_dev *pctldev,
					unsigned group)
{
	int ret;
	const unsigned *pins;
	unsigned num_pins;
	const struct pinctrl_pin_desc *pin0;
	const struct pinctrl_pin_desc *pin;
	const struct pc3x3_pin_config *pd0;
	const struct pc3x3_pin_config *pd;
	unsigned i;
	const struct pc3x3_group *grp = &pc3x3_groups[group];

	if (!grp->group_mux) {
		dev_err(pctldev->dev, "%s: not a group-muxable group\n",
			grp->name);
		return ERR_PTR(-EINVAL);
	}

	ret = pc3x3_get_group_pins(pctldev, group, &pins, &num_pins);
	if (ret != 0) {
		dev_err(pctldev->dev, "%s: unable to get group pins\n",
			grp->name);
		return ERR_PTR(ret);
	}

	pin0 = &pc3x3_pins[*pins];
	pd0 = (const struct pc3x3_pin_config*)pin0->drv_data;
	if (!pd0 || !(pd0->capabilities & MUX_OPT_GROUP)) {
		dev_err(pctldev->dev, "first pin in group-muxable group "
				      "%s is not a group pin\n", grp->name);
		return ERR_PTR(-EINVAL);
	}

	for (i = 1; i < num_pins; ++i) {
		pin = &pc3x3_pins[pins[i]];
		pd = (const struct pc3x3_pin_config*)pin->drv_data;
		if (!pd) {
			dev_err(pctldev->dev, "no driver data for pin "
					      "%s (%u)\n", pin->name, pins[i]);
			return ERR_PTR(-EINVAL);
		}

		/*
		 * Group mux functionality is only implemented
		 * in the SCR and MISC registers, but we prefer
		 * to check that everything is set up the same,
		 * as this board has no mux pins that can be further
		 * muxed to SDGPIO or GPIO.  The only GPIO pins
		 * affected by a group MUX are ARM GPIO 40-52.
		 */
		if (pd->capabilities != pd0->capabilities ||
		    pd->shd_bit != pd0->shd_bit ||
		    pd->scr_bit != pd0->scr_bit ||
		    pd->pai_bit != pd0->pai_bit ||
		    pd->ebi_bit != pd0->ebi_bit ||
		    pd->decode_bit != pd0->decode_bit ||
		    pd->misc_bit != pd0->misc_bit) {
			dev_err(pctldev->dev, "mismatched pin mux config for "
					      "pin %s (%u) in group %s\n",
				pin->name, pins[i], grp->name);
			return ERR_PTR(-EINVAL);
		}
	}

	return pin0;
}

static int pc3x3_check_function(struct pinctrl_dev *pctldev, unsigned selector)
{
	unsigned function = pc3x3_functions[selector].fnc;

	if (function > PC3X3_FUNC_SELECTOR_MAX) {
		dev_err(pctldev->dev, "bad mux function %u (selector %u)\n",
			function, selector);
		return -EINVAL;
	}

	return 0;
}

static int pc3x3_check_gpio_conflict(struct pinctrl_dev *pctldev,
				     const struct pinctrl_pin_desc *pin,
				     unsigned selector)
{
	u32 i;
	const struct pc3x3_pmx *info = pinctrl_dev_get_drvdata(pctldev);
	const struct pc3x3_pin_config *pd =
		(const struct pc3x3_pin_config*)pin->drv_data;

	if (!pd)
		return -EINVAL;

	if (!IS_GPIO_SELECTOR(selector) || !PIN_COULD_CONFLICT(pin))
		return 0;

	for (i = 0; i < NUM_CONFLICT_PINS; ++i) {
		if (info->gpio_conflict_pins[i] == pd->gpio_conflict) {
			dev_err(pctldev->dev, "pin %s cannot be muxed to GPIO "
					      "as pin %s has already been "
					      "muxed to that function\n",
				pin->name, pc3x3_pins[pd->gpio_conflict].name);
			return -EINVAL;
		}
	}

	return 0;
}

static int pc3x3_track_gpio_conflict(struct pinctrl_dev *pctldev,
				     const struct pinctrl_pin_desc *pin)
{
	u32 i;
	struct pc3x3_pmx *info = pinctrl_dev_get_drvdata(pctldev);
	const struct pc3x3_pin_config *pd =
		(const struct pc3x3_pin_config*)pin->drv_data;

	if (!pd)
		return -EINVAL;

	if (!PIN_COULD_CONFLICT(pin))
		return 0;

	for (i = 0; i < NUM_CONFLICT_PINS; ++i) {
		if (!info->gpio_conflict_pins[i]) {
			info->gpio_conflict_pins[i] = pin->number;
			return 0;
		}
	}

	dev_err(pctldev->dev,
		"no free slots to track GPIO electrical conflicts\n");
	return -EINVAL;
}

static void pc3x3_clear_gpio_conflict(struct pinctrl_dev *pctldev,
				      const struct pinctrl_pin_desc *pin)
{
	u32 i;
	struct pc3x3_pmx *info = pinctrl_dev_get_drvdata(pctldev);
	const struct pc3x3_pin_config *pd =
		(const struct pc3x3_pin_config*)pin->drv_data;

	if (!pd)
		return;

	if (!PIN_COULD_CONFLICT(pin))
		return;

	for (i = 0; i < NUM_CONFLICT_PINS; ++i) {
		if (info->gpio_conflict_pins[i] == pin->number) {
			info->gpio_conflict_pins[i] = 0;
			return;
		}
	}
}

static int pc3x3_check_mux_legality(struct pinctrl_dev *pctldev,
				    const struct pinctrl_pin_desc *pin,
				    unsigned selector)
{
	int ret = 0;

	switch (selector) {
	case PC3X3_SDGPIO_SELECTOR:
		if (!PIN_HAS_CAP(pin, SHD_GPIO))
			ret = -EINVAL;

		break;
	case PC3X3_EBI_SELECTOR:
		if (PIN_HAS_CAP(pin, EBI))
			break;

		if (PIN_HAS_CAP(pin, SYSCONF) && PIN_HAS_CAP(pin, DECODE)) {
			if (PIN_BIT(pin, scr) < 8 || PIN_BIT(pin, scr) > 11)
				ret = -EINVAL;

			break;
		}

		ret = -EINVAL;
		break;
	case PC3X3_PAI_SELECTOR:
		if (!PIN_HAS_CAP(pin, PAI))
			ret = -EINVAL;

		break;
	case PC3X3_MII_SELECTOR:
		if (!PIN_HAS_CAP(pin, MII))
			ret = -EINVAL;

		break;
	case PC3X3_FSYN_SELECTOR:
		if (!PIN_HAS_CAP(pin, FSYN))
			ret = -EINVAL;

		break;
	case PC3X3_SSI_SELECTOR:
		if (PIN_HAS_CAP(pin, MISC)) {
			if (PIN_BIT(pin, misc) != 0)
				ret = -EINVAL;

			break;
		}

		if (PIN_HAS_CAP(pin, SYSCONF) && PIN_HAS_CAP(pin, DECODE)) {
			if (PIN_BIT(pin, scr) < 8 || PIN_BIT(pin, scr) > 11)
				ret = -EINVAL;

			break;
		}

		ret = -EINVAL;
		break;
	case PC3X3_MAXPHY_SELECTOR:
		if (!PIN_HAS_CAP(pin, MISC))
			ret = -EINVAL;

		if (PIN_BIT(pin, misc) != 1)
			ret = -EINVAL;

		break;
	}

	if (ret == 0) {
		ret = pc3x3_check_gpio_conflict(pctldev, pin, selector);
		if (ret != 0)
			return ret;
	}

	if (ret != 0) {
		dev_err(pctldev->dev, "pin %s cannot perform function %s\n",
			pin->name, pc3x3_get_fname(pctldev, selector));
		return ret;
	}

	return ret;
}

static inline int tst_scr_bit(struct pinctrl_dev *pctldev, u8 bit)
{
	int ret;
	u32 val;
	struct pc3x3_pmx *info = pinctrl_dev_get_drvdata(pctldev);

	ret = regmap_read(info->scr, 0x0, &val);
	if (ret < 0)
		return ret;

	return ((val >> bit) & 0x1);
}

static inline int set_clear_scr_bit(struct pinctrl_dev *pctldev,
				    u8 bit, bool set)
{
	u32 val;
	u32 mask;
	struct pc3x3_pmx *info = pinctrl_dev_get_drvdata(pctldev);

	mask = (1U << bit);
	val = set ? mask : 0;
	return regmap_update_bits(info->scr, 0x0, mask, val);
}

#define TST_SCR_BIT(P, B)	\
	tst_scr_bit((P), (B))
#define SET_SCR_BIT(P, B)	\
	set_clear_scr_bit((P), (B), true)
#define CLR_SCR_BIT(P, B)	\
	set_clear_scr_bit((P), (B), false)

static inline int tst_shd_bit(struct pinctrl_dev *pctldev, u8 bit)
{
	int ret;
	u32 val;
	struct pc3x3_pmx *info = pinctrl_dev_get_drvdata(pctldev);

	ret = regmap_read(info->shd, 0x0, &val);
	if (ret < 0)
		return ret;

	return ((val >> bit) & 0x1);
}

static inline int set_clear_shd_bit(struct pinctrl_dev *pctldev,
				    u8 bit, bool set)
{
	u32 val;
	u32 mask;
	struct pc3x3_pmx *info = pinctrl_dev_get_drvdata(pctldev);

	mask = (1U << bit);
	val = set ? mask : 0;
	return regmap_update_bits(info->shd, 0x0, mask, val);
}

#define TST_SHD_BIT(P, B)	\
	tst_shd_bit((P), (B))
#define SET_SHD_BIT(P, B)	\
	set_clear_shd_bit((P), (B), true)
#define CLR_SHD_BIT(P, B)	\
	set_clear_shd_bit((P), (B), false)

static inline int set_clear_bit_at_offset(struct pinctrl_dev *pctldev,
					   u32 offset, u32 vmask,
					   u8 bit, bool set)
{
	u32 val, tmp;
	struct pc3x3_pmx *info = pinctrl_dev_get_drvdata(pctldev);

	val = readl(info->virtbase + offset) & vmask;

	if (set)
		tmp = val | (1U << bit);
	else
		tmp = val & ~(1U << bit);

	tmp &= vmask;

	if (tmp != val)
		writel(tmp, info->virtbase + offset);

	return 0;
}

#define SET_PAI_BIT(P, B)	\
	set_clear_bit_at_offset((P), PC3X3_PAI_OFFSET, 0xffff, (B), true)
#define CLR_PAI_BIT(P, B)	\
	set_clear_bit_at_offset((P), PC3X3_PAI_OFFSET, 0xffff, (B), false)
#define SET_EBI_BIT(P, B)	\
	set_clear_bit_at_offset((P), PC3X3_EBI_OFFSET, 0x3fff, (B), true)
#define CLR_EBI_BIT(P, B)	\
	set_clear_bit_at_offset((P), PC3X3_EBI_OFFSET, 0x3fff, (B), false)
#define SET_DECODE_BIT(P, B)	\
	set_clear_bit_at_offset((P), PC3X3_DECODE_OFFSET, 0xf, (B), true)
#define CLR_DECODE_BIT(P, B)	\
	set_clear_bit_at_offset((P), PC3X3_DECODE_OFFSET, 0xf, (B), false)
#define SET_MISC_BIT(P, B)	\
	set_clear_bit_at_offset((P), PC3X3_MISC_OFFSET, 0x3, (B), true)
#define CLR_MISC_BIT(P, B)	\
	set_clear_bit_at_offset((P), PC3X3_MISC_OFFSET, 0x3, (B), false)

static int pc3x3_mux_pin_to_gpio_sdgpio(struct pinctrl_dev *pctldev,
					const struct pinctrl_pin_desc *pin,
					bool sd)
{
	int ret;

	if (sd && !PIN_HAS_CAP(pin, SHD_GPIO))
		return -EINVAL;

	if (PIN_IS_CAP(pin, BERR))
		return 0;

	if (PIN_HAS_CAP(pin, FSYN)) {
		ret = CLR_SCR_BIT(pctldev, PIN_BIT(pin, scr));
		if (ret != 0)
			return ret;
	}

	if (PIN_HAS_CAP(pin, SHD_GPIO)) {
		if (sd)
			ret = CLR_SHD_BIT(pctldev, PIN_BIT(pin, shd));
		else
			ret = SET_SHD_BIT(pctldev, PIN_BIT(pin, shd));

		if (ret != 0)
			return ret;
	}

	if (PIN_IS_CAP(pin, MII)) {
		ret = TST_SCR_BIT(pctldev, PIN_BIT(pin, scr));
		if (ret < 0)
			return ret;

		return ret ? 0 : -EINVAL;
	}

	if (PIN_HAS_CAP(pin, EBI_PAI)) {
		ret = CLR_PAI_BIT(pctldev, PIN_BIT(pin, pai));
		if (ret != 0)
			return ret;
	}

	if (PIN_HAS_CAP(pin, EBI)) {
		return SET_EBI_BIT(pctldev, PIN_BIT(pin, ebi));
	}

	if (PIN_HAS_CAP(pin, PAI_EBI)) {
		ret = CLR_EBI_BIT(pctldev, PIN_BIT(pin, ebi));
		if (ret != 0)
			return ret;
	}

	if (PIN_HAS_CAP(pin, PAI)) {
		return SET_PAI_BIT(pctldev, PIN_BIT(pin, pai));
	}

	if (PIN_HAS_CAP(pin, DECODE)) {
		return SET_DECODE_BIT(pctldev, PIN_BIT(pin, decode));
	}

	if (PIN_HAS_CAP(pin, MISC)) {
		return SET_MISC_BIT(pctldev, PIN_BIT(pin, misc));
	}

	return 0;
}

static int pc3x3_mux_pin_to_ebi(struct pinctrl_dev *pctldev,
				const struct pinctrl_pin_desc *pin)
{
	int ret;

	if (PIN_HAS_CAP(pin, DECODE)) {
		BUG_ON(!PIN_HAS_CAP(pin, SYSCONF));

		ret = SET_SCR_BIT(pctldev, PIN_BIT(pin, scr));
		if (ret != 0)
			return ret;

		return CLR_DECODE_BIT(pctldev, PIN_BIT(pin, decode));
	}

	if (!PIN_HAS_CAP(pin, EBI))
		return -EINVAL;

	return CLR_EBI_BIT(pctldev, PIN_BIT(pin, ebi));
}

static int pc3x3_mux_pin_to_pai(struct pinctrl_dev *pctldev,
				const struct pinctrl_pin_desc *pin)
{
	if (!PIN_HAS_CAP(pin, PAI))
		return -EINVAL;

	return CLR_PAI_BIT(pctldev, PIN_BIT(pin, pai));
}

static int pc3x3_mux_pin_to_mii(struct pinctrl_dev *pctldev,
				const struct pinctrl_pin_desc *pin)
{
	int ret;

	if (!PIN_IS_CAP(pin, MII))
		return -EINVAL;

	ret = TST_SCR_BIT(pctldev, PIN_BIT(pin, scr));
	if (ret < 0)
		return ret;

	return ret ? -EINVAL : 0;
}

static int pc3x3_mux_pin_to_fsyn(struct pinctrl_dev *pctldev,
				 const struct pinctrl_pin_desc *pin)
{
	int ret;

	if (!PIN_IS_CAP(pin, SHD_F))
		return -EINVAL;

	ret = SET_SCR_BIT(pctldev, PIN_BIT(pin, scr));
	if (ret != 0)
		return ret;

	return CLR_SHD_BIT(pctldev, PIN_BIT(pin, shd));
}

static int pc3x3_mux_pin_to_ssi(struct pinctrl_dev *pctldev,
				const struct pinctrl_pin_desc *pin)
{
	int ret;

	if (PIN_HAS_CAP(pin, DECODE)) {
		BUG_ON(!PIN_HAS_CAP(pin, SYSCONF));
		ret = CLR_DECODE_BIT(pctldev, PIN_BIT(pin, decode));
		if (ret != 0)
			return ret;

		return CLR_SCR_BIT(pctldev, PIN_BIT(pin, scr));
	}

	if (!PIN_HAS_CAP(pin, MISC))
		return -EINVAL;

	if (PIN_BIT(pin, misc) != 0)
		return -EINVAL;

	return CLR_MISC_BIT(pctldev, PIN_BIT(pin, misc));
}

static int pc3x3_mux_pin_to_maxphy(struct pinctrl_dev *pctldev,
				   const struct pinctrl_pin_desc *pin)
{
	if (!PIN_HAS_CAP(pin, MISC))
		return -EINVAL;

	if (PIN_BIT(pin, misc) != 1)
		return -EINVAL;

	return CLR_MISC_BIT(pctldev, PIN_BIT(pin, misc));
}

static int pc3x3_set_pin_mux(struct pinctrl_dev *pctldev,
			     const struct pinctrl_pin_desc *pin,
			     unsigned selector)
{
	int ret;

	if (!PIN_HAS_DRV_DATA(pin)) {
		dev_err(pctldev->dev, "pin %s has no driver data\n",
			pin->name);
		return -EINVAL;
	}

	ret = pc3x3_check_mux_legality(pctldev, pin, selector);
	if (0 != ret)
		return ret;

#if 0
	dev_info(pctldev->dev, "setting pin %s to function %s\n",
		pin->name, pc3x3_get_fname(pctldev, selector));
#endif

	switch (selector) {
	case PC3X3_GPIO_SELECTOR:
		ret = pc3x3_mux_pin_to_gpio_sdgpio(pctldev, pin, false);
		break;
	case PC3X3_SDGPIO_SELECTOR:
		ret = pc3x3_mux_pin_to_gpio_sdgpio(pctldev, pin, true);
		break;
	case PC3X3_EBI_SELECTOR:
		ret = pc3x3_mux_pin_to_ebi(pctldev, pin);
		break;
	case PC3X3_PAI_SELECTOR:
		ret = pc3x3_mux_pin_to_pai(pctldev, pin);
		break;
	case PC3X3_MII_SELECTOR:
		ret = pc3x3_mux_pin_to_mii(pctldev, pin);
		break;
	case PC3X3_FSYN_SELECTOR:
		ret = pc3x3_mux_pin_to_fsyn(pctldev, pin);
		break;
	case PC3X3_SSI_SELECTOR:
		ret = pc3x3_mux_pin_to_ssi(pctldev, pin);
		break;
	case PC3X3_MAXPHY_SELECTOR:
		ret = pc3x3_mux_pin_to_maxphy(pctldev, pin);
		break;
	}

	if (!ret) {
		if (IS_GPIO_SELECTOR(selector))
			ret = pc3x3_track_gpio_conflict(pctldev, pin);
		else
			pc3x3_clear_gpio_conflict(pctldev, pin);
	}

	return ret;
}

static int pc3x3_gpio_request_enable(struct pinctrl_dev *pctldev,
				     struct pinctrl_gpio_range *range,
				     unsigned offset)
{
	const struct pinctrl_pin_desc *pin = &pc3x3_pins[offset];
	BUG_ON(!PIN_HAS_DRV_DATA(pin));
	/* dev_info(pctldev->dev, "request for GPIO on pin %s (%u)\n", pin->name, offset); */
	/* if this is a group mux we need to ensure that the group has not been muxed elsewhere */
	/* struct pin_desc * pin_desc_get(pctldev, offset) */
	return pc3x3_set_pin_mux(pctldev, pin, PC3X3_GPIO_SELECTOR);
}

static void pc3x3_gpio_disable_free(struct pinctrl_dev *pctldev,
				    struct pinctrl_gpio_range *range,
				    unsigned offset)
{
	const struct pinctrl_pin_desc *pin = &pc3x3_pins[offset];
	BUG_ON(!PIN_HAS_DRV_DATA(pin));
	/* dev_info(pctldev->dev, "free of GPIO on pin %s (%u)\n", pin->name, offset); */
	pc3x3_clear_gpio_conflict(pctldev, pin);
}

static int pc3x3_set_mux(struct pinctrl_dev *pctldev, unsigned selector,
			 unsigned group)
{
	const struct pinctrl_pin_desc *pin;
	const unsigned *pins;
	unsigned num_pins;
	unsigned i;
	int ret;
	const struct pc3x3_group *grp = &pc3x3_groups[group];

	ret = pc3x3_check_function(pctldev, selector);
	if (ret != 0)
		return ret;

	if (grp->group_mux) {
		pin = pc3x3_check_group_mux(pctldev, group);
		if (IS_ERR(pin))
			return PTR_ERR(pin);

		return pc3x3_set_pin_mux(pctldev, pin, selector);
	}

	ret = pc3x3_get_group_pins(pctldev, group, &pins, &num_pins);
	if (ret != 0)
		return ret;

	for (i = 0; i < num_pins; ++i) {
		pin = &pc3x3_pins[pins[i]];
		ret = pc3x3_set_pin_mux(pctldev, pin, selector);
		if (ret != 0)
			return ret;
	}

	return ret;
}

struct pinmux_ops pc3x3_pmxops = {
	.get_functions_count = pc3x3_get_functions_count,
	.get_function_name = pc3x3_get_fname,
	.get_function_groups = pc3x3_get_groups,
	.gpio_request_enable = pc3x3_gpio_request_enable,
	.gpio_disable_free = pc3x3_gpio_disable_free,
	.set_mux = pc3x3_set_mux,
	.strict = true,
};

static struct pinctrl_desc pc3x3_desc = {
	.name = "pc3x3",
	.pins = pc3x3_pins,
	.npins = ARRAY_SIZE(pc3x3_pins),
	.pctlops = &pc3x3_pctrl_ops,
	.pmxops = &pc3x3_pmxops,
	.owner = THIS_MODULE,
};

static int pc3x3_probe(struct platform_device *pdev)
{
	struct pc3x3_pmx *pmx;
	struct resource *res;

	pmx = devm_kzalloc(&pdev->dev, sizeof(*pmx), GFP_KERNEL);
	if (!pmx)
		return -ENOMEM;

	pmx->scr = syscon_regmap_lookup_by_phandle(pdev->dev.of_node, "picochip,syscon");
	if (IS_ERR(pmx->scr))
		return PTR_ERR(pmx->scr);

	pmx->shd = syscon_regmap_lookup_by_phandle(pdev->dev.of_node, "picochip,shdmux");
	if (IS_ERR(pmx->shd))
		return PTR_ERR(pmx->shd);

	pmx->dev = &pdev->dev;

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	pmx->virtbase = devm_ioremap_resource(&pdev->dev, res);
	if (IS_ERR(pmx->virtbase))
		return PTR_ERR(pmx->virtbase);

	pmx->pctl = pinctrl_register(&pc3x3_desc, &pdev->dev, pmx);
	if (!pmx->pctl) {
		dev_err(&pdev->dev, "could not register pc3x3 pinmux driver\n");
		return -EINVAL;
	}

	platform_set_drvdata(pdev, pmx);
	dev_info(&pdev->dev, "initialized pc3x3 pin control driver\n");
	return 0;
}

static int pc3x3_remove(struct platform_device *pdev)
{
	struct pc3x3_pmx *pmx = platform_get_drvdata(pdev);

	pinctrl_unregister(pmx->pctl);

	return 0;
}

static const struct of_device_id pc3x3_pinctrl_match[] = {
	{ .compatible = "picochip,pc3x3-pinctrl" },
	{},
};

static struct platform_driver pc3x3_pmx_driver = {
	.driver = {
		.name = "pinctrl-pc3x3",
		.owner = THIS_MODULE,
		.of_match_table = of_match_ptr(pc3x3_pinctrl_match),
	},
	.probe = pc3x3_probe,
	.remove = pc3x3_remove,
};

static int __init pc3x3_pmx_init(void)
{
	return platform_driver_register(&pc3x3_pmx_driver);
}
arch_initcall(pc3x3_pmx_init);

static void __exit pc3x3_pmx_exit(void)
{
	platform_driver_unregister(&pc3x3_pmx_driver);
}
module_exit(pc3x3_pmx_exit);

MODULE_AUTHOR("Michael van der Westhuizen <michael@smart-africa.com>");
MODULE_DESCRIPTION("Intel picoXcell pc3x3 SoC Pin Control driver");
MODULE_ALIAS("platform:pinctrl-pc3x3");
MODULE_LICENSE("GPL v2");
