/*
 * Copyright (c) 2015, Michael van der Westhuizen <michael@smart-africa.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This header provides constants for the Picohip picoXcell pc3x3 SoC
 * pinctrl bindings.
 */
#ifndef _DT_BINDINGS_PINCTRL_PICOXCELL_PC3X3_H
#define _DT_BINDINGS_PINCTRL_PICOXCELL_PC3X3_H

/*
 * The ball layout on the pc3x3 hardware is defined in figure 122 of
 * the datasheet.  This describes a grid layout of 20x20 balls, with
 * the corner balls missing.
 *
 * Pins are numbered, when viewed through the package, from the top
 * left of the package, with the (missing) A1 ball being at the top
 * left.  Numbers run horizontally from 1-20 and letters run
 * vertically from A-Y (A, B, C, D, E, F, G, H, J, K, L, M, N, P, R,
 * T, U, V, W, Y).
 *
 * Only 62 of the balls can be controlled, and only the mux of these
 * balls can be set to pre-determined functions.  Electrical properties
 * of the balls cannot be controlled other than through the pre-determined
 * functions, so pin configuration is not supported.
 *
 * This driver only exposes those balls that can be controlled.
 */

#define PC3X3_BALL_D4	0	/* arm_gpio[0]      GPIO[0]/SDGPIO[16]      */
#define PC3X3_BALL_E4	1	/* arm_gpio[1]      GPIO[1]/SDGPIO[17]      */
#define PC3X3_BALL_D3	2	/* arm_gpio[2]      GPIO[2]/SDGPIO[18]      */
#define PC3X3_BALL_E3	3	/* arm_gpio[3]      GPIO[3]/SDGPIO[19]      */
#define PC3X3_BALL_A3	4	/* ebi_addr[22]     EBI/GPIO[4]/SDGPIO[20]  */
#define PC3X3_BALL_U7	5	/* pai_tx_data[0]   PAI/GPIO[4]/SDGPIO[20]  */
#define PC3X3_BALL_B3	6	/* ebi_addr[23]     EBI/GPIO[5]/SDGPIO[21]  */
#define PC3X3_BALL_Y6	7	/* pai_tx_data[1]   PAI/GPIO[5]/SDGPIO[21]  */
#define PC3X3_BALL_C3	8	/* ebi_addr[24]     EBI/GPIO[6]/SDGPIO[22]  */
#define PC3X3_BALL_W6	9	/* pai_tx_data[2]   PAI/GPIO[6]/SDGPIO[22]  */
#define PC3X3_BALL_B2	10	/* ebi_addr[25]     EBI/GPIO[7]/SDGPIO[23]  */
#define PC3X3_BALL_V6	11	/* pai_tx_data[3]   PAI/GPIO[7]/SDGPIO[23]  */
#define PC3X3_BALL_N4	12	/* shd_gpio         GPIO[8]/SDGPIO[8]       */
#define PC3X3_BALL_P3	13	/* boot_mode[0]     GPIO[9]/SDGPIO[9]       */
#define PC3X3_BALL_P2	14	/* boot_mode[1]     GPIO[10]/SDGPIO[10]     */
#define PC3X3_BALL_P16	15	/* sdram_speed_sel  GPIO[11]/SDGPIO[11]     */
#define PC3X3_BALL_P17	16	/* mii_rev_en       GPIO[12]/SDGPIO[12]     */
#define PC3X3_BALL_T20	17	/* mii_rmii_en      GPIO[13]/SDGPIO[13]     */
#define PC3X3_BALL_T18	18	/* mii_speed_sel    GPIO[14]/SDGPIO[14]     */
#define PC3X3_BALL_M1	19	/* ebi_addr[26]     EBI/GPIO[15]/SDGPIO[15] */
#define PC3X3_BALL_P1	20	/* sd_gpio[0]       FSYN/GPIO[16]/SDGPIO[0] */
#define PC3X3_BALL_N1	21	/* sd_gpio[1]       GPIO[17]/SDGPIO[1]      */
#define PC3X3_BALL_N2	22	/* sd_gpio[2]       GPIO[18]/SDGPIO[2]      */
#define PC3X3_BALL_N3	23	/* sd_gpio[3]       GPIO[19]/SDGPIO[3]      */
#define PC3X3_BALL_M2	24	/* ebi_addr[18]     EBI/GPIO[20]/SDGPIO[4]  */
#define PC3X3_BALL_U12	25	/* pai_rx_data[0]   PAI/GPIO[20]/SDGPIO[4]  */
#define PC3X3_BALL_M3	26	/* ebi_addr[19]     EBI/GPIO[21]/SDGPIO[5]  */
#define PC3X3_BALL_Y11	27	/* pai_rx_data[1]   PAI/GPIO[21]/SDGPIO[5]  */
#define PC3X3_BALL_M4	28	/* ebi_addr[20]     EBI/GPIO[22]/SDGPIO[6]  */
#define PC3X3_BALL_W11	29	/* pai_rx_data[2]   PAI/GPIO[22]/SDGPIO[6]  */
#define PC3X3_BALL_L1	30	/* ebi_addr[21]     EBI/GPIO[23]/SDGPIO[7]  */
#define PC3X3_BALL_V11	31	/* pai_rx_data[3]   PAI/GPIO[23]/SDGPIO[7]  */
#define PC3X3_BALL_Y5	32	/* pai_tx_data[4]   PAI/GPIO[24]            */
#define PC3X3_BALL_W5	33	/* pai_tx_data[5]   PAI/GPIO[25]            */
#define PC3X3_BALL_Y4	34	/* pai_tx_data[6]   PAI/GPIO[26]            */
#define PC3X3_BALL_W4	35	/* pai_tx_data[7]   PAI/GPIO[27]            */
#define PC3X3_BALL_U11	36	/* pai_rx_data[4]   PAI/GPIO[28]            */
#define PC3X3_BALL_Y10	37	/* pai_rx_data[5]   PAI/GPIO[29]            */
#define PC3X3_BALL_W10	38	/* pai_rx_data[6]   PAI/GPIO[30]            */
#define PC3X3_BALL_V10	39	/* pai_rx_data[7]   PAI/GPIO[31]            */
#define PC3X3_BALL_L2	40	/* ebi_addr[14]     EBI/GPIO[32]            */
#define PC3X3_BALL_L4	41	/* ebi_addr[15]     EBI/GPIO[33]            */
#define PC3X3_BALL_K4	42	/* ebi_addr[16]     EBI/GPIO[34]            */
#define PC3X3_BALL_K3	43	/* ebi_addr[17]     EBI/GPIO[35]            */
#define PC3X3_BALL_F3	44	/* ebi_decode[0]_n  SSI/EBI/GPIO[36]        */
#define PC3X3_BALL_F2	45	/* ebi_decode[1]_n  SSI/EBI/GPIO[37]        */
#define PC3X3_BALL_E2	46	/* ebi_decode[2]_n  SSI/EBI/GPIO[38]        */
#define PC3X3_BALL_D1	47	/* ebi_decode[3]_n  SSI/EBI/GPIO[39]        */
#define PC3X3_BALL_C2	48	/* spi_clk          SSI/GPIO[40]            */
#define PC3X3_BALL_C1	49	/* spi_data_in      SSI/GPIO[41]            */
#define PC3X3_BALL_D2	50	/* spi_data_out     SSI/GPIO[42]            */
#define PC3X3_BALL_W19	51	/* mii_tx_data[2]   MII/GPIO[43]            */
#define PC3X3_BALL_V16	52	/* mii_tx_data[3]   MII/GPIO[44]            */
#define PC3X3_BALL_U18	53	/* mii_rx_data[2]   MII/GPIO[45]            */
#define PC3X3_BALL_U17	54	/* mii_rx_data[3]   MII/GPIO[46]            */
#define PC3X3_BALL_R17	55	/* mii_col          MII/GPIO[47]            */
#define PC3X3_BALL_U20	56	/* mii_crs          MII/GPIO[48]            */
#define PC3X3_BALL_W18	57	/* mii_tx_clk       MII/GPIO[49]            */
#define PC3X3_BALL_Y12	58	/* max_tx_ctrl      MaxPHY/GPIO[50]         */
#define PC3X3_BALL_W15	59	/* max_clk_ref      MaxPHY/GPIO[51]         */
#define PC3X3_BALL_W12	60	/* max_trig_clk     MaxPHY/GPIO[52]         */
#define PC3X3_BALL_K1	61	/* ebi_clk          EBI/GPIO[53]            */
#define PC3X3_BALL_R1	62	/* boot_error       GPIO[54]                */

#define PC3X3_ARM_GPIO_0	PC3X3_BALL_D4
#define PC3X3_ARM_GPIO_1	PC3X3_BALL_E4
#define PC3X3_ARM_GPIO_2	PC3X3_BALL_D3
#define PC3X3_ARM_GPIO_3	PC3X3_BALL_E3
#define PC3X3_ARM_GPIO_4_EBI	PC3X3_BALL_A3
#define PC3X3_ARM_GPIO_4_PAI	PC3X3_BALL_U7
#define PC3X3_ARM_GPIO_5_EBI	PC3X3_BALL_B3
#define PC3X3_ARM_GPIO_5_PAI	PC3X3_BALL_Y6
#define PC3X3_ARM_GPIO_6_EBI	PC3X3_BALL_C3
#define PC3X3_ARM_GPIO_6_PAI	PC3X3_BALL_W6
#define PC3X3_ARM_GPIO_7_EBI	PC3X3_BALL_B2
#define PC3X3_ARM_GPIO_7_PAI	PC3X3_BALL_V6
#define PC3X3_ARM_GPIO_8	PC3X3_BALL_N4
#define PC3X3_ARM_GPIO_9	PC3X3_BALL_P3
#define PC3X3_ARM_GPIO_10	PC3X3_BALL_P2
#define PC3X3_ARM_GPIO_11	PC3X3_BALL_P16
#define PC3X3_ARM_GPIO_12	PC3X3_BALL_P17
#define PC3X3_ARM_GPIO_13	PC3X3_BALL_T20
#define PC3X3_ARM_GPIO_14	PC3X3_BALL_T18
#define PC3X3_ARM_GPIO_15	PC3X3_BALL_M1
#define PC3X3_ARM_GPIO_16	PC3X3_BALL_P1
#define PC3X3_ARM_GPIO_17	PC3X3_BALL_N1
#define PC3X3_ARM_GPIO_18	PC3X3_BALL_N2
#define PC3X3_ARM_GPIO_19	PC3X3_BALL_N3
#define PC3X3_ARM_GPIO_20_EBI	PC3X3_BALL_M2
#define PC3X3_ARM_GPIO_20_PAI	PC3X3_BALL_U12
#define PC3X3_ARM_GPIO_21_EBI	PC3X3_BALL_M3
#define PC3X3_ARM_GPIO_21_PAI	PC3X3_BALL_Y11
#define PC3X3_ARM_GPIO_22_EBI	PC3X3_BALL_M4
#define PC3X3_ARM_GPIO_22_PAI	PC3X3_BALL_W11
#define PC3X3_ARM_GPIO_23_EBI	PC3X3_BALL_L1
#define PC3X3_ARM_GPIO_23_PAI	PC3X3_BALL_V11
#define PC3X3_ARM_GPIO_24	PC3X3_BALL_Y5
#define PC3X3_ARM_GPIO_25	PC3X3_BALL_W5
#define PC3X3_ARM_GPIO_26	PC3X3_BALL_Y4
#define PC3X3_ARM_GPIO_27	PC3X3_BALL_W4
#define PC3X3_ARM_GPIO_28	PC3X3_BALL_U11
#define PC3X3_ARM_GPIO_29	PC3X3_BALL_Y10
#define PC3X3_ARM_GPIO_30	PC3X3_BALL_W10
#define PC3X3_ARM_GPIO_31	PC3X3_BALL_V10
#define PC3X3_ARM_GPIO_32	PC3X3_BALL_L2
#define PC3X3_ARM_GPIO_33	PC3X3_BALL_L4
#define PC3X3_ARM_GPIO_34	PC3X3_BALL_K4
#define PC3X3_ARM_GPIO_35	PC3X3_BALL_K3
#define PC3X3_ARM_GPIO_36	PC3X3_BALL_F3
#define PC3X3_ARM_GPIO_37	PC3X3_BALL_F2
#define PC3X3_ARM_GPIO_38	PC3X3_BALL_E2
#define PC3X3_ARM_GPIO_39	PC3X3_BALL_D1
#define PC3X3_ARM_GPIO_40	PC3X3_BALL_C2
#define PC3X3_ARM_GPIO_41	PC3X3_BALL_C1
#define PC3X3_ARM_GPIO_42	PC3X3_BALL_D2
#define PC3X3_ARM_GPIO_43	PC3X3_BALL_W19
#define PC3X3_ARM_GPIO_44	PC3X3_BALL_V16
#define PC3X3_ARM_GPIO_45	PC3X3_BALL_U18
#define PC3X3_ARM_GPIO_46	PC3X3_BALL_U17
#define PC3X3_ARM_GPIO_47	PC3X3_BALL_R17
#define PC3X3_ARM_GPIO_48	PC3X3_BALL_U20
#define PC3X3_ARM_GPIO_49	PC3X3_BALL_W18
#define PC3X3_ARM_GPIO_50	PC3X3_BALL_Y12
#define PC3X3_ARM_GPIO_51	PC3X3_BALL_W15
#define PC3X3_ARM_GPIO_52	PC3X3_BALL_W12
#define PC3X3_ARM_GPIO_53	PC3X3_BALL_K1
#define PC3X3_ARM_GPIO_54	PC3X3_BALL_R1

#define PC3X3_SDGPIO_0		PC3X3_BALL_P1
#define PC3X3_SDGPIO_1		PC3X3_BALL_N1
#define PC3X3_SDGPIO_2		PC3X3_BALL_N2
#define PC3X3_SDGPIO_3		PC3X3_BALL_N3
#define PC3X3_SDGPIO_4_EBI	PC3X3_BALL_M2
#define PC3X3_SDGPIO_4_PAI	PC3X3_BALL_U12
#define PC3X3_SDGPIO_5_EBI	PC3X3_BALL_M3
#define PC3X3_SDGPIO_5_PAI	PC3X3_BALL_Y11
#define PC3X3_SDGPIO_6_EBI	PC3X3_BALL_M4
#define PC3X3_SDGPIO_6_PAI	PC3X3_BALL_W11
#define PC3X3_SDGPIO_7_EBI	PC3X3_BALL_L1
#define PC3X3_SDGPIO_7_PAI	PC3X3_BALL_V11
#define PC3X3_SDGPIO_8		PC3X3_BALL_N4
#define PC3X3_SDGPIO_9		PC3X3_BALL_P3
#define PC3X3_SDGPIO_10		PC3X3_BALL_P2
#define PC3X3_SDGPIO_11		PC3X3_BALL_P16
#define PC3X3_SDGPIO_12		PC3X3_BALL_P17
#define PC3X3_SDGPIO_13		PC3X3_BALL_T20
#define PC3X3_SDGPIO_14		PC3X3_BALL_T18
#define PC3X3_SDGPIO_15		PC3X3_BALL_M1
#define PC3X3_SDGPIO_16		PC3X3_BALL_D4
#define PC3X3_SDGPIO_17		PC3X3_BALL_E4
#define PC3X3_SDGPIO_18		PC3X3_BALL_D3
#define PC3X3_SDGPIO_19		PC3X3_BALL_E3
#define PC3X3_SDGPIO_20_EBI	PC3X3_BALL_A3
#define PC3X3_SDGPIO_20_PAI	PC3X3_BALL_U7
#define PC3X3_SDGPIO_21_EBI	PC3X3_BALL_B3
#define PC3X3_SDGPIO_21_PAI	PC3X3_BALL_Y6
#define PC3X3_SDGPIO_22_EBI	PC3X3_BALL_C3
#define PC3X3_SDGPIO_22_PAI	PC3X3_BALL_W6
#define PC3X3_SDGPIO_23_EBI	PC3X3_BALL_B2
#define PC3X3_SDGPIO_23_PAI	PC3X3_BALL_V6

#endif
