// SPDX-License-Identifier: GPL-2.0+ OR BSD-3-Clause
/*
 * Copyright (C) 2018, STMicroelectronics - All Rights Reserved
 */

#include <config.h>
#include <common.h>
#include <dm/device.h>
#include <init.h>
#include <asm/io.h>
#include <asm/arch/sys_proto.h>
#include <asm/gpio.h>
#include <linux/bitops.h>
#include <linux/delay.h>
#include "../common/stpmic1.h"

/* board early initialisation in board_f: need to use global variable */
static u32 opp_voltage_mv __section(".data");

void board_vddcore_init(u32 voltage_mv)
{
	if (IS_ENABLED(CONFIG_PMIC_STPMIC1) && CONFIG_IS_ENABLED(POWER_SUPPORT))
		opp_voltage_mv = voltage_mv;
}

int board_early_init_f(void)
{
	if (IS_ENABLED(CONFIG_PMIC_STPMIC1) && CONFIG_IS_ENABLED(POWER_SUPPORT))
		stpmic1_init(opp_voltage_mv);

	return 0;
}

#if IS_ENABLED(CONFIG_SPL_OS_BOOT)
int spl_start_uboot(void)
{
	ofnode node;
	struct gpio_desc gpio;
	int boot_uboot = 1;

	node = ofnode_path("/config");
	if (!ofnode_valid(node)) {
		pr_warn("%s: no /config node?\n", __func__);
		return 0;
	}
	if (gpio_request_by_name_nodev(node, "st,fastboot-gpios", 0,
		&gpio, GPIOD_IS_IN)) {
		pr_warn("%s: could not find a /config/st,fastboot-gpios\n",
		      __func__);
		return 1;
		}

		boot_uboot = dm_gpio_get_value(&gpio);
	dm_gpio_free(NULL, &gpio);

	return boot_uboot;
}

#if IS_ENABLED(CONFIG_ARMV7_NONSEC)
/*
 * A bit of a hack, but armv7_boot_nonsec() is provided by bootm.c. This is not
 * available in SPL, so we have to provide an implementation.
 */
bool armv7_boot_nonsec(void)
{
	return 0;
}
#endif /* CONFIG_ARMV7_NONSEC */
#endif /* CONFIG_SPL_OS_BOOT */

#ifdef CONFIG_DEBUG_UART_BOARD_INIT
void board_debug_uart_init(void)
{
#if (CONFIG_DEBUG_UART_BASE == STM32_UART4_BASE)

#define RCC_MP_APB1ENSETR (STM32_RCC_BASE + 0x0A00)
#define RCC_MP_AHB4ENSETR (STM32_RCC_BASE + 0x0A28)

	/* UART4 clock enable */
	setbits_le32(RCC_MP_APB1ENSETR, BIT(16));

#define GPIOG_BASE 0x50008000
	/* GPIOG clock enable */
	writel(BIT(6), RCC_MP_AHB4ENSETR);
	/* GPIO configuration for ST boards: Uart4 TX = G11 */
	writel(0xffbfffff, GPIOG_BASE + 0x00);
	writel(0x00006000, GPIOG_BASE + 0x24);
#else

#error("CONFIG_DEBUG_UART_BASE: not supported value")

#endif
}
#endif

int board_fit_config_name_match(const char *name)
{
	if (of_machine_is_compatible("st,stm32mp157c-dk2"))
		return !strstr(name, "stm32mp157c-dk2");

	/* Okay, it's most likely an EV board */
	return !strstr(name, "stm32mp157") + !strstr(name, "-ev");
}
