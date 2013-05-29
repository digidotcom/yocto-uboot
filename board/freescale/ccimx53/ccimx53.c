/*
 * Copyright (C) 2011 Digi International, Inc.
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

#include <common.h>
#include <nvram.h>
#include <cmd_bsp.h>
#include <asm/io.h>
#include <asm/arch/mx53.h>
#include <asm/arch/mx53_pins.h>
#include <asm/arch/iomux.h>
#include <asm/errno.h>
#include <asm/imx_iim.h>
#include <asm/arch/mxc_nand.h>
#include <asm/arch/gpio.h>
#include <asm/imx_iim.h>
#include <netdev.h>
#include <serial.h>
#include <linux/ctype.h>

#if defined(CONFIG_VIDEO_MX5)
#include <linux/list.h>
#include <ipu.h>
#include <lcd.h>
#include <linux/fb.h>
#include <linux/mxcfb.h>
#endif

#if CONFIG_I2C_MXC
#include <i2c.h>
#endif

#ifdef CONFIG_CMD_MMC
#include <mmc.h>
#include <fsl_esdhc.h>
#endif

#ifdef CONFIG_ARCH_MMU
#include <asm/mmu.h>
#include <asm/arch/mmu.h>
#endif

#ifdef CONFIG_MXC_WATCHDOG
# include <asm/arch/mxc_wdt.h>
#endif

#ifdef CONFIG_CMD_CLOCK
#include <asm/clock.h>
#endif

#ifdef CONFIG_ANDROID_RECOVERY
#include "../common/recovery.h"
#include <part.h>
#include <ext2fs.h>
#include <linux/mtd/mtd.h>
#include <linux/mtd/partitions.h>
#include <ubi_uboot.h>
#include <jffs2/load_kernel.h>
#endif

#define DA9053_GPIO_10_11	26
#define DA9053_GPIO_12_13	27

#include "board-ccimx53.h"

DECLARE_GLOBAL_DATA_PTR;

static u32 system_rev;
static enum boot_device boot_dev;
static unsigned long boot_verb = 0;
struct ccimx53_hwid mod_hwid;
u32 swap_bbi_limit = 0;
extern size_t uboot_part_size;
u8 da905x_i2c_addr = DA905x_I2C_ALT_ADDR;
#ifdef CONFIG_BOARD_CLEANUP_BEFORE_LINUX
extern int mxc_serial_deinit(void);
extern nv_os_type_e eOSType;
#endif

extern unsigned int get_total_ram(void);

#ifdef CONFIG_VIDEO_MX5
int display_initialized;

#if defined(CONFIG_BMP_8BPP)
ushort colormap[256];
#elif defined(CONFIG_BMP_16BPP)
ushort colormap[65536];
#else
ushort colormap[16777216];
#endif

extern int ipuv3_fb_init(struct fb_videomode *mode, int di,
			int interface_pix_fmt,
			ipu_di_clk_parent_t di_clk_parent,
			int di_clk_val);
vidinfo_t panel_info;
#endif

/* Write a single register */
int pmic_wr_reg(uchar reg, uchar data)
{
#ifdef CONFIG_USE_PMIC_I2C_WORKARROUND
	uchar data2 = 0xff;
	int ret;

	ret = i2c_write(da905x_i2c_addr, reg, 1, &data, 1);
	if (!ret)
		ret = i2c_write(da905x_i2c_addr, 0xff, 1, &data2, 1);

	return ret;
#else
	return i2c_write(da905x_i2c_addr, reg, 1, &data, 1);
#endif
}

/* Read a single register */
int pmic_rd_reg(uchar reg)
{
#ifdef CONFIG_USE_PMIC_I2C_WORKARROUND
	int ret;
	uchar data, data2 = 0xff;

	ret = i2c_read(da905x_i2c_addr, reg, 1, &data, 1);
	if (!ret) {
		ret = i2c_write(da905x_i2c_addr, 0xff, 1, &data2, 1);
		if (!ret)
			ret = (int)data;
	}

	return ret;
#else
	int ret;
	uchar data;

	ret = i2c_read(da905x_i2c_addr, reg, 1, &data, 1);
	if (!ret)
		ret = (int)data;

	return ret;
#endif
}

int pmic_wr_msk_reg(uchar reg, uchar data, uchar mask)
{
	int ret;
	uchar newval;

	ret = pmic_rd_reg(reg);
	if (ret < 0)
		return ret;

	newval = (uchar)ret & ~mask;
	newval |= (data & mask);

	return pmic_wr_reg(reg, newval);
}

static inline void setup_boot_device(void)
{
	uint soc_sbmr = readl(SRC_BASE_ADDR + 0x4);
	uint bt_mem_ctl = (soc_sbmr & 0x000000FF) >> 4 ;
	uint bt_mem_type = (soc_sbmr & 0x00000008) >> 3;

	switch (bt_mem_ctl) {
	case 0x0:
		if (bt_mem_type)
			boot_dev = ONE_NAND_BOOT;
		else
			boot_dev = WEIM_NOR_BOOT;
		break;
	case 0x2:
		if (bt_mem_type)
			boot_dev = SATA_BOOT;
		else
			boot_dev = PATA_BOOT;
		break;
	case 0x3:
		if (bt_mem_type)
			boot_dev = SPI_NOR_BOOT;
		else
			boot_dev = I2C_BOOT;
		break;
	case 0x4:
	case 0x5:
		boot_dev = SD_BOOT;
		break;
	case 0x6:
	case 0x7:
		boot_dev = MMC_BOOT;
		break;
	case 0x8 ... 0xf:
		boot_dev = NAND_BOOT;
		break;
	default:
		boot_dev = UNKNOWN_BOOT;
		break;
	}
}

enum boot_device get_boot_device(void)
{
	return boot_dev;
}

u32 get_board_rev(void)
{
	return system_rev;
}

u8 get_module_variant_from_fuse(void)
{
	return (u8)sense_fuse(CONFIG_IIM_HWID_BANK, CONFIG_IIM_HWID_ROW, 0);
}

u8 get_hw_rev_from_fuse(void)
{
	return (u8)sense_fuse(CONFIG_IIM_HWID_BANK, CONFIG_IIM_HWID_ROW + 1, 0);
}

static int is_valid_hw_id(u8 variant)
{
	int ret = 0;

	if (variant < ARRAY_SIZE(ccimx53_id)) {
		if (ccimx53_id[variant].mem[0] != 0)
			ret = 1;
	}
	return ret;
}

int variant_has_ethernet(void)
{
	return 1;
}

int variant_machid(void)
{
	/* Return the MACHINE ID from the variant
	 */
	switch(mod_hwid.variant) {
	case 0x03:
	case 0x05:
	case 0x07:
	case 0x09:
	case 0x0b:
	case 0x0d:
	case 0x11:
		/* Non-wireless variants */
		return MACH_TYPE_CCMX53JS;
	default:
		break;
	}
	/* Wireless variants (or unspecified) */
	return MACH_TYPE_CCWMX53JS;
}

static unsigned int get_ram_from_hwid(u8 variant)
{
	return ccimx53_id[variant].mem[0];
}

static unsigned int get_cpu_freq_from_hwid(u8 variant)
{
	return ccimx53_id[variant].cpu_freq;
}

void get_board_serial(struct tag_serialnr *serialnr)
{
	u8 *p = (u8 *)serialnr;
	/* On linux, we pass the hwid information using serialnr TAG */
	memcpy(p, (u8 *)gd->bd->hwid, 6);

	/* The extra 2 bytes are used to pass additional information */
	/* Indicate if swap bi is done by the bootloader */
	*(p + 6) = SWAP_BI_TAG;
}

int array_to_hwid(u8 *hwid)
{
	char manloc;

	if (!is_valid_hw_id(hwid[0]))
		return -EINVAL;

	manloc = hwid[2] & 0xc0;
	mod_hwid.variant = hwid[0];
	mod_hwid.version = hwid[1];
	mod_hwid.sn =  ((hwid[2] & 0x3f) << 24) | (hwid[3] << 16) |
		        (hwid[4] << 8) | hwid[5];
	mod_hwid.mloc = (manloc == 0) ? 'B' : (manloc == 0x40) ? 'W' :
			(manloc == 0x80) ? 'S' : 'N';

	/* Pass the hwid information using the board data info */
	memcpy((u8 *)gd->bd->hwid, hwid, 6);

	return  0;
}

int get_module_hw_id(void)
{
	nv_critical_t *pNVRAM;
	u8 hwid[6];

	memset(&mod_hwid, 0, sizeof(struct ccimx53_hwid));

	/* nvram settings override fuse hw id */
	if (NvCriticalGet(&pNVRAM)) {
		const nv_param_module_id_t *pModule = &pNVRAM->s.p.xID;

		if (!array_to_hwid((u8 *)pModule->szProductType))
			return 0;
	}

	/* Use fuses if no valid hw id was set in the nvram */
	if (iim_read_hw_id(hwid))
		array_to_hwid(hwid);

	/* returning something != 0 will halt the boot process */
	return 0;
}

void NvPrintHwID(void)
{
	printf("    Variant       %02x\n", mod_hwid.variant);
	printf("    Revision:     %02x\n", mod_hwid.version);
	if (!isalpha(mod_hwid.mloc) || 0 == mod_hwid.sn)
		printf("    S/N:\n");
	else
		printf("    S/N:          %c%09u\n", isalpha(mod_hwid.mloc) ?
				mod_hwid.mloc : '-', mod_hwid.sn);
}

void setup_cpu_freq(void)
{
	unsigned int cpu_freq;

	if (is_valid_hw_id(mod_hwid.variant)) {
		cpu_freq = get_cpu_freq_from_hwid(mod_hwid.variant);
		if (cpu_freq == 1000000000) {
			/* Switch to 1GHZ */
			clk_config(CONFIG_REF_CLK_FREQ, 1000, CPU_CLK);
		}
	}
}

#ifdef CONFIG_MXC_NAND_SWAP_BI
#define SKIP_SWAP_BI_MAX_PAGE_2K               (PART_UBOOT_SIZE / 0x800)
#define SKIP_SWAP_BI_MAX_PAGE_4K               (PART_UBOOT_SIZE_4K / 0x1000)
inline int skip_swap_bi(int page)
{
	/**
	 * MX53 TO1 ROM doesn't support BI swap.
	 * Avoid doing that when programming U-Boot into the flash
	 */
	if ((is_soc_rev(CHIP_REV_1_0) == 0) && (page < swap_bbi_limit))
		return 1;

	return 0;
}
#else
inline int skip_swap_bi(int page_addr)
{
	return 1;
}
#endif

static inline void setup_soc_rev(void)
{
	int reg;

	/* Si rev is obtained from ROM */
	reg = __REG(ROM_SI_REV);

	switch (reg) {
	case 0x10:
		system_rev = 0x53000 | CHIP_REV_1_0;
		break;
	case 0x20:
		system_rev = 0x53000 | CHIP_REV_2_0;
		break;
	case 0x21:
		system_rev = 0x53000 | CHIP_REV_2_1;
		break;
	default:
		system_rev = 0x53000 | CHIP_REV_UNKNOWN;
	}
}

static inline void setup_board_rev(int rev)
{
	system_rev |= (rev & 0xF) << 8;
}

inline int is_soc_rev(int rev)
{
	return (system_rev & 0xFF) - rev;
}

#ifdef CONFIG_COMPUTE_LPJ
unsigned int get_preset_lpj(void)
{
	unsigned int mcu_clk, lpj = 0;

	mcu_clk = mxc_get_clock(MXC_ARM_CLK);

	switch (mcu_clk) {
	case 1000000000:
		lpj = CONFIG_PRESET_LPJ_1000MHZ;
		break;
	case 800000000:
		lpj = CONFIG_PRESET_LPJ_800MHZ;
		break;
	}
	return lpj;
}
#endif

#ifdef CONFIG_BOARD_CLEANUP_BEFORE_LINUX
void board_cleanup_before_linux(void)
{
	if (gd->flags & GD_FLG_SILENT)
		mxc_serial_deinit();
}
#endif

#ifdef CONFIG_ARCH_MMU
void board_mmu_init(void)
{
	unsigned long ttb_base = PHYS_SDRAM_1 + 0x4000;
	unsigned long i;

	/*
	* Set the TTB register
	*/
	asm volatile ("mcr  p15,0,%0,c2,c0,0" : : "r"(ttb_base) /*:*/);

	/*
	* Set the Domain Access Control Register
	*/
	i = ARM_ACCESS_DACR_DEFAULT;
	asm volatile ("mcr  p15,0,%0,c3,c0,0" : : "r"(i) /*:*/);

	/*
	* First clear all TT entries - ie Set them to Faulting
	*/
	memset((void *)ttb_base, 0, ARM_FIRST_LEVEL_PAGE_TABLE_SIZE);
	/* Actual   Virtual  Size   Attributes          Function */
	/* Base     Base     MB     cached? buffered?  access permissions */
	/* xxx00000 xxx00000 */
	X_ARM_MMU_SECTION(0x000, 0x000, 0x10,
			ARM_UNCACHEABLE, ARM_UNBUFFERABLE,
			ARM_ACCESS_PERM_RW_RW); /* ROM, 16M */
	X_ARM_MMU_SECTION(0x010, 0x010, 0x060,
			ARM_UNCACHEABLE, ARM_UNBUFFERABLE,
			ARM_ACCESS_PERM_RW_RW); /* Reserved, 96M */
	X_ARM_MMU_SECTION(0x070, 0x070, 0x010,
			ARM_UNCACHEABLE, ARM_UNBUFFERABLE,
			ARM_ACCESS_PERM_RW_RW); /* IRAM, 16M */
	X_ARM_MMU_SECTION(0x080, 0x080, 0x080,
			ARM_UNCACHEABLE, ARM_UNBUFFERABLE,
			ARM_ACCESS_PERM_RW_RW); /* Reserved region + TZIC. 1M */
	X_ARM_MMU_SECTION(0x100, 0x100, 0x040,
			ARM_UNCACHEABLE, ARM_UNBUFFERABLE,
			ARM_ACCESS_PERM_RW_RW); /* SATA */
	X_ARM_MMU_SECTION(0x140, 0x140, 0x040,
			ARM_UNCACHEABLE, ARM_UNBUFFERABLE,
			ARM_ACCESS_PERM_RW_RW); /* Reserved, 64M */
	X_ARM_MMU_SECTION(0x180, 0x180, 0x080,
			ARM_UNCACHEABLE, ARM_UNBUFFERABLE,
			ARM_ACCESS_PERM_RW_RW); /* IPUv3M */
	X_ARM_MMU_SECTION(0x200, 0x200, 0x200,
			ARM_UNCACHEABLE, ARM_UNBUFFERABLE,
			ARM_ACCESS_PERM_RW_RW); /* GPU */
	X_ARM_MMU_SECTION(0x400, 0x400, 0x300,
			ARM_UNCACHEABLE, ARM_UNBUFFERABLE,
			ARM_ACCESS_PERM_RW_RW); /* periperals */
	X_ARM_MMU_SECTION(0x700, 0x700, 0x200,
			ARM_CACHEABLE, ARM_BUFFERABLE,
			ARM_ACCESS_PERM_RW_RW); /* CSD0 512M */
	X_ARM_MMU_SECTION(0x700, 0x900, 0x200,
			ARM_UNCACHEABLE, ARM_UNBUFFERABLE,
			ARM_ACCESS_PERM_RW_RW); /* CSD0 512M */
	X_ARM_MMU_SECTION(0xB00, 0xB00, 0x200,
			ARM_CACHEABLE, ARM_BUFFERABLE,
			ARM_ACCESS_PERM_RW_RW); /* CSD1 512M */
	X_ARM_MMU_SECTION(0xB00, 0xD00, 0x200,
			ARM_UNCACHEABLE, ARM_UNBUFFERABLE,
			ARM_ACCESS_PERM_RW_RW); /* CSD1 512M */
	X_ARM_MMU_SECTION(0xF00, 0xF00, 0x07F,
			ARM_UNCACHEABLE, ARM_UNBUFFERABLE,
			ARM_ACCESS_PERM_RW_RW); /* CS1 EIM control*/
	X_ARM_MMU_SECTION(0xF7F, 0xF7F, 0x001,
			ARM_UNCACHEABLE, ARM_UNBUFFERABLE,
			ARM_ACCESS_PERM_RW_RW); /* NAND Flash buffer */
	X_ARM_MMU_SECTION(0xF80, 0xF80, 0x080,
			ARM_UNCACHEABLE, ARM_UNBUFFERABLE,
			ARM_ACCESS_PERM_RW_RW); /* iRam + GPU3D + Reserved */

	/* Workaround for arm errata #709718 */
	/* Setup PRRR so device is always mapped to non-shared */
	asm volatile ("mrc p15, 0, %0, c10, c2, 0" : "=r"(i) : /*:*/);
	i &= (~(3 << 0x10));
	asm volatile ("mcr p15, 0, %0, c10, c2, 0" : : "r"(i) /*:*/);

	/* Enable MMU */
	MMU_ON();
}
#endif

#ifdef CONFIG_GET_CONS_FROM_CONS_VAR
struct serial_device *default_serial_console(void)
{
	int conidx = CONFIG_CONS_INDEX;
	char *s;

	if ((s = getenv("console")) != NULL) {
		if ((s = strstr(s, CONFIG_CONSOLE)) != NULL) {
			/* Index is only one digit... */
			conidx = s[strlen(CONFIG_CONSOLE)] - '0';
			if (conidx < 0 || conidx > 2)
				conidx = CONFIG_CONS_INDEX;
			if (conidx != CONFIG_CONS_INDEX)
				serial_mxc_devices[conidx].init();
		}
	}

	return &serial_mxc_devices[conidx];
}
#endif

int dram_init(void)
{
	gd->bd->bi_dram[0].start = PHYS_SDRAM_1;

	if (is_valid_hw_id(mod_hwid.variant)) {
		gd->bd->bi_dram[0].size = get_ram_from_hwid(mod_hwid.variant);
	} else {
		gd->bd->bi_dram[0].size = PHYS_SDRAM_1_SIZE;
	}
#if CONFIG_NR_DRAM_BANKS > 1
	gd->bd->bi_dram[1].start = PHYS_SDRAM_2;
	gd->bd->bi_dram[0].size = gd->bd->bi_dram[0].size / 2;
	gd->bd->bi_dram[1].size = gd->bd->bi_dram[0].size;

	/* If second bank is also active, touch that memory. This fixes a strange
	 * problem that was randomly happening while booting after a PMIC poweroff
	 * with the system hanging just after printing the Uncompressing linux
	 * message. Accessing the memory connected to CS1 seems to solve the issue. */
	memset((void *)gd->bd->bi_dram[1].start, 0, 100);
#endif
	return 0;
}

void mxc_serial_gpios_init(int port)
{
	switch (port) {
	case 0:
		/* UART1 RXD */
		mxc_request_iomux(MX53_PIN_ATA_DMACK, IOMUX_CONFIG_ALT3);
		mxc_iomux_set_pad(MX53_PIN_ATA_DMACK, 0x1E4);

		mxc_iomux_set_input(MUX_IN_UART1_IPP_UART_RXD_MUX_SELECT_INPUT, INPUT_CTL_PATH3);
		/* UART1 TXD */
		mxc_request_iomux(MX53_PIN_ATA_DIOW, IOMUX_CONFIG_ALT3);
		mxc_iomux_set_pad(MX53_PIN_ATA_DIOW, 0x1E4);
		break;
	case 1:
		/* UART2 RXD */
		mxc_request_iomux(MX53_PIN_ATA_BUFFER_EN, IOMUX_CONFIG_ALT3);
		mxc_iomux_set_pad(MX53_PIN_ATA_BUFFER_EN, 0x1E4);

		mxc_iomux_set_input(MUX_IN_UART2_IPP_UART_RXD_MUX_SELECT_INPUT, INPUT_CTL_PATH3);
		/* UART2 TXD */
		mxc_request_iomux(MX53_PIN_ATA_DMARQ, IOMUX_CONFIG_ALT3);
		mxc_iomux_set_pad(MX53_PIN_ATA_DMARQ, 0x1E4);
		break;
	case 2:
		/* UART2 RXD */
		mxc_request_iomux(MX53_PIN_ATA_CS_1, IOMUX_CONFIG_ALT4);
		mxc_iomux_set_pad(MX53_PIN_ATA_CS_1, 0x1E4);

		mxc_iomux_set_input(MUX_IN_UART3_IPP_UART_RXD_MUX_SELECT_INPUT, INPUT_CTL_PATH3);
		/* UART2 TXD */
		mxc_request_iomux(MX53_PIN_ATA_CS_0, IOMUX_CONFIG_ALT4);
		mxc_iomux_set_pad(MX53_PIN_ATA_CS_0, 0x1E4);
		break;
	case 3:
	case 4:	break;
	}
}

#define NAND_PAD_DATA	(PAD_CTL_PKE_ENABLE | PAD_CTL_100K_PU | PAD_CTL_DRV_HIGH)
#define NAND_PAD_CTRL	(PAD_CTL_PKE_ENABLE | PAD_CTL_100K_PU | PAD_CTL_PUE_PULL)
void setup_nfc(void)
{
	int i;

	/*
	 * To be compatible with some old NAND flash,
	 * limit NFC clocks as 34MHZ. The user can modify
	 * it according to dedicate NAND flash
	 */
	clk_config(0, 34, NFC_CLK);

	/*
	 * The nand pins should be already configured if we are booting
	 * from the nand, but may be the bootstrap is different.
	 * Therefore, we reconfig the pins here and the nand controller.
	 * TODO: we should check if we are booting from flash or SD card
	 * and, based on that, reconfigure the nand controller.
	 */
	mxc_request_iomux(MX53_PIN_ATA_DATA0, IOMUX_CONFIG_ALT3);
	mxc_request_iomux(MX53_PIN_ATA_DATA1, IOMUX_CONFIG_ALT3);
	mxc_request_iomux(MX53_PIN_ATA_DATA2, IOMUX_CONFIG_ALT3);
	mxc_request_iomux(MX53_PIN_ATA_DATA3, IOMUX_CONFIG_ALT3);
	mxc_request_iomux(MX53_PIN_ATA_DATA4, IOMUX_CONFIG_ALT3);
	mxc_request_iomux(MX53_PIN_ATA_DATA5, IOMUX_CONFIG_ALT3);
	mxc_request_iomux(MX53_PIN_ATA_DATA6, IOMUX_CONFIG_ALT3);
	mxc_request_iomux(MX53_PIN_ATA_DATA7, IOMUX_CONFIG_ALT3);

	mxc_request_iomux(MX53_PIN_NANDF_CS0, IOMUX_CONFIG_ALT0);
	mxc_request_iomux(MX53_PIN_NANDF_RE_B, IOMUX_CONFIG_ALT0);
	mxc_request_iomux(MX53_PIN_NANDF_WE_B, IOMUX_CONFIG_ALT0);
	mxc_request_iomux(MX53_PIN_NANDF_WP_B, IOMUX_CONFIG_ALT0);
	mxc_request_iomux(MX53_PIN_NANDF_ALE, IOMUX_CONFIG_ALT0);
	mxc_request_iomux(MX53_PIN_NANDF_CLE, IOMUX_CONFIG_ALT0);
	mxc_request_iomux(MX53_PIN_NANDF_RB0, IOMUX_CONFIG_ALT0);

	mxc_iomux_set_pad(MX53_PIN_ATA_DATA0, NAND_PAD_DATA);
	mxc_iomux_set_pad(MX53_PIN_ATA_DATA1, NAND_PAD_DATA);
	mxc_iomux_set_pad(MX53_PIN_ATA_DATA2, NAND_PAD_DATA);
	mxc_iomux_set_pad(MX53_PIN_ATA_DATA3, NAND_PAD_DATA);
	mxc_iomux_set_pad(MX53_PIN_ATA_DATA4, NAND_PAD_DATA);
	mxc_iomux_set_pad(MX53_PIN_ATA_DATA5, NAND_PAD_DATA);
	mxc_iomux_set_pad(MX53_PIN_ATA_DATA6, NAND_PAD_DATA);
	mxc_iomux_set_pad(MX53_PIN_ATA_DATA7, NAND_PAD_DATA);

	mxc_iomux_set_pad(MX53_PIN_NANDF_CS0, NAND_PAD_CTRL);
	mxc_iomux_set_pad(MX53_PIN_NANDF_RE_B, NAND_PAD_CTRL);
	mxc_iomux_set_pad(MX53_PIN_NANDF_WE_B, NAND_PAD_CTRL);
	mxc_iomux_set_pad(MX53_PIN_NANDF_WP_B, NAND_PAD_CTRL);
	mxc_iomux_set_pad(MX53_PIN_NANDF_ALE, NAND_PAD_CTRL);
	mxc_iomux_set_pad(MX53_PIN_NANDF_CLE, NAND_PAD_CTRL);
	mxc_iomux_set_pad(MX53_PIN_NANDF_RB0, NAND_PAD_CTRL);

	/* initial NAND setup */
	/* Unlock CS and block addresses */
	for (i = 0; i < 8; i++) {
		writel(0xFFFF0000, REG_UNLOCK_BLK_ADD0 + i * 4);
		writel(0x84 | (i << 3), NFC_WRPROT);
	}
	writel(0, NFC_IPC);
	writel(0, NFC_CONFIG1);
	writel(0x00108609, NFC_CONFIG3);
}

#ifdef CONFIG_AFTER_FLASH_INIT
void config_after_flash_init(void)
{
	u32 pagesize = NFC_PS_2K;

	/* Here the flash should have been detected, so we can use the information
	 * written in the CONFIG2 register to (the pagesize) to update same variables
	 * that depend on its value */
	pagesize = readl(NFC_CONFIG2) & ~NFC_FIELD_RESET(NFC_PS_WIDTH, NFC_PS_SHIFT);
	if (pagesize == NFC_PS_4K) {
		swap_bbi_limit = SKIP_SWAP_BI_MAX_PAGE_4K;
		uboot_part_size = PART_UBOOT_SIZE_4K;
	} else if (pagesize == NFC_PS_2K) {
		swap_bbi_limit = SKIP_SWAP_BI_MAX_PAGE_2K;
	}
}
#endif

#ifdef CONFIG_USER_LED
static void setup_user_leds(void)
{
#define USER_LED_PAD_CFG	(PAD_CTL_SRE_SLOW | PAD_CTL_DRV_MEDIUM | \
				 PAD_CTL_ODE_OPENDRAIN_ENABLE | PAD_CTL_HYS_NONE | \
				 PAD_CTL_PUE_KEEPER | PAD_CTL_PKE_NONE | PAD_CTL_DRV_VOT_LOW)
	/*
	 * Configure iomux for gpio operation and the corresponding
	 * gpios as inputs
	 */
	mxc_request_iomux(USER_LED1_GPIO, IOMUX_CONFIG_GPIO);
	mxc_iomux_set_pad(USER_LED1_GPIO, USER_LED_PAD_CFG);
	mxc_request_iomux(USER_LED2_GPIO, IOMUX_CONFIG_GPIO);
	mxc_iomux_set_pad(USER_LED2_GPIO, USER_LED_PAD_CFG);

	/* Confgiure direction as output */
	imx_gpio_pin_cfg_dir(USER_LED1_GPIO, 1);
	imx_gpio_pin_cfg_dir(USER_LED2_GPIO, 1);
}
#endif

#ifdef CONFIG_USER_KEY
#define USER_KEY_PAD_CFG	(PAD_CTL_SRE_SLOW | PAD_CTL_DRV_MEDIUM | \
				 PAD_CTL_ODE_OPENDRAIN_NONE | PAD_CTL_HYS_ENABLE | \
				 PAD_CTL_PUE_KEEPER | PAD_CTL_PKE_NONE | PAD_CTL_DRV_VOT_LOW)
static void setup_user_keys(void)
{
	/* Configure iomux for gpio operation */
	mxc_request_iomux(USER_KEY1_GPIO, IOMUX_CONFIG_GPIO);
	mxc_iomux_set_pad(USER_KEY1_GPIO, USER_KEY_PAD_CFG);

	mxc_request_iomux(USER_KEY2_GPIO, IOMUX_CONFIG_GPIO);
	mxc_iomux_set_pad(USER_KEY2_GPIO, USER_KEY_PAD_CFG);

	/* Confgiure direction as input */
	imx_gpio_pin_cfg_dir(USER_KEY1_GPIO, 0);
	imx_gpio_pin_cfg_dir(USER_KEY2_GPIO, 0);
}

static void run_user_keys(void)
{
	char cmd[60];

#ifdef CONFIG_USER_KEY12
	if ((imx_gpio_get_pin(USER_KEY1_GPIO) == 0) &&
	    (imx_gpio_get_pin(USER_KEY2_GPIO) == 0)) {
		printf("\nUser Key 1 and 2 pressed\n");
		if (getenv("key12") != NULL) {
			sprintf(cmd, "run key12");
			run_command(cmd, 0);
			return;
		}
	}
#endif /* CONFIG_USER_KEY12 */

	if (imx_gpio_get_pin(USER_KEY1_GPIO) == 0) {
		printf("\nUser Key 1 pressed\n");
		if (getenv("key1") != NULL) {
			sprintf(cmd, "run key1");
			run_command(cmd, 0);
		}
	}

	if (imx_gpio_get_pin(USER_KEY2_GPIO) == 0) {
		printf("\nUser Key 2 pressed\n");
		if (getenv("key2") != NULL) {
			sprintf(cmd, "run key2");
			run_command(cmd, 0);
		}
	}
}
#endif

#ifdef CONFIG_I2C_MXC
static void setup_i2c(unsigned int module_base)
{
	switch (module_base) {
#if (CONFIG_SYS_I2C_PORT == I2C1_BASE_ADDR)
	case I2C1_BASE_ADDR:
		/* i2c1 SDA */
		mxc_request_iomux(MX53_PIN_CSI0_D8,
				IOMUX_CONFIG_ALT5 | IOMUX_CONFIG_SION);
		mxc_iomux_set_input(MUX_IN_I2C1_IPP_SDA_IN_SELECT_INPUT,
				INPUT_CTL_PATH0);
		mxc_iomux_set_pad(MX53_PIN_CSI0_D8, PAD_CTL_SRE_FAST |
				PAD_CTL_ODE_OPENDRAIN_ENABLE |
				PAD_CTL_PKE_ENABLE | PAD_CTL_PUE_PULL |
				PAD_CTL_DRV_HIGH | PAD_CTL_100K_PU |
				PAD_CTL_HYS_ENABLE);
		/* i2c1 SCL */
		mxc_request_iomux(MX53_PIN_CSI0_D9,
				IOMUX_CONFIG_ALT5 | IOMUX_CONFIG_SION);
		mxc_iomux_set_input(MUX_IN_I2C1_IPP_SCL_IN_SELECT_INPUT,
				INPUT_CTL_PATH0);
		mxc_iomux_set_pad(MX53_PIN_CSI0_D9, PAD_CTL_SRE_FAST |
				PAD_CTL_ODE_OPENDRAIN_ENABLE |
				PAD_CTL_PKE_ENABLE | PAD_CTL_PUE_PULL |
				PAD_CTL_DRV_HIGH | PAD_CTL_100K_PU |
				PAD_CTL_HYS_ENABLE);
		break;
#endif /* (CONFIG_SYS_I2C_PORT == I2C1_BASE_ADDR) */
#if (CONFIG_SYS_I2C_PORT == I2C2_BASE_ADDR)
	case I2C2_BASE_ADDR:
		/* i2c2 SDA */
		mxc_request_iomux(MX53_PIN_KEY_ROW3,
				IOMUX_CONFIG_ALT4 | IOMUX_CONFIG_SION);
		mxc_iomux_set_input(MUX_IN_I2C2_IPP_SDA_IN_SELECT_INPUT,
				INPUT_CTL_PATH0);
		mxc_iomux_set_pad(MX53_PIN_KEY_ROW3,
				PAD_CTL_SRE_FAST |
				PAD_CTL_ODE_OPENDRAIN_ENABLE |
				PAD_CTL_DRV_HIGH | PAD_CTL_100K_PU |
				PAD_CTL_HYS_ENABLE);

		/* i2c2 SCL */
		mxc_request_iomux(MX53_PIN_KEY_COL3,
				IOMUX_CONFIG_ALT4 | IOMUX_CONFIG_SION);
		mxc_iomux_set_input(MUX_IN_I2C2_IPP_SCL_IN_SELECT_INPUT,
				INPUT_CTL_PATH0);
		mxc_iomux_set_pad(MX53_PIN_KEY_COL3,
				PAD_CTL_SRE_FAST |
				PAD_CTL_ODE_OPENDRAIN_ENABLE |
				PAD_CTL_DRV_HIGH | PAD_CTL_100K_PU |
				PAD_CTL_HYS_ENABLE);
		break;
#endif /* (CONFIG_SYS_I2C_PORT == I2C2_BASE_ADDR) */
#if (CONFIG_SYS_I2C_PORT == I2C3_BASE_ADDR)
	case I2C3_BASE_ADDR:
#ifdef CONFIG_CAMERA_RESET_WORKAROUND
		/* If cameras are connected to I2C3, they may leave the
		 * I2C lines in a state that prevents correct communication
		 * with the PMIC, leading to boot problems. This patch drives
		 * the cameras reset line low to hold the I2C lines in high
		 * impedance.
		 */
#define CAM_RESET_GPIO_PAD_CFG	(PAD_CTL_SRE_SLOW | PAD_CTL_DRV_MEDIUM | \
				 PAD_CTL_ODE_OPENDRAIN_NONE | PAD_CTL_HYS_ENABLE | \
				 PAD_CTL_PUE_PULL | PAD_CTL_PKE_ENABLE | \
				 PAD_CTL_100K_PU | PAD_CTL_DRV_VOT_LOW)

		mxc_request_iomux(CONFIG_CAMERA0_RESET_GPIO, IOMUX_CONFIG_GPIO);
		mxc_request_iomux(CONFIG_CAMERA1_RESET_GPIO, IOMUX_CONFIG_GPIO);
		mxc_iomux_set_pad(CONFIG_CAMERA0_RESET_GPIO, CAM_RESET_GPIO_PAD_CFG);
		mxc_iomux_set_pad(CONFIG_CAMERA1_RESET_GPIO, CAM_RESET_GPIO_PAD_CFG);
		imx_gpio_set_pin(CONFIG_CAMERA0_RESET_GPIO, 0);
		imx_gpio_set_pin(CONFIG_CAMERA1_RESET_GPIO, 0);
		imx_gpio_pin_cfg_dir(CONFIG_CAMERA0_RESET_GPIO, 1);
		imx_gpio_pin_cfg_dir(CONFIG_CAMERA1_RESET_GPIO, 1);
#endif /* CONFIG_CAMERA_RESET_WORKAROUND */

		/* i2c3 SDA */
		mxc_request_iomux(MX53_PIN_GPIO_6,
				IOMUX_CONFIG_ALT2 | IOMUX_CONFIG_SION);
		mxc_iomux_set_input(MUX_IN_I2C3_IPP_SDA_IN_SELECT_INPUT,
				INPUT_CTL_PATH1);
		mxc_iomux_set_pad(MX53_PIN_GPIO_6,
				PAD_CTL_SRE_FAST |
				PAD_CTL_ODE_OPENDRAIN_ENABLE |
				PAD_CTL_DRV_HIGH | PAD_CTL_100K_PU |
				PAD_CTL_HYS_ENABLE);
		/* i2c3 SCL */
		mxc_request_iomux(MX53_PIN_GPIO_5,
				IOMUX_CONFIG_ALT6 | IOMUX_CONFIG_SION);
		mxc_iomux_set_input(MUX_IN_I2C3_IPP_SCL_IN_SELECT_INPUT,
				INPUT_CTL_PATH2);
		mxc_iomux_set_pad(MX53_PIN_GPIO_5,
				PAD_CTL_SRE_FAST |
				PAD_CTL_ODE_OPENDRAIN_ENABLE |
				PAD_CTL_DRV_HIGH | PAD_CTL_100K_PU |
				PAD_CTL_HYS_ENABLE);
		break;
#endif /* (CONFIG_SYS_I2C_PORT == I2C3_BASE_ADDR) */
	default:
		printf("Invalid I2C base: 0x%x, or not active IF\n", module_base);
		break;
	}
}

#ifdef CONFIG_I2C_RECOVER_STUCK
#define I2C_GPIO_PAD_CFG	(PAD_CTL_SRE_SLOW | PAD_CTL_DRV_MEDIUM | \
				 PAD_CTL_ODE_OPENDRAIN_NONE | PAD_CTL_HYS_ENABLE | \
				 PAD_CTL_PUE_KEEPER | PAD_CTL_PKE_NONE | PAD_CTL_DRV_VOT_LOW)
void i2c_set_pins_as_gpio(void)
{
	/* SDA as input gpio */
	mxc_request_iomux(CONFIG_I2C_SDA_GPIO, IOMUX_CONFIG_GPIO);
	mxc_iomux_set_pad(CONFIG_I2C_SDA_GPIO, I2C_GPIO_PAD_CFG);
	imx_gpio_pin_cfg_dir(CONFIG_I2C_SDA_GPIO, 0);

	/* SCL as input gpio */
	mxc_request_iomux(CONFIG_I2C_SCL_GPIO, IOMUX_CONFIG_GPIO);
	mxc_iomux_set_pad(CONFIG_I2C_SCL_GPIO, I2C_GPIO_PAD_CFG);
	imx_gpio_pin_cfg_dir(CONFIG_I2C_SCL_GPIO, 0);
}

void i2c_recover(void)
{
	int i;

	i2c_set_pins_as_gpio();

	/* Try to recover by clocking the bus (bitbanging) until the data line goes up */
	imx_gpio_set_pin(CONFIG_I2C_SCL_GPIO, 1);
	for (i = 0; i < 20; i++) {
		imx_gpio_set_pin(CONFIG_I2C_SCL_GPIO, 0);
		imx_gpio_pin_cfg_dir(CONFIG_I2C_SCL_GPIO, 1);
		udelay(100);
		imx_gpio_pin_cfg_dir(CONFIG_I2C_SCL_GPIO, 0);
		udelay(100);
	}
	/* reconfigure the lines as i2c */
	setup_i2c(CONFIG_SYS_I2C_PORT);
}

int i2c_verify_stuck_and_recover(void)
{
	int i2c_ready = 1, i;

	i2c_set_pins_as_gpio();

	/* First of all, verify that both i2c lines are ready (pulled up) */
	if (imx_gpio_get_pin(CONFIG_I2C_SDA_GPIO) != 1)
		i2c_ready = 0;

	if (imx_gpio_get_pin(CONFIG_I2C_SCL_GPIO) != 1)
		i2c_ready = 0;

	/* If not, try to recover by clocking the bus (bitbanging) */
	if (!i2c_ready) {
		imx_gpio_set_pin(CONFIG_I2C_SCL_GPIO, 1);

		for (i = 0; i < 50; i++) {
			if (imx_gpio_get_pin(CONFIG_I2C_SDA_GPIO) == 1)
				break;
			imx_gpio_set_pin(CONFIG_I2C_SCL_GPIO, 0);
			imx_gpio_pin_cfg_dir(CONFIG_I2C_SCL_GPIO, 1);
			udelay(100);
			imx_gpio_pin_cfg_dir(CONFIG_I2C_SCL_GPIO, 0);
			udelay(100);
		}
	}
	/* reconfigure the lines as i2c */
	setup_i2c(CONFIG_SYS_I2C_PORT);

	return i2c_ready;
}
#endif /* CONFIG_I2C_RECOVER_STUCK */

void setup_pmic_voltages(void)
{
	int val;
	u8 hwrev;

	/* Check if revisions are different than 0x00 or 0x01. If that is the case,
	 * use the alternative pmic i2c address. The address was changed due to an
	 * address colision between the PMIC and the camera (when using dual camera.
	 * Hardware revisions equal to 0x02 and higher come with the new PMIC i2c
	 * address programmed so, we now determine dinamically the address of the
	 * PMiC. */
	hwrev = get_hw_rev_from_fuse();
	if (hwrev == 1)
		da905x_i2c_addr = DA905x_I2C_ADDR;

	i2c_init(CONFIG_SYS_I2C_SPEED, CONFIG_SYS_I2C_SLAVE);

	/* Configure the power sequencer timer to the maximum. Keep and eye on this...
	 * Long sequence times was causing problems with the RTC, when the PMIC was
	 * not switching fast enough to the coin cell power.
	 * On the other hand, short sequence times was causing i2c communication
	 * problems with the PMIC. */
	pmic_wr_reg(43, 0xfe);

	/* Switch LDO8 and LDO9 (1.8V and 2.775V) to sequence slot 6 */
	pmic_wr_reg(33, 0x66);

	/* increase VDDGP as 1.2V for 1GHZ */
	pmic_wr_reg(0x2e, 0x5e);
	val = pmic_rd_reg(0x3c);
	pmic_wr_reg(0x3c, (unsigned char)val | 0x01);

	/* Set 3.3V in VLDO3 */
	pmic_wr_reg(0x34, 0x7f);
	/* Set VBUCKMEM to 1.8V */
	pmic_wr_reg(0x30, 0x63);

	/* Ramp VBUCKMEM and VLDO3 */;
	(void)pmic_wr_msk_reg(0x3c, 0x14, 0x14);

	/* Increase the current limit to the max */
	pmic_wr_reg(0x3e, 0x9f);

	/* Activate the 3.3V regulator */
	pmic_wr_reg(0x19, 0xfa);


	/* Increase VCC as 1.35V for TO2.0 */
	if (is_soc_rev(CHIP_REV_2_0) == 0) {
		/* bit[5:0] VBPRO: 0x22 (1.35V)
		 * bit[6] BPRO_EN: 1 - enable
		 */
		pmic_wr_reg(0x2f, 0x62);
		pmic_wr_reg(0x3c, 0x62);
	}

	(void)pmic_wr_msk_reg(17, 0x00, 0x10);  /* Disable onkey */
}
#endif

#if defined(CONFIG_DWC_AHSATA)
static void setup_sata_device(void)
{
	u32 *tmp_base =
		(u32 *)(IIM_BASE_ADDR + 0x180c);

	/* Set USB_PHY1 clk, fuse bank4 row3 bit2 */
	set_usb_phy1_clk();
	writel((readl(tmp_base) & (~0x7)) | 0x4, tmp_base);
}
#endif

#ifdef CONFIG_MXC_FEC
int fec_get_mac_addr(unsigned char *mac)
{
	nv_critical_t *pNvram;

	if (!NvCriticalGet(&pNvram))
		return -1;

	memcpy(mac, &pNvram->s.p.xID.axMAC[0], 6);

	return 0;
}

static void setup_fec(void)
{
	/*FEC_MDIO*/
	mxc_request_iomux(MX53_PIN_FEC_MDIO, IOMUX_CONFIG_ALT0);
	mxc_iomux_set_pad(MX53_PIN_FEC_MDIO, 0x1FC);
	mxc_iomux_set_input(MUX_IN_FEC_FEC_MDI_SELECT_INPUT, 0x1);

	/*FEC_MDC*/
	mxc_request_iomux(MX53_PIN_FEC_MDC, IOMUX_CONFIG_ALT0);
	mxc_iomux_set_pad(MX53_PIN_FEC_MDC, 0x004);

	/* FEC RXD1 */
	mxc_request_iomux(MX53_PIN_FEC_RXD1, IOMUX_CONFIG_ALT0);
	mxc_iomux_set_pad(MX53_PIN_FEC_RXD1, 0x180);

	/* FEC RXD0 */
	mxc_request_iomux(MX53_PIN_FEC_RXD0, IOMUX_CONFIG_ALT0);
	mxc_iomux_set_pad(MX53_PIN_FEC_RXD0, 0x180);

	 /* FEC TXD1 */
	mxc_request_iomux(MX53_PIN_FEC_TXD1, IOMUX_CONFIG_ALT0);
	mxc_iomux_set_pad(MX53_PIN_FEC_TXD1, 0x004);

	/* FEC TXD0 */
	mxc_request_iomux(MX53_PIN_FEC_TXD0, IOMUX_CONFIG_ALT0);
	mxc_iomux_set_pad(MX53_PIN_FEC_TXD0, 0x004);

	/* FEC TX_EN */
	mxc_request_iomux(MX53_PIN_FEC_TX_EN, IOMUX_CONFIG_ALT0);
	mxc_iomux_set_pad(MX53_PIN_FEC_TX_EN, 0x004);

	/* FEC TX_CLK */
	mxc_request_iomux(MX53_PIN_FEC_REF_CLK, IOMUX_CONFIG_ALT0);
	mxc_iomux_set_pad(MX53_PIN_FEC_REF_CLK, 0x180);

	/* FEC RX_ER */
	mxc_request_iomux(MX53_PIN_FEC_RX_ER, IOMUX_CONFIG_ALT0);
	mxc_iomux_set_pad(MX53_PIN_FEC_RX_ER, 0x180);

	/* FEC CRS */
	mxc_request_iomux(MX53_PIN_FEC_CRS_DV, IOMUX_CONFIG_ALT0);
	mxc_iomux_set_pad(MX53_PIN_FEC_CRS_DV, 0x180);

	/* Configure DA9053 GPIO11 as output and reset the phy */
	(void)pmic_wr_msk_reg(DA9053_GPIO_10_11, 0x20, 0xf0);	/* PHY reset line to low (active low) */
	udelay(100);
	(void)pmic_wr_msk_reg(DA9053_GPIO_10_11, 0xb0, 0xf0);	/* PHY reset line to high */
}
#endif


#ifdef CONFIG_NET_MULTI
#if defined(CONFIG_SMC911X)
#include <net.h>
#include "../../../drivers/net/smc911x.h"
u32 mx53_io_base_addr;
extern int smc911x_initialize(u8 dev_num, int base_addr);
#endif
int board_eth_init(bd_t *bis)
{
	int rc = -ENODEV;
#if defined(CONFIG_SMC911X)
	struct eth_device *dev;
	nv_critical_t *pNvram;
	unsigned long addrh, addrl;
	uchar *m;
	unsigned int reg;


	/* Configure the pin mux for CS line and address and data buses */

	/* Data bus */
	mxc_request_iomux(MX53_PIN_EIM_D16, IOMUX_CONFIG_ALT0);
	mxc_iomux_set_pad(MX53_PIN_EIM_D16, 0xA4);
	mxc_request_iomux(MX53_PIN_EIM_D17, IOMUX_CONFIG_ALT0);
	mxc_iomux_set_pad(MX53_PIN_EIM_D17, 0xA4);
	mxc_request_iomux(MX53_PIN_EIM_D18, IOMUX_CONFIG_ALT0);
	mxc_iomux_set_pad(MX53_PIN_EIM_D18, 0xA4);
	mxc_request_iomux(MX53_PIN_EIM_D19, IOMUX_CONFIG_ALT0);
	mxc_iomux_set_pad(MX53_PIN_EIM_D19, 0xA4);
	mxc_request_iomux(MX53_PIN_EIM_D20, IOMUX_CONFIG_ALT0);
	mxc_iomux_set_pad(MX53_PIN_EIM_D20, 0xA4);
	mxc_request_iomux(MX53_PIN_EIM_D21, IOMUX_CONFIG_ALT0);
	mxc_iomux_set_pad(MX53_PIN_EIM_D21, 0xA4);
	mxc_request_iomux(MX53_PIN_EIM_D22, IOMUX_CONFIG_ALT0);
	mxc_iomux_set_pad(MX53_PIN_EIM_D22, 0xA4);
	mxc_request_iomux(MX53_PIN_EIM_D23, IOMUX_CONFIG_ALT0);
	mxc_iomux_set_pad(MX53_PIN_EIM_D23, 0xA4);
	mxc_request_iomux(MX53_PIN_EIM_D24, IOMUX_CONFIG_ALT0);
	mxc_iomux_set_pad(MX53_PIN_EIM_D24, 0xA4);
	mxc_request_iomux(MX53_PIN_EIM_D25, IOMUX_CONFIG_ALT0);
	mxc_iomux_set_pad(MX53_PIN_EIM_D25, 0xA4);
	mxc_request_iomux(MX53_PIN_EIM_D26, IOMUX_CONFIG_ALT0);
	mxc_iomux_set_pad(MX53_PIN_EIM_D26, 0xA4);
	mxc_request_iomux(MX53_PIN_EIM_D27, IOMUX_CONFIG_ALT0);
	mxc_iomux_set_pad(MX53_PIN_EIM_D27, 0xA4);
	mxc_request_iomux(MX53_PIN_EIM_D28, IOMUX_CONFIG_ALT0);
	mxc_iomux_set_pad(MX53_PIN_EIM_D28, 0xA4);
	mxc_request_iomux(MX53_PIN_EIM_D29, IOMUX_CONFIG_ALT0);
	mxc_iomux_set_pad(MX53_PIN_EIM_D29, 0xA4);
	mxc_request_iomux(MX53_PIN_EIM_D30, IOMUX_CONFIG_ALT0);
	mxc_iomux_set_pad(MX53_PIN_EIM_D30, 0xA4);
	mxc_request_iomux(MX53_PIN_EIM_D31, IOMUX_CONFIG_ALT0);
	mxc_iomux_set_pad(MX53_PIN_EIM_D31, 0xA4);

	/* Address lines */
	mxc_request_iomux(MX53_PIN_EIM_DA0, IOMUX_CONFIG_ALT0);
	mxc_iomux_set_pad(MX53_PIN_EIM_DA0, 0xA4);
	mxc_request_iomux(MX53_PIN_EIM_DA1, IOMUX_CONFIG_ALT0);
	mxc_iomux_set_pad(MX53_PIN_EIM_DA1, 0xA4);
	mxc_request_iomux(MX53_PIN_EIM_DA2, IOMUX_CONFIG_ALT0);
	mxc_iomux_set_pad(MX53_PIN_EIM_DA2, 0xA4);
	mxc_request_iomux(MX53_PIN_EIM_DA3, IOMUX_CONFIG_ALT0);
	mxc_iomux_set_pad(MX53_PIN_EIM_DA3, 0xA4);
	mxc_request_iomux(MX53_PIN_EIM_DA4, IOMUX_CONFIG_ALT0);
	mxc_iomux_set_pad(MX53_PIN_EIM_DA4, 0xA4);
	mxc_request_iomux(MX53_PIN_EIM_DA5, IOMUX_CONFIG_ALT0);
	mxc_iomux_set_pad(MX53_PIN_EIM_DA5, 0xA4);
	mxc_request_iomux(MX53_PIN_EIM_DA6, IOMUX_CONFIG_ALT0);
	mxc_iomux_set_pad(MX53_PIN_EIM_DA6, 0xA4);

	/* OE, RW and CS1 */
	mxc_request_iomux(MX53_PIN_EIM_OE, IOMUX_CONFIG_ALT0);
	mxc_request_iomux(MX53_PIN_EIM_RW, IOMUX_CONFIG_ALT0);
	mxc_request_iomux(MX53_PIN_EIM_CS1, IOMUX_CONFIG_ALT0);

	/* Configure the CS timming, bus width, etc */
	/* 16 bit on DATA[31..16], not multiplexed, async */
	writel(0x00420081, WEIM_BASE_ADDR + 0x18 + CSGCR1);
	/* ADH has not effect on non muxed bus */
	writel(0, WEIM_BASE_ADDR + 0x18 + CSGCR2);
	/* RWSC=50, RADVA=2, RADVN=6, OEA=0, OEN=0, RCSA=0, RCSN=0 */
	writel(0x32260000, WEIM_BASE_ADDR + 0x18 + CSRCR1);
	/* APR=0 */
	writel(0, WEIM_BASE_ADDR + 0x18 + CSRCR2);
	/* WAL=0, WBED=1, WWSC=50, WADVA=2, WADVN=6, WEA=0, WEN=0, WCSA=0 */
	writel(0x72080f00, WEIM_BASE_ADDR + 0x18 + CSWCR1);
	writel(0, WEIM_BASE_ADDR + 0x18 + CSWCR2);

	mx53_io_base_addr = CS1_BASE_ADDR;

	/* specify 64 MB on CS1 and CS0 */
	reg = readl(IOMUXC_BASE_ADDR + 0x4);
	reg &= ~0x3F;
	reg |= 0x1B;
	writel(reg, (IOMUXC_BASE_ADDR + 0x4));

	/* Configure DA9053 GPIO12 as output, push-pull 1.8V and reset the
	 * SMSC ethernet controller setting the output low - high */
	(void)pmic_wr_msk_reg(DA9053_GPIO_12_13, 0x07, 0x0f);
	udelay(100);
	(void)pmic_wr_msk_reg(DA9053_GPIO_12_13, 0x0f, 0x0f);

	/* ...and lets initialize the device */
	rc = smc911x_initialize(0, CONFIG_SMC911X_BASE);
	if (!rc) {
		/* set mac address even if there is no access to the network */
		if ((dev = eth_get_dev_by_name("smc911x-0")) != NULL) {
			if (!NvCriticalGet(&pNvram))
				return -1;
			m = (uchar *)&pNvram->s.p.eth1addr;

			addrl = m[0] | (m[1] << 8) | (m[2] << 16) | (m[3] << 24);
			addrh = m[4] | (m[5] << 8);
			smc911x_set_mac_csr(dev, ADDRL, addrl);
			smc911x_set_mac_csr(dev, ADDRH, addrh);
		}
	}
#endif
	return rc;
}
#endif

#ifdef CONFIG_CMD_MMC

struct fsl_esdhc_cfg esdhc_cfg[2] = MMC_CFGS;

#ifdef CONFIG_DYNAMIC_MMC_DEVNO
int get_mmc_env_devno(void)
{
	uint soc_sbmr = readl(SRC_BASE_ADDR + 0x4);
	return (soc_sbmr & 0x00300000) ? 1 : 0;
}
#endif

#ifdef CONFIG_EMMC_DDR_PORT_DETECT
int detect_mmc_emmc_ddr_port(struct fsl_esdhc_cfg *cfg)
{
	return (MMC_SDHC3_BASE_ADDR == cfg->esdhc_base) ? 1 : 0;
}
#endif

#define MX53_SDHC_PAD_CTRL 	(PAD_CTL_HYS_ENABLE | PAD_CTL_PKE_ENABLE | PAD_CTL_PUE_PULL | \
				PAD_CTL_75K_PU | PAD_CTL_DRV_HIGH | PAD_CTL_SRE_FAST)
int esdhc_gpio_init(bd_t *bis)
{
	s32 status = 0;
	u32 index = 0;

	for (index = 0; index < CONFIG_SYS_FSL_ESDHC_NUM; ++index) {
		switch (index) {
		case 0:
			/* SD2 CMD */
			mxc_request_iomux(MX53_PIN_SD2_CMD, IOMUX_CONFIG_ALT0);
			mxc_iomux_set_pad(MX53_PIN_SD2_CMD, MX53_SDHC_PAD_CTRL);
			/* SD2 CLK */
			mxc_request_iomux(MX53_PIN_SD2_CLK, IOMUX_CONFIG_ALT0);
			mxc_iomux_set_pad(MX53_PIN_SD2_CLK, MX53_SDHC_PAD_CTRL);
			/* SD2 DATA0 */
			mxc_request_iomux(MX53_PIN_SD2_DATA0, IOMUX_CONFIG_ALT0);
			mxc_iomux_set_pad(MX53_PIN_SD2_DATA0, MX53_SDHC_PAD_CTRL);
			/* SD2 DATA1 */
			mxc_request_iomux(MX53_PIN_SD2_DATA1, IOMUX_CONFIG_ALT0);
			mxc_iomux_set_pad(MX53_PIN_SD2_DATA1, MX53_SDHC_PAD_CTRL);
			/* SD2 DATA2 */
			mxc_request_iomux(MX53_PIN_SD2_DATA2, IOMUX_CONFIG_ALT0);
			mxc_iomux_set_pad(MX53_PIN_SD2_DATA2, MX53_SDHC_PAD_CTRL);
			/* SD2 DATA3 */
			mxc_request_iomux(MX53_PIN_SD2_DATA3, IOMUX_CONFIG_ALT0);
			mxc_iomux_set_pad(MX53_PIN_SD2_DATA3, MX53_SDHC_PAD_CTRL);
			break;
		case 1:
			/* SD3 CMD */
			mxc_request_iomux(MX53_PIN_ATA_RESET_B,	IOMUX_CONFIG_ALT2);
			mxc_iomux_set_pad(MX53_PIN_ATA_RESET_B, 0x1E4);
			/* SD3 CLK */
			mxc_request_iomux(MX53_PIN_ATA_IORDY, IOMUX_CONFIG_ALT2);
			mxc_iomux_set_pad(MX53_PIN_ATA_IORDY, 0xD4);
			/* SD3 DATA0 */
			mxc_request_iomux(MX53_PIN_ATA_DATA8, IOMUX_CONFIG_ALT4);
			mxc_iomux_set_pad(MX53_PIN_ATA_DATA8, 0x1D4);
			/* SD3 DATA1 */
			mxc_request_iomux(MX53_PIN_ATA_DATA9, IOMUX_CONFIG_ALT4);
			mxc_iomux_set_pad(MX53_PIN_ATA_DATA9, 0x1D4);
			/* SD3 DATA2 */
			mxc_request_iomux(MX53_PIN_ATA_DATA10, IOMUX_CONFIG_ALT4);
			mxc_iomux_set_pad(MX53_PIN_ATA_DATA10, 0x1D4);
			/* SD3 DATA3 */
			mxc_request_iomux(MX53_PIN_ATA_DATA11, IOMUX_CONFIG_ALT4);
			mxc_iomux_set_pad(MX53_PIN_ATA_DATA11, 0x1D4);
			break;

		default:
			printf("Warning: you configured more ESDHC controller"
				"(%d) as supported by the board(2)\n",
				CONFIG_SYS_FSL_ESDHC_NUM);
			return status;
		}
		status |= fsl_esdhc_initialize(bis, &esdhc_cfg[index]);
	}

	return status;
}

int board_mmc_init(bd_t *bis)
{
	if (!esdhc_gpio_init(bis))
		return 0;
	else
		return -1;
}

#endif

#ifdef CONFIG_LCD
#include "../include/displays/displays.h"

static unsigned short ipu_display_index;

void lcd_gpio_init(void)
{
#define DISP0_PAD		(PAD_CTL_DRV_HIGH | PAD_CTL_SRE_FAST)
#define DISP0_PAD_DATA		(PAD_CTL_DRV_HIGH | PAD_CTL_SRE_SLOW)

	mxc_request_iomux(MX53_PIN_DI0_DISP_CLK, IOMUX_CONFIG_ALT0);
	mxc_iomux_set_pad(MX53_PIN_DI0_DISP_CLK, DISP0_PAD);
	mxc_request_iomux(MX53_PIN_DI0_PIN15, IOMUX_CONFIG_ALT0);
	mxc_iomux_set_pad(MX53_PIN_DI0_PIN15, DISP0_PAD);
	mxc_request_iomux(MX53_PIN_DI0_PIN2, IOMUX_CONFIG_ALT0);
	mxc_iomux_set_pad(MX53_PIN_DI0_PIN2, DISP0_PAD);
	mxc_request_iomux(MX53_PIN_DI0_PIN3, IOMUX_CONFIG_ALT0);
	mxc_iomux_set_pad(MX53_PIN_DI0_PIN3, DISP0_PAD);
	mxc_request_iomux(MX53_PIN_DISP0_DAT0, IOMUX_CONFIG_ALT0);
	mxc_iomux_set_pad(MX53_PIN_DISP0_DAT0, DISP0_PAD_DATA);
	mxc_request_iomux(MX53_PIN_DISP0_DAT1, IOMUX_CONFIG_ALT0);
	mxc_iomux_set_pad(MX53_PIN_DISP0_DAT1, DISP0_PAD_DATA);
	mxc_request_iomux(MX53_PIN_DISP0_DAT2, IOMUX_CONFIG_ALT0);
	mxc_iomux_set_pad(MX53_PIN_DISP0_DAT2, DISP0_PAD_DATA);
	mxc_request_iomux(MX53_PIN_DISP0_DAT3, IOMUX_CONFIG_ALT0);
	mxc_iomux_set_pad(MX53_PIN_DISP0_DAT3, DISP0_PAD_DATA);
	mxc_request_iomux(MX53_PIN_DISP0_DAT4, IOMUX_CONFIG_ALT0);
	mxc_iomux_set_pad(MX53_PIN_DISP0_DAT4, DISP0_PAD_DATA);
	mxc_request_iomux(MX53_PIN_DISP0_DAT5, IOMUX_CONFIG_ALT0);
	mxc_iomux_set_pad(MX53_PIN_DISP0_DAT5, DISP0_PAD_DATA);
	mxc_request_iomux(MX53_PIN_DISP0_DAT6, IOMUX_CONFIG_ALT0);
	mxc_iomux_set_pad(MX53_PIN_DISP0_DAT6, DISP0_PAD_DATA);
	mxc_request_iomux(MX53_PIN_DISP0_DAT7, IOMUX_CONFIG_ALT0);
	mxc_iomux_set_pad(MX53_PIN_DISP0_DAT7, DISP0_PAD_DATA);
	mxc_request_iomux(MX53_PIN_DISP0_DAT8, IOMUX_CONFIG_ALT0);
	mxc_iomux_set_pad(MX53_PIN_DISP0_DAT8, DISP0_PAD_DATA);
	mxc_request_iomux(MX53_PIN_DISP0_DAT9, IOMUX_CONFIG_ALT0);
	mxc_iomux_set_pad(MX53_PIN_DISP0_DAT9, DISP0_PAD_DATA);
	mxc_request_iomux(MX53_PIN_DISP0_DAT10, IOMUX_CONFIG_ALT0);
	mxc_iomux_set_pad(MX53_PIN_DISP0_DAT10, DISP0_PAD_DATA);
	mxc_request_iomux(MX53_PIN_DISP0_DAT11, IOMUX_CONFIG_ALT0);
	mxc_iomux_set_pad(MX53_PIN_DISP0_DAT11, DISP0_PAD_DATA);
	mxc_request_iomux(MX53_PIN_DISP0_DAT12, IOMUX_CONFIG_ALT0);
	mxc_iomux_set_pad(MX53_PIN_DISP0_DAT12, DISP0_PAD_DATA);
	mxc_request_iomux(MX53_PIN_DISP0_DAT13, IOMUX_CONFIG_ALT0);
	mxc_iomux_set_pad(MX53_PIN_DISP0_DAT13, DISP0_PAD_DATA);
	mxc_request_iomux(MX53_PIN_DISP0_DAT14, IOMUX_CONFIG_ALT0);
	mxc_iomux_set_pad(MX53_PIN_DISP0_DAT14, DISP0_PAD_DATA);
	mxc_request_iomux(MX53_PIN_DISP0_DAT15, IOMUX_CONFIG_ALT0);
	mxc_iomux_set_pad(MX53_PIN_DISP0_DAT15, DISP0_PAD_DATA);
	mxc_request_iomux(MX53_PIN_DISP0_DAT16, IOMUX_CONFIG_ALT0);
	mxc_iomux_set_pad(MX53_PIN_DISP0_DAT16, DISP0_PAD_DATA);
	mxc_request_iomux(MX53_PIN_DISP0_DAT17, IOMUX_CONFIG_ALT0);
	mxc_iomux_set_pad(MX53_PIN_DISP0_DAT17, DISP0_PAD_DATA);
#if defined(IPU_PIX_FMT_RGB24)
	mxc_request_iomux(MX53_PIN_DISP0_DAT18, IOMUX_CONFIG_ALT0);
	mxc_iomux_set_pad(MX53_PIN_DISP0_DAT18, DISP0_PAD_DATA);
	mxc_request_iomux(MX53_PIN_DISP0_DAT19, IOMUX_CONFIG_ALT0);
	mxc_iomux_set_pad(MX53_PIN_DISP0_DAT19, DISP0_PAD_DATA);
	mxc_request_iomux(MX53_PIN_DISP0_DAT20, IOMUX_CONFIG_ALT0);
	mxc_iomux_set_pad(MX53_PIN_DISP0_DAT20, DISP0_PAD_DATA);
	mxc_request_iomux(MX53_PIN_DISP0_DAT21, IOMUX_CONFIG_ALT0);
	mxc_iomux_set_pad(MX53_PIN_DISP0_DAT21, DISP0_PAD_DATA);
	mxc_request_iomux(MX53_PIN_DISP0_DAT22, IOMUX_CONFIG_ALT0);
	mxc_iomux_set_pad(MX53_PIN_DISP0_DAT22, DISP0_PAD_DATA);
	mxc_request_iomux(MX53_PIN_DISP0_DAT23, IOMUX_CONFIG_ALT0);
	mxc_iomux_set_pad(MX53_PIN_DISP0_DAT23, DISP0_PAD_DATA);
#endif
	mxc_request_iomux(MX53_PIN_DI0_PIN4, IOMUX_CONFIG_GPIO);
	mxc_iomux_set_pad(MX53_PIN_DI0_PIN4, PAD_CTL_PKE_ENABLE |
		PAD_CTL_PUE_KEEPER | PAD_CTL_DRV_MEDIUM | PAD_CTL_SRE_FAST);
}

int lcd_display_init(void)
{
	int i;
	char *s, *display;
	char scopy[30];

	display_initialized = 0;

	s = (char *)getenv("videoinit");
	if ((s != NULL) && !strcmp(s, "no"))
		return 0;

	/* Get selected display from 'video' variable, in the format:
	 * displayfb:DISPLAYNAME */
	s = (char *)getenv("video");
	if (s == NULL)
		return -1;

	strcpy(scopy,s);
	s = scopy;

	for (i = 0; i < 2; i++)
		display = strsep(&s, ":");
	/* Find the matching display in the array */
	for (i = 0; i < ARRAY_SIZE(ipu_displays); i++) {
		if (!strcmp(display, ipu_displays[i].name)) {
			/* Extract display info */
			ipu_display_index = i;
			break;
		}
	}
	if( ARRAY_SIZE(ipu_displays) == i ) {
		printf("error: unknown display selected\n");
		return -1;
	}

	lcd_gpio_init();

	/* Set other default parameters */
	panel_info.vl_col = ipu_displays[ipu_display_index].xres;
	panel_info.vl_row = ipu_displays[ipu_display_index].yres;
	panel_info.vl_bpix = LCD_BPP;
	panel_info.cmap = colormap;

	display_initialized = 1;

	return 0;
}

void lcd_enable(void)
{
	int ret;

	/* GPIO backlight */
	imx_gpio_pin_cfg_dir(MX53_PIN_DI0_PIN4, 1);
	imx_gpio_set_pin(MX53_PIN_DI0_PIN4, 0);

	ret = ipuv3_fb_init(&ipu_displays[ipu_display_index], 0, CONFIG_IPU_PIX_FMT,
			DI_PCLK_PLL3, 0);
	if (ret)
		puts("LCD cannot be configured\n");
}
#endif

int board_init(void)
{
	uint soc_src;

	/* Clear the warm reset enable, this has been causing boot problems
	 * (the system was not able to reboot) under certain conditions (some
	 * times, not often, when reseting the cpu with the watchdog).
	 * This forces that all resets will be cold resets. */
	soc_src = readl(SRC_BASE_ADDR);
	soc_src &= ~1;
	writel(soc_src, SRC_BASE_ADDR);

#ifdef CONFIG_MFG
/* MFG firmware need reset usb to avoid host crash firstly */
#define USBCMD 0x140
	int val = readl(OTG_BASE_ADDR + USBCMD);
	val &= ~0x1; /*RS bit*/
	writel(val, OTG_BASE_ADDR + USBCMD);
#endif
	setup_boot_device();
	setup_soc_rev();

	/* address of boot parameters */
	gd->bd->bi_boot_params = PHYS_SDRAM_1 + 0x100;

#ifdef CONFIG_USER_LED
	setup_user_leds();
#endif

	setup_nfc();

#ifdef CONFIG_I2C_MXC
	setup_i2c(CONFIG_SYS_I2C_PORT);
	/* Increase VDDGP voltage */
	setup_pmic_voltages();
#endif

#ifdef CONFIG_MXC_FEC
	setup_fec();
#endif

#if defined(CONFIG_DWC_AHSATA)
	setup_sata_device();
#endif

#ifdef CONFIG_USER_KEY
	setup_user_keys();
#endif

	return 0;
}


#ifdef CONFIG_ANDROID_RECOVERY
struct reco_envs supported_reco_envs[BOOT_DEV_NUM] = {
	{
	 .cmd = NULL,
	 .args = NULL,
	 },
	{
	 .cmd = NULL,
	 .args = NULL,
	 },
	{
	 .cmd = NULL,
	 .args = NULL,
	 },
	{
	 .cmd = NULL,
	 .args = NULL,
	 },
	{
	 .cmd = NULL,
	 .args = NULL,
	 },
	{
	 .cmd = NULL,
	 .args = NULL,
	 },
	{
	 .cmd = CONFIG_ANDROID_RECOVERY_BOOTCMD_MMC,
	 .args = CONFIG_ANDROID_RECOVERY_BOOTARGS_MMC,
	 },
	{
	 .cmd = CONFIG_ANDROID_RECOVERY_BOOTCMD_MMC,
	 .args = CONFIG_ANDROID_RECOVERY_BOOTARGS_MMC,
	 },
	{
	 .cmd = NULL,
	 .args = NULL,
	 },
};

int check_recovery_cmd_file(void)
{
	disk_partition_t info;
	ulong part_length;
	int filelen;
	char *env;

	/* For test only */
	/* When detecting android_recovery_switch,
	 * enter recovery mode directly */
	env = getenv("android_recovery_switch");
	if (!strcmp(env, "1")) {
		printf("Env recovery detected!\nEnter recovery mode!\n");
		return 1;
	}

	printf("Checking for recovery command file...\n");
	switch (get_boot_device()) {
	case MMC_BOOT:
	case SD_BOOT:
		{
			block_dev_desc_t *dev_desc = NULL;
			struct mmc *mmc = find_mmc_device(0);

			dev_desc = get_dev("mmc", 0);

			if (NULL == dev_desc) {
				puts("** Block device MMC 0 not supported\n");
				return 0;
			}

			mmc_init(mmc);

			if (get_partition_info(dev_desc,
					CONFIG_ANDROID_CACHE_PARTITION_MMC,
					&info)) {
				printf("** Bad partition %d **\n",
					CONFIG_ANDROID_CACHE_PARTITION_MMC);
				return 0;
			}

			part_length = ext2fs_set_blk_dev(dev_desc,
					CONFIG_ANDROID_CACHE_PARTITION_MMC);
			if (part_length == 0) {
				printf("** Bad partition - mmc 0:%d **\n",
					CONFIG_ANDROID_CACHE_PARTITION_MMC);
				ext2fs_close();
				return 0;
			}

			if (!ext2fs_mount(part_length)) {
				printf("** Bad ext2 partition or "
					"disk - mmc 0:%d **\n",
					CONFIG_ANDROID_CACHE_PARTITION_MMC);
				ext2fs_close();
				return 0;
			}

			filelen = ext2fs_open(CONFIG_ANDROID_RECOVERY_CMD_FILE);

			ext2fs_close();
		}
		break;
	case NAND_BOOT:
		return 0;
		break;
	case SPI_NOR_BOOT:
		return 0;
		break;
	case UNKNOWN_BOOT:
	default:
		return 0;
		break;
	}

	return (filelen > 0) ? 1 : 0;

}
#endif

#ifdef CONFIG_PRINT_SYS_INFO
extern void mxc_dump_clocks(void);
extern u32 __get_mcu_main_clk(void);

void print_sys_info(void)
{
	char *s;

	if ((s = getenv("bv")) != NULL)
		boot_verb = simple_strtoul(s, NULL, 10);

	if (boot_verb) {

		printf("CPU           : Freescale i.MX53 family %d.%dV at %d MHz\n",
				(get_board_rev() & 0xFF) >> 4,
				(get_board_rev() & 0xF),
				__get_mcu_main_clk() / 1000000);

		/* Read and print the CHIP_ID and CONFIG_ID registers */
		printf("PMIC ID       : %02x [Conf: %02x]\n",
			(uchar)pmic_rd_reg(129),
			(uchar)pmic_rd_reg(134));

		if (is_valid_hw_id(mod_hwid.variant)) {
			puts("Module HW ID  : ");
			printf("%s (%02x)\n", ccimx53_id[mod_hwid.variant].id_string, mod_hwid.variant);
			printf("Module HW Rev : %02x\n", mod_hwid.version);
			printf("Module SN     : %c%d\n", mod_hwid.mloc, mod_hwid.sn);
		}

		mxc_dump_clocks();

		puts("Last reset was: ");
		switch (__REG(SRC_BASE_ADDR + 0x8)) {
		case 0x0001:	puts("POR\n");			break;
		case 0x0009:	puts("RST\n");			break;
		case 0x0010:
		case 0x0011:	puts("WDOG\n");			break;
		default:	puts("unknown\n");		break;
		}
		puts("Boot Device: ");
		switch (get_boot_device()) {
		case WEIM_NOR_BOOT:	puts("NOR\n");		break;
		case ONE_NAND_BOOT:	puts("ONE NAND\n");	break;
		case PATA_BOOT:		puts("PATA\n");		break;
		case SATA_BOOT:		puts("SATA\n");		break;
		case I2C_BOOT:		puts("I2C\n");		break;
		case SPI_NOR_BOOT:	puts("SPI NOR\n");	break;
		case SD_BOOT:		puts("SD\n");		break;
		case MMC_BOOT:		puts("MMC\n");		break;
		case NAND_BOOT:		puts("NAND\n");		break;
		case UNKNOWN_BOOT:
		default:		puts("UNKNOWN\n");	break;
		}
	}
}
#endif

#ifdef BOARD_LATE_INIT
int board_late_init(void)
{
	char *s;
	unsigned int ram_clk;
	nv_critical_t *pNvram;

	setup_cpu_freq();

	if ((s = getenv("ram_clk")) != NULL) {
		ram_clk = simple_strtoul(s, NULL, 10);
		if (ram_clk < 400) {
			printf("Reconfiguring the RAM DDR clock to %d MHz\n", ram_clk);
			clk_config(CONFIG_REF_CLK_FREQ, ram_clk, DDR_CLK);
		}
	}

	if (NvCriticalGet(&pNvram)) {
		/* Pass location of environment in memory to OS through bi */
		gd->bd->nvram_addr = (unsigned long)pNvram;
	}

	return 0;
}
#endif

#ifdef BOARD_BEFORE_MLOOP_INIT
int board_before_mloop_init (void)
{
#ifdef CONFIG_PRINT_SYS_INFO
	print_sys_info();
#endif
#ifdef CONFIG_USER_KEY
	run_user_keys();
#endif
	return 0;
}
#endif

int checkboard(void)
{
	printf("Board: CCWMX51 JSK Development Board\n");
	return 0;
}

#ifdef CONFIG_CMD_PMIC
int do_pmic_r(cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
	ulong reg, count = 1, i;

	if ((argc < 2) || (argc > 3))
		goto usage;

	if (argc == 3)
		count = simple_strtoul(argv[2], NULL, 10);

	reg = simple_strtoul(argv[1], NULL, 10);

	if ((reg + count - 1) > 255) {
		printf ("*** ERROR: invalid parameters, last addr should be less than 255\n");
		return 1;
	}

	for (i = reg; i < (reg + count); i++) {
		if (!((i - reg) % 8))
			printf("%s%03d:", (i - reg) ? "\n" : "", (int)i);
		printf(" %02x", (uchar)pmic_rd_reg(i));
	}
	printf("\n");
	return 0;

usage:
	printf ("Usage:\n%s\n", cmdtp->usage);
	return 1;
}

int do_pmic_w(cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
	ulong reg, val;

	if (argc != 3)
		goto usage;

	reg = simple_strtoul(argv[1], NULL, 10);
	val = simple_strtoul(argv[2], NULL, 16);

	if (reg > 255) {
		printf ("*** ERROR: invalid parameters, reg addr should be less than 255\n");
		return 1;
	}

	(void)pmic_wr_msk_reg(reg, (uchar)val, 0xff);
	return 0;

usage:
	printf ("Usage:\n%s\n", cmdtp->usage);
	return 1;
}

int do_pmic(cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
	if (strlen(argv[0]) != 6 || argv[0][4] != '.' ||
	   (argv[0][5] != 'r' && argv[0][5] != 'w'))
		goto error;
	if (argv[0][5] == 'r')
		return do_pmic_r(cmdtp, flag, argc, argv);
	if (argv[0][5] == 'w')
		return do_pmic_w(cmdtp, flag, argc, argv);
error:
	printf ("Usage:\n%s\n", cmdtp->usage);
	return 1;
}

U_BOOT_CMD(
	pmic,	3,	1,	do_pmic,
	"pmic register access",
	".r address count    - Read count registers starting from address (values in dec)\n"
	"     .w address value    - Write register at address (in dec) with value (in hex)\n"
);
#endif /* CONFIG_CMD_PMIC */

#ifdef CONFIG_DUAL_BOOT_WDT_ENABLE
/* Enables the watchdog timer with a given timeout value (in seconds)
 * Returns the real programmed value
 */
unsigned long board_wdt_enable(unsigned long seconds)
{
	void *wdt_base_reg = (void *)WDOG1_BASE_ADDR;

	/* Configure wdt */
	mxc_wdt_config(wdt_base_reg);
	/* Set timeout */
	mxc_wdt_adjust_timeout(seconds);
	mxc_wdt_set_timeout(wdt_base_reg);
	/* Read programmed timeout */
	seconds = (unsigned long)mxc_wdt_get_timeout(wdt_base_reg);
	/* Enable wdt */
	mxc_wdt_enable(wdt_base_reg);

	return seconds;
}
#endif /* CONFIG_DUAL_BOOT_WDT_ENABLE */

int board_ehci_hcd_init(int port)
{
	/* Reset USB HUB */
	mxc_request_iomux(CONFIG_USB_HUB_RESET, IOMUX_CONFIG_GPIO);
	imx_gpio_set_pin(CONFIG_USB_HUB_RESET, 0);
	imx_gpio_pin_cfg_dir(CONFIG_USB_HUB_RESET, 1);
	udelay(100);
	imx_gpio_set_pin(CONFIG_USB_HUB_RESET, 1);

	return 0;
}

#define SIZE_128MB	0x8000000
#define SIZE_256MB	0x10000000

unsigned int get_platform_loadaddr(char * loadaddr)
{
	/* We distinguish three RAM memory variants:
	 * 128MB, 512MB (non-contiguous) and 1GB.
	 * U-Boot is loaded at 120MB offset.
	 */
	if (!strcmp(loadaddr, "loadaddr")) {
		/* loadaddr would be:
		 *  For 512MB: second bank base address (the reason is that
		 *             the two banks of RAM in this module variant are
		 *             not mapped contiguous)
		 *  For 1GB: 128MB (in this case the two banks are mapped
		 *           contiguous)
		 *  For 128MB: CONFIG_LOADADDR
		 */
#if CONFIG_NR_DRAM_BANKS > 1
		if (gd->bd->bi_dram[0].size == SIZE_256MB)
			/* 512MB variant: start of second bank */
			return gd->bd->bi_dram[1].start;
		else
			/* 1GB variant: 128MB */
			return (gd->bd->bi_dram[0].start + SIZE_128MB);
#else
		return CONFIG_LOADADDR;
#endif
	} else if (!strcmp(loadaddr, "wceloadaddr")) {
		/* WinCE images must be located in RAM and then uncompressed
		 * to their final location, so we need to set a load address
		 * half-way through the available RAM
		 */
#if CONFIG_NR_DRAM_BANKS > 1
		if (gd->bd->bi_dram[0].size == SIZE_256MB)
			/* 512MB variant: start of second bank */
			return gd->bd->bi_dram[1].start;
		else
			/* 1GB variant: 128MB */
			return (gd->bd->bi_dram[0].start + SIZE_128MB);
#else
		/* For RAM == 128MB: 64MB (half the memory) */
		return (gd->bd->bi_dram[0].start +
			(gd->bd->bi_dram[0].size / 2));
#endif
	} else
		return CONFIG_LOADADDR;
}

/*
 * This function returns the size of available RAM for doing a TFTP transfer.
 * This size depends on:
 *   - The total RAM available
 *   - The loadaddr
 *   - The RAM occupied by U-Boot and its location
 */
unsigned int get_tftp_available_ram(unsigned int loadaddr)
{
	/* We distinguish three RAM memory variants:
	 * 128MB, 512MB (non-contiguous) and 1GB.
	 * U-Boot is loaded at 120MB offset.
	 */
	unsigned int la_off;

#if CONFIG_NR_DRAM_BANKS > 1
		if (gd->bd->bi_dram[0].size == SIZE_256MB) {
			/* 512MB variant (non-contiguous memory) */
			if (loadaddr >= gd->bd->bi_dram[1].start) {
				/* size of bank 2 - loadaddr offset */
				la_off = loadaddr - gd->bd->bi_dram[1].start;
				return gd->bd->bi_dram[1].size - la_off;
			} else {
				la_off =  loadaddr - gd->bd->bi_dram[0].start;
				if (la_off >= SIZE_128MB)
					/*  Bank size - loadaddr offset */
					return (gd->bd->bi_dram[0].size -
						la_off);
				else
					/* 128M - UBOOT_RESERVED -
					 * loadaddr offset */
					return (SIZE_128MB -
						CONFIG_UBOOT_RESERVED_RAM -
						la_off);
			}
		} else {
			/* 1GB variant (contiguous memory) */
			la_off = loadaddr - gd->bd->bi_dram[0].start;
			if (loadaddr >= SIZE_128MB)
				/*  total RAM - loadaddr offset */
				return (get_total_ram() - la_off);
			else
				/* 128M - UBOOT_RESERVED - loadaddr offset */
				return (SIZE_128MB - CONFIG_UBOOT_RESERVED_RAM -
					la_off);
		}
#else
		/* 128MB variant */
		la_off = loadaddr - gd->bd->bi_dram[0].start;
		/* 128M - UBOOT_RESERVED - loadaddr offset */
		return (SIZE_128MB - CONFIG_UBOOT_RESERVED_RAM - la_off);
#endif
}
