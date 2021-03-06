/*
 * Copyright (C) 2013 Gabor Juhos <juhosg@openwrt.org>
 * Copyright (C) 2013 Imagination Technologies
 *
 * SPDX-License-Identifier:	GPL-2.0
 */

#include <common.h>
#include <netdev.h>
#include <pci.h>
#include <pci_gt64120.h>
#include <pci_msc01.h>
#include <rtc.h>
#include <serial.h>

#include <asm/addrspace.h>
#include <asm/io.h>
#include <asm/malta.h>

#include "superio.h"

enum core_card {
	CORE_UNKNOWN,
	CORE_LV,
	CORE_FPGA6,
};

enum sys_con {
	SYSCON_UNKNOWN,
	SYSCON_GT64120,
	SYSCON_MSC01,
};

static void malta_lcd_puts(const char *str)
{
	int i;
	void *reg = (void *)CKSEG1ADDR(MALTA_ASCIIPOS0);

	/* print up to 8 characters of the string */
	for (i = 0; i < min(strlen(str), 8); i++) {
		__raw_writel(str[i], reg);
		reg += MALTA_ASCIIPOS1 - MALTA_ASCIIPOS0;
	}

	/* fill the rest of the display with spaces */
	for (; i < 8; i++) {
		__raw_writel(' ', reg);
		reg += MALTA_ASCIIPOS1 - MALTA_ASCIIPOS0;
	}
}

static enum core_card malta_core_card(void)
{
	u32 corid, rev;

	rev = __raw_readl(CKSEG1ADDR(MALTA_REVISION));
	corid = (rev & MALTA_REVISION_CORID_MSK) >> MALTA_REVISION_CORID_SHF;

	switch (corid) {
	case MALTA_REVISION_CORID_CORE_LV:
		return CORE_LV;

	case MALTA_REVISION_CORID_CORE_FPGA6:
		return CORE_FPGA6;

	default:
		return CORE_UNKNOWN;
	}
}

static enum sys_con malta_sys_con(void)
{
	switch (malta_core_card()) {
	case CORE_LV:
		return SYSCON_GT64120;

	case CORE_FPGA6:
		return SYSCON_MSC01;

	default:
		return SYSCON_UNKNOWN;
	}
}

phys_size_t initdram(int board_type)
{
	return CONFIG_SYS_MEM_SIZE;
}

int checkboard(void)
{
	enum core_card core;

	malta_lcd_puts("U-boot");
	puts("Board: MIPS Malta");

	core = malta_core_card();
	switch (core) {
	case CORE_LV:
		puts(" CoreLV");
		break;

	case CORE_FPGA6:
		puts(" CoreFPGA6");
		break;

	default:
		puts(" CoreUnknown");
	}

	putc('\n');
	return 0;
}

int board_eth_init(bd_t *bis)
{
	return pci_eth_init(bis);
}

void _machine_restart(void)
{
	void __iomem *reset_base;

	reset_base = (void __iomem *)CKSEG1ADDR(MALTA_RESET_BASE);
	__raw_writel(GORESET, reset_base);
}

int board_early_init_f(void)
{
	void *io_base;

	/* choose correct PCI I/O base */
	switch (malta_sys_con()) {
	case SYSCON_GT64120:
		io_base = (void *)CKSEG1ADDR(MALTA_GT_PCIIO_BASE);
		break;

	case SYSCON_MSC01:
		io_base = (void *)CKSEG1ADDR(MALTA_MSC01_PCIIO_BASE);
		break;

	default:
		return -1;
	}

	/* setup FDC37M817 super I/O controller */
	malta_superio_init(io_base);

	return 0;
}

int misc_init_r(void)
{
	rtc_reset();

	return 0;
}

struct serial_device *default_serial_console(void)
{
	switch (malta_sys_con()) {
	case SYSCON_GT64120:
		return &eserial1_device;

	default:
	case SYSCON_MSC01:
		return &eserial2_device;
	}
}

void pci_init_board(void)
{
	pci_dev_t bdf;

	switch (malta_sys_con()) {
	case SYSCON_GT64120:
		set_io_port_base(CKSEG1ADDR(MALTA_GT_PCIIO_BASE));

		gt64120_pci_init((void *)CKSEG1ADDR(MALTA_GT_BASE),
				 0x00000000, 0x00000000, CONFIG_SYS_MEM_SIZE,
				 0x10000000, 0x10000000, 128 * 1024 * 1024,
				 0x00000000, 0x00000000, 0x20000);
		break;

	default:
	case SYSCON_MSC01:
		set_io_port_base(CKSEG1ADDR(MALTA_MSC01_PCIIO_BASE));

		msc01_pci_init((void *)CKSEG1ADDR(MALTA_MSC01_PCI_BASE),
			       0x00000000, 0x00000000, CONFIG_SYS_MEM_SIZE,
			       MALTA_MSC01_PCIMEM_MAP,
			       CKSEG1ADDR(MALTA_MSC01_PCIMEM_BASE),
			       MALTA_MSC01_PCIMEM_SIZE, MALTA_MSC01_PCIIO_MAP,
			       0x00000000, MALTA_MSC01_PCIIO_SIZE);
		break;
	}

	bdf = pci_find_device(PCI_VENDOR_ID_INTEL,
			      PCI_DEVICE_ID_INTEL_82371AB_0, 0);
	if (bdf == -1)
		panic("Failed to find PIIX4 PCI bridge\n");

	/* setup PCI interrupt routing */
	pci_write_config_byte(bdf, PCI_CFG_PIIX4_PIRQRCA, 10);
	pci_write_config_byte(bdf, PCI_CFG_PIIX4_PIRQRCB, 10);
	pci_write_config_byte(bdf, PCI_CFG_PIIX4_PIRQRCC, 11);
	pci_write_config_byte(bdf, PCI_CFG_PIIX4_PIRQRCD, 11);
}
