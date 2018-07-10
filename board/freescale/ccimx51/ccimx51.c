/*
 * (C) Copyright 2009-2010 Digi International, Inc.
 *     Derived from imx51.c
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
#include <command.h>
#include <nvram.h>
#include <i2c.h>
#include <cmd_bsp.h>

#include <asm/io.h>
#include <asm/arch/mx51.h>
#include <asm/arch/mx51_pins.h>
#include <asm/arch/iomux.h>
#include <asm/errno.h>
#if defined(CONFIG_VIDEO_MX5)
#include <linux/list.h>
#include <ipu.h>
#include <lcd.h>
#include <linux/fb.h>
#include <linux/mxcfb.h>
#endif
#include <imx_spi.h>
#include <asm/arch/imx_spi_pmic.h>
#include <asm/arch/gpio.h>
#include <asm/arch/mxc_nand.h>
#include <netdev.h>
#include <serial.h>
#include <linux/ctype.h>

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

#include <asm/imx_iim.h>
#include <asm/cache-cp15.h>
#include "board-ccimx51.h"

DECLARE_GLOBAL_DATA_PTR;

static u32 system_rev;
static enum boot_device boot_dev;
struct spi_slave *pmicslv = NULL;
unsigned long boot_verb = 0;
struct ccimx51_hwid mod_hwid;
u32 swap_bbi_limit = 0;
extern size_t uboot_part_size;
#ifdef CONFIG_BOARD_CLEANUP_BEFORE_LINUX
extern int mxc_serial_deinit(void);
extern nv_os_type_e eOSType;
#endif

extern void setup_pll1_freq_800mhz(void);
extern unsigned int get_total_ram(void);

#define PMIC_SET_ALL_GPO_LOW_MSK	((1<<21) | (3 << 12) | (3 << 10) | (3 << 8))

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

u32 get_board_rev(void)
{
	return system_rev;
}

static int is_valid_hw_id(u8 variant)
{
	int ret = 0;

	if (variant < ARRAY_SIZE(ccimx51_id)) {
		if (ccimx51_id[variant].mem[0] != 0)
			ret = 1;
	}
	return ret;
}

int variant_has_ethernet(void)
{
	if ((mod_hwid.variant == 0x0d) || (mod_hwid.variant == 0x19))
		return 0;

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
	case 0x0f:
	case 0x11:
	case 0x13:
	case 0x15:
	case 0x17:
	case 0x19:
	case 0x1c:
		/* Non-wireless variants */
		return MACH_TYPE_CCMX51JS;
	default:
		break;
	}
	/* Wireless variants (or unspecified) */
	return MACH_TYPE_CCWMX51JS;
}

static unsigned int get_ram_from_hwid(u8 variant)
{
	return ccimx51_id[variant].mem[0];
}

static unsigned int get_cpu_freq_from_hwid(u8 variant)
{
	return ccimx51_id[variant].cpu_freq;
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

	memset(&mod_hwid, 0, sizeof(struct ccimx51_hwid));

	/* nvram settings override fuse hw id */
	if (NvCriticalGet(&pNVRAM)) {
		const nv_param_module_id_t *pModule = &pNVRAM->s.p.xID;

		if (!array_to_hwid((u8 *)pModule->szProductType))
			return 0;
	}

	/* Use fuses if no valid hw id was set in the nvram */
	if (iim_read_hw_id(hwid))
		array_to_hwid(hwid);

	/* returnning something != 0 will halt hte boot process */
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
		if (cpu_freq == 600000000)
			return;
	}

	setup_pll1_freq_800mhz();
}

#ifdef CONFIG_MXC_NAND_SWAP_BI
#define SKIP_SWAP_BI_MAX_PAGE_2K		(PART_UBOOT_SIZE / 0x800)
#define SKIP_SWAP_BI_MAX_PAGE_4K		(PART_UBOOT_SIZE_4K / 0x1000)

inline int skip_swap_bi(int page)
{
	/**
	 * Seems that the boot code in the i.mx515 rom is not able to
	 * boot from flash when the data has been written swapping the
	 * bad block byte. Avoid doing that when programming U-Boot into
	 * the flash
	 */
	if (page < swap_bbi_limit)
		return 1;
	return 0;
}
#else
inline int skip_swap_bi(int page_addr)
{
	return 1;
}
#endif

/* The ConnectCore Wi-i.MX51 only supports booting from NAND or MMC */
static inline void setup_boot_device(void)
{
	uint *fis_addr = (uint *)IRAM_BASE_ADDR;

	switch (*fis_addr) {
	case NAND_FLASH_BOOT:
		boot_dev = NAND_BOOT;
		break;
	case SPI_NOR_FLASH_BOOT:
		boot_dev = SPI_NOR_BOOT;
		break;
	case MMC_FLASH_BOOT:
		boot_dev = MMC_BOOT;
		break;
	default:
		{
			uint soc_sbmr = readl(SRC_BASE_ADDR + 0x4);
			uint bt_mem_ctl = soc_sbmr & 0x00000003;
			uint bt_mem_type = (soc_sbmr & 0x00000180) >> 7;

			switch (bt_mem_ctl) {
			case 0x3:
				if (bt_mem_type == 0)
					boot_dev = MMC_BOOT;
				else if (bt_mem_type == 3)
					boot_dev = SPI_NOR_BOOT;
				else
					boot_dev = UNKNOWN_BOOT;
				break;
			case 0x1:
				boot_dev = NAND_BOOT;
				break;
			default:
				boot_dev = UNKNOWN_BOOT;
			}
		}
		break;
	}
}

enum boot_device get_boot_device(void)
{
	return boot_dev;
}

static inline void setup_soc_rev(void)
{
	int reg;
#ifdef CONFIG_ARCH_MMU
	reg = __REG(0x20000000 + ROM_SI_REV); /* Virtual address */
#else
	reg = __REG(ROM_SI_REV);
#endif
	switch (reg) {
	case 0x10:	system_rev = 0x51000 | CHIP_REV_2_5;	break;
	case 0x20:	system_rev = 0x51000 | CHIP_REV_3_0;	break;
	default:	system_rev = 0x51000 | CHIP_REV_2_5;	break;
	}
}

static inline void set_board_rev(int rev)
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
	case 800000000:
		lpj = CONFIG_PRESET_LPJ_800MHZ;
		break;
	case 600000000:
		lpj = CONFIG_PRESET_LPJ_600MHZ;
		break;
	}
	return lpj;
}
#endif

#ifdef CONFIG_BOARD_CLEANUP_BEFORE_LINUX
void board_cleanup_before_linux(void)
{
	unsigned int val;

	/* Set all module external peripherals that we have enabled again in reset */
	val = pmic_reg(pmicslv, 34, 0, 0);
	val &= ~PMIC_SET_ALL_GPO_LOW_MSK;
#ifdef CONFIG_CCIMX51
	/**
	 * FIXME: enable the wireless when booting linux, due to a remaining problem
	 * setting up the wireless interface, if its not powered up early.
	 * Not sure why this happens. Powering the wireless module early in the OS
	 * doenst seem to solve the problem. Must be investigated in future.
	 */
	if (eOSType == NVOS_LINUX)
		val |= (1 << 12);
#endif
	pmic_reg(pmicslv, 34, val, 1);

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
       X_ARM_MMU_SECTION(0x000, 0x200, 0x1,
                       ARM_UNCACHEABLE, ARM_UNBUFFERABLE,
                       ARM_ACCESS_PERM_RW_RW); /* ROM */
       X_ARM_MMU_SECTION(0x1FF, 0x1FF, 0x001,
                       ARM_UNCACHEABLE, ARM_UNBUFFERABLE,
                       ARM_ACCESS_PERM_RW_RW); /* IRAM */
       X_ARM_MMU_SECTION(0x300, 0x300, 0x100,
                       ARM_UNCACHEABLE, ARM_UNBUFFERABLE,
                       ARM_ACCESS_PERM_RW_RW); /* GPU */
       X_ARM_MMU_SECTION(0x400, 0x400, 0x200,
                       ARM_UNCACHEABLE, ARM_UNBUFFERABLE,
                       ARM_ACCESS_PERM_RW_RW); /* IPUv3D */
       X_ARM_MMU_SECTION(0x600, 0x600, 0x300,
                       ARM_UNCACHEABLE, ARM_UNBUFFERABLE,
                       ARM_ACCESS_PERM_RW_RW); /* periperals */
#ifdef CONFIG_CCIMX5_SDRAM_128MB
       X_ARM_MMU_SECTION(0x900, 0x000, 0x020,
                       ARM_UNCACHEABLE, ARM_UNBUFFERABLE,
                       ARM_ACCESS_PERM_RW_RW); /* SDRAM uncached */
       X_ARM_MMU_SECTION(0x900, 0x900, 0x100,
                       ARM_CACHEABLE, ARM_BUFFERABLE,
                       ARM_ACCESS_PERM_RW_RW); /* SDRAM cached */
       X_ARM_MMU_SECTION(0x900, 0xE00, 0x100,
                       ARM_UNCACHEABLE, ARM_UNBUFFERABLE,
                       ARM_ACCESS_PERM_RW_RW); /* SDRAM uncached:256M */
#else
       X_ARM_MMU_SECTION(0x900, 0x000, 0x020,
                       ARM_UNCACHEABLE, ARM_UNBUFFERABLE,
                       ARM_ACCESS_PERM_RW_RW); /* SDRAM uncached */
       X_ARM_MMU_SECTION(0x900, 0x900, 0x200,
                       ARM_CACHEABLE, ARM_BUFFERABLE,
                       ARM_ACCESS_PERM_RW_RW); /* SDRAM cached */
       X_ARM_MMU_SECTION(0x900, 0xE00, 0x200,
                       ARM_UNCACHEABLE, ARM_UNBUFFERABLE,
                       ARM_ACCESS_PERM_RW_RW); /* SDRAM uncached:256M */
#endif
       X_ARM_MMU_SECTION(0xB80, 0xB80, 0x10,
                       ARM_UNCACHEABLE, ARM_UNBUFFERABLE,
                       ARM_ACCESS_PERM_RW_RW); /* CS1 EIM control*/
       X_ARM_MMU_SECTION(0xCC0, 0xCC0, 0x040,
                       ARM_UNCACHEABLE, ARM_UNBUFFERABLE,
                       ARM_ACCESS_PERM_RW_RW); /* CS4/5/NAND Flash buffer */

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

	return 0;
}

#define SERIAL_PORT_PAD		(PAD_CTL_HYS_ENABLE | PAD_CTL_PKE_ENABLE | \
				 PAD_CTL_PUE_PULL | PAD_CTL_DRV_HIGH | \
				 PAD_CTL_SRE_FAST)

void mxc_serial_gpios_init(int port)
{
	switch (port) {
	case 0:
		mxc_request_iomux(MX51_PIN_UART1_RXD, IOMUX_CONFIG_ALT0);
		mxc_request_iomux(MX51_PIN_UART1_TXD, IOMUX_CONFIG_ALT0);
		mxc_iomux_set_pad(MX51_PIN_UART1_RXD, SERIAL_PORT_PAD);
		mxc_iomux_set_pad(MX51_PIN_UART1_TXD, SERIAL_PORT_PAD);
		mxc_iomux_set_input(MUX_IN_UART1_IPP_UART_RXD_MUX_SELECT_INPUT, 0x0);
		break;

	case 1:
		mxc_request_iomux(MX51_PIN_UART2_RXD, IOMUX_CONFIG_ALT0);
		mxc_request_iomux(MX51_PIN_UART2_TXD, IOMUX_CONFIG_ALT0);
		mxc_iomux_set_pad(MX51_PIN_UART2_RXD, SERIAL_PORT_PAD);
		mxc_iomux_set_pad(MX51_PIN_UART2_TXD, SERIAL_PORT_PAD);
		mxc_iomux_set_input(MUX_IN_UART2_IPP_UART_RXD_MUX_SELECT_INPUT, 0x2);
		break;

	case 2:
		mxc_request_iomux(MX51_PIN_UART3_RXD, IOMUX_CONFIG_ALT1);
		mxc_request_iomux(MX51_PIN_UART3_TXD, IOMUX_CONFIG_ALT1);
		mxc_iomux_set_pad(MX51_PIN_UART3_RXD, SERIAL_PORT_PAD);
		mxc_iomux_set_pad(MX51_PIN_UART3_TXD, SERIAL_PORT_PAD);
		mxc_iomux_set_input(MUX_IN_UART3_IPP_UART_RXD_MUX_SELECT_INPUT, 0x4);
		break;
	}
}

void setup_nfc(void)
{
	int i;

	/*
	 * The nand pins should be already configured if we are booting
	 * from the nand, but may be the bootstrap is different.
	 * Therefore, we reconfig the pins here and the nand controller.
	 * TODO: we should check if we are booting from flash or SD card
	 * and, based on that, reconfigure the nand controller.
	 */
	mxc_request_iomux(MX51_PIN_NANDF_D0, IOMUX_CONFIG_ALT0);
	mxc_request_iomux(MX51_PIN_NANDF_D1, IOMUX_CONFIG_ALT0);
	mxc_request_iomux(MX51_PIN_NANDF_D2, IOMUX_CONFIG_ALT0);
	mxc_request_iomux(MX51_PIN_NANDF_D3, IOMUX_CONFIG_ALT0);
	mxc_request_iomux(MX51_PIN_NANDF_D4, IOMUX_CONFIG_ALT0);
	mxc_request_iomux(MX51_PIN_NANDF_D5, IOMUX_CONFIG_ALT0);
	mxc_request_iomux(MX51_PIN_NANDF_D6, IOMUX_CONFIG_ALT0);
	mxc_request_iomux(MX51_PIN_NANDF_D7, IOMUX_CONFIG_ALT0);

	mxc_request_iomux(MX51_PIN_NANDF_CS0, IOMUX_CONFIG_ALT0);
	mxc_request_iomux(MX51_PIN_NANDF_RE_B, IOMUX_CONFIG_ALT0);
	mxc_request_iomux(MX51_PIN_NANDF_WE_B, IOMUX_CONFIG_ALT0);
	mxc_request_iomux(MX51_PIN_NANDF_WP_B, IOMUX_CONFIG_ALT0);
	mxc_request_iomux(MX51_PIN_NANDF_ALE, IOMUX_CONFIG_ALT0);
	mxc_request_iomux(MX51_PIN_NANDF_CLE, IOMUX_CONFIG_ALT0);
	mxc_request_iomux(MX51_PIN_NANDF_RB0, IOMUX_CONFIG_ALT0);

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

s32 spi_get_cfg(struct imx_spi_dev_t *dev)
{
	switch (dev->slave.cs) {
	case 0:
		/* pmic */
		dev->base = CSPI1_BASE_ADDR;
		dev->freq = CONFIG_IMX_SPI_PMIC_SPEED;
		dev->ss_pol = IMX_SPI_ACTIVE_HIGH;
		dev->ss = 0;
		dev->fifo_sz = 64 * 4;
		dev->us_delay = 0;
		break;
	default:
		printf("Invalid Bus ID! \n");
		break;
	}

	return 0;
}

#define CSPI1_MOSI_PAD_CFG	(PAD_CTL_SRE_FAST | PAD_CTL_DRV_HIGH | \
				PAD_CTL_HYS_ENABLE)
#define CSPI1_MISO_PAD_CFG	(PAD_CTL_SRE_FAST | PAD_CTL_DRV_HIGH | \
				PAD_CTL_HYS_ENABLE)
#define CSPI1_SSX_PAD_CFG_UP	(PAD_CTL_SRE_FAST | PAD_CTL_DRV_HIGH | \
				PAD_CTL_PKE_ENABLE | PAD_CTL_HYS_ENABLE)
#define CSPI1_RDY_PAD_CFG	(PAD_CTL_PUE_PULL | PAD_CTL_22K_PU | \
				PAD_CTL_PKE_ENABLE | PAD_CTL_DRV_HIGH)
#define CSPI1_SCLK_PAD_CFG	(PAD_CTL_SRE_FAST | PAD_CTL_DRV_HIGH | \
				PAD_CTL_HYS_ENABLE)
void spi_io_init(struct imx_spi_dev_t *dev)
{
	/* 000: Select mux mode: ALT0 mux port: MOSI of instance: ecspi1 */
	mxc_request_iomux(MX51_PIN_CSPI1_MOSI, IOMUX_CONFIG_ALT0);
	mxc_iomux_set_pad(MX51_PIN_CSPI1_MOSI, CSPI1_MOSI_PAD_CFG);

	/* 000: Select mux mode: ALT0 mux port: MISO of instance: ecspi1. */
	mxc_request_iomux(MX51_PIN_CSPI1_MISO, IOMUX_CONFIG_ALT0);
	mxc_iomux_set_pad(MX51_PIN_CSPI1_MISO, CSPI1_MISO_PAD_CFG);

	/* 000: Select mux mode: ALT0 mux port: SCLK of instance: ecspi1. */
	mxc_request_iomux(MX51_PIN_CSPI1_SCLK, IOMUX_CONFIG_ALT0);
	mxc_iomux_set_pad(MX51_PIN_CSPI1_SCLK, CSPI1_SCLK_PAD_CFG);

	/* 000: Select mux mode: ALT0 mux port: SS0 of instance: ecspi1. */
	mxc_request_iomux(MX51_PIN_CSPI1_SS0, IOMUX_CONFIG_ALT0);
	mxc_iomux_set_pad(MX51_PIN_CSPI1_SS0, CSPI1_SSX_PAD_CFG_UP);

	/* Setup MX51_PIN_CSPI1_RDY to be high and disable the external touch */
	mxc_iomux_set_pad(MX51_PIN_CSPI1_RDY, CSPI1_RDY_PAD_CFG);
}

#ifdef CONFIG_USER_KEY
#define USER_KEY_PAD_CFG	(PAD_CTL_SRE_SLOW | PAD_CTL_DRV_MEDIUM | \
				 PAD_CTL_ODE_OPENDRAIN_NONE | PAD_CTL_HYS_ENABLE | \
				 PAD_CTL_PUE_KEEPER | PAD_CTL_PKE_NONE | PAD_CTL_DRV_VOT_LOW)
static void setup_user_keys(void)
{
	/*
	 * Configure iomux for gpio operation and the corresponding
	 * gpios as inputs
	 */
	mxc_request_iomux(USER_KEY1_GPIO, IOMUX_CONFIG_GPIO);
	mxc_iomux_set_pad(USER_KEY1_GPIO, USER_KEY_PAD_CFG);
#ifdef BOARD_USER_KEY1_GPIO_SEL_INP
	mxc_iomux_set_input(BOARD_USER_KEY1_GPIO_SEL_INP, BOARD_USER_KEY1_GPIO_SINP_PATH);
#endif
	mxc_request_iomux(USER_KEY2_GPIO, IOMUX_CONFIG_GPIO);
	mxc_iomux_set_pad(USER_KEY2_GPIO, USER_KEY_PAD_CFG);
#ifdef BOARD_USER_KEY2_GPIO_SEL_INP
	mxc_iomux_set_input(BOARD_USER_KEY2_GPIO_SEL_INP, BOARD_USER_KEY2_GPIO_SINP_PATH);
#endif
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

#if defined(CONFIG_SILENT_CONSOLE)
#ifdef ENABLE_CONSOLE_GPIO
#define ENCON_GPIO_PAD_CFG	(PAD_CTL_SRE_SLOW | PAD_CTL_DRV_MEDIUM | \
				 PAD_CTL_ODE_OPENDRAIN_NONE | PAD_CTL_HYS_ENABLE | \
				 PAD_CTL_PUE_KEEPER | PAD_CTL_PKE_NONE | PAD_CTL_DRV_VOT_LOW)

static void init_console_gpio(void)
{
	mxc_request_iomux(ENABLE_CONSOLE_GPIO, ENCON_GPIO_MUX_MODE);
	mxc_iomux_set_pad(ENABLE_CONSOLE_GPIO, ENCON_GPIO_PAD_CFG);

	/* Confgiure direction as input */
	imx_gpio_pin_cfg_dir(ENABLE_CONSOLE_GPIO, 0);
}

int gpio_enable_console(void)
{
	return (imx_gpio_get_pin(ENABLE_CONSOLE_GPIO) == CONSOLE_ENABLE_GPIO_STATE);
}
#else
int gpio_enable_console(void) { return 0; }
#endif /* ENABLE_CONSOLE_GPIO */
#endif /* CONFIG_SILENT_CONSOLE */


#ifdef CONFIG_I2C_MXC
#define I2C_PAD_CFG	(PAD_CTL_SRE_FAST | PAD_CTL_DRV_HIGH | \
	PAD_CTL_ODE_OPENDRAIN_ENABLE | PAD_CTL_100K_PU | PAD_CTL_HYS_ENABLE)
static void setup_i2c(unsigned int module_base)
{
	switch (module_base) {
	case I2C1_BASE_ADDR:
		/* SCL */
		mxc_request_iomux(MX51_PIN_SD2_CMD, IOMUX_CONFIG_ALT1 | IOMUX_CONFIG_SION);
		mxc_iomux_set_pad(MX51_PIN_SD2_CMD, I2C_PAD_CFG);
		mxc_iomux_set_input(MUX_IN_I2C1_IPP_SCL_IN_SELECT_INPUT, INPUT_CTL_PATH2);
		/* SDA */
		mxc_request_iomux(MX51_PIN_SD2_CLK, IOMUX_CONFIG_ALT1 | IOMUX_CONFIG_SION);
		mxc_iomux_set_pad(MX51_PIN_SD2_CLK, I2C_PAD_CFG);
		mxc_iomux_set_input(MUX_IN_I2C1_IPP_SDA_IN_SELECT_INPUT, INPUT_CTL_PATH2);
		break;

	case I2C2_BASE_ADDR:
		/* SCL */
		mxc_request_iomux(MX51_PIN_GPIO1_2, IOMUX_CONFIG_ALT2 | IOMUX_CONFIG_SION);
		mxc_iomux_set_pad(MX51_PIN_GPIO1_2, I2C_PAD_CFG);
		mxc_iomux_set_input(MUX_IN_I2C2_IPP_SCL_IN_SELECT_INPUT, INPUT_CTL_PATH3);
		/* SDA */
		mxc_request_iomux(MX51_PIN_GPIO1_3, IOMUX_CONFIG_ALT2 | IOMUX_CONFIG_SION);
		mxc_iomux_set_pad(MX51_PIN_GPIO1_3, I2C_PAD_CFG);
		mxc_iomux_set_input(MUX_IN_I2C2_IPP_SDA_IN_SELECT_INPUT, INPUT_CTL_PATH3);
		break;

	default:
		printf("Invalid I2C base: 0x%x\n", module_base);
		break;
	}
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

#define CCIMX51_FEC_PAD		(PAD_CTL_DRV_MEDIUM | PAD_CTL_HYS_ENABLE | PAD_CTL_DRV_VOT_HIGH)
static void setup_fec(void)
{

	/* The CCIMX51 module maps the FEC pins to DISP2/DI2 interface */
	/* FEC COL muxed as ALT2 of DISP_DATA10 */
	mxc_request_iomux(MX51_PIN_DISP2_DAT10, IOMUX_CONFIG_ALT2);
	mxc_iomux_set_pad(MX51_PIN_DISP2_DAT10, CCIMX51_FEC_PAD);

	/* FEC CRS muxed as ALT2 of DI2_PIN4 */
	mxc_request_iomux(MX51_PIN_DI2_PIN4, IOMUX_CONFIG_ALT2);
	mxc_iomux_set_pad(MX51_PIN_DI2_PIN4, CCIMX51_FEC_PAD);

	/* FEC MDC muxed as ALT2 of DI2_PIN2 */
	mxc_request_iomux(MX51_PIN_DI2_PIN2, IOMUX_CONFIG_ALT2);
	mxc_iomux_set_pad(MX51_PIN_DI2_PIN2, CCIMX51_FEC_PAD);

	/* FEC MDIO muxed as ALT2 of DI2_PIN3 */
	mxc_request_iomux(MX51_PIN_DI2_PIN3, IOMUX_CONFIG_ALT2);
	mxc_iomux_set_pad(MX51_PIN_DI2_PIN3, CCIMX51_FEC_PAD);

	/* FEC RDATA[0] muxed as ALT2 of DISP2_DAT14 */
	mxc_request_iomux(MX51_PIN_DISP2_DAT14, IOMUX_CONFIG_ALT2);
	mxc_iomux_set_pad(MX51_PIN_DISP2_DAT14, CCIMX51_FEC_PAD);

	/* FEC RDATA[1] muxed as ALT2 of DI2_DISP_CLK */
	mxc_request_iomux(MX51_PIN_DI2_DISP_CLK, IOMUX_CONFIG_ALT2);
	mxc_iomux_set_pad(MX51_PIN_DI2_DISP_CLK, CCIMX51_FEC_PAD);

	/* FEC RDATA[2] muxed as ALT2 of DI_GP4 */
	mxc_request_iomux(MX51_PIN_DI_GP4, IOMUX_CONFIG_ALT2);
	mxc_iomux_set_pad(MX51_PIN_DI_GP4, CCIMX51_FEC_PAD);

	/* FEC RDATA[3] muxed as ALT2 of DISP2_DAT0 */
	mxc_request_iomux(MX51_PIN_DISP2_DAT0, IOMUX_CONFIG_ALT2);
	mxc_iomux_set_pad(MX51_PIN_DISP2_DAT0, CCIMX51_FEC_PAD);

	/* FEC RX_CLK muxed as ALT2 of DISP2_DAT11 */
	mxc_request_iomux(MX51_PIN_DISP2_DAT11, IOMUX_CONFIG_ALT2);
	mxc_iomux_set_pad(MX51_PIN_DISP2_DAT11, CCIMX51_FEC_PAD);

	/* FEC RX_DV muxed as ALT2 of DISP2_DAT12 */
	mxc_request_iomux(MX51_PIN_DISP2_DAT12, IOMUX_CONFIG_ALT2);
	mxc_iomux_set_pad(MX51_PIN_DISP2_DAT12, CCIMX51_FEC_PAD);

	/* FEC RX_ER muxed as ALT2 of DISP2_DAT1 */
	mxc_request_iomux(MX51_PIN_DISP2_DAT1, IOMUX_CONFIG_ALT2);
	mxc_iomux_set_pad(MX51_PIN_DISP2_DAT1, CCIMX51_FEC_PAD);

	/* FEC TDATA[0] muxed as ALT2 of DISP2_DAT15 */
	mxc_request_iomux(MX51_PIN_DISP2_DAT15, IOMUX_CONFIG_ALT2);
	mxc_iomux_set_pad(MX51_PIN_DISP2_DAT15, CCIMX51_FEC_PAD);

	/* FEC TDATA[1] muxed as ALT2 of DISP2_DAT6 */
	mxc_request_iomux(MX51_PIN_DISP2_DAT6, IOMUX_CONFIG_ALT2);
	mxc_iomux_set_pad(MX51_PIN_DISP2_DAT6, CCIMX51_FEC_PAD);

	/* FEC TDATA[2] muxed as ALT2 of DISP2_DAT7 */
	mxc_request_iomux(MX51_PIN_DISP2_DAT7, IOMUX_CONFIG_ALT2);
	mxc_iomux_set_pad(MX51_PIN_DISP2_DAT7, CCIMX51_FEC_PAD);

	/* FEC TDATA[3] muxed as ALT2 of DISP2_DAT8 */
	mxc_request_iomux(MX51_PIN_DISP2_DAT8, IOMUX_CONFIG_ALT2);
	mxc_iomux_set_pad(MX51_PIN_DISP2_DAT8, CCIMX51_FEC_PAD);

	/* FEC TX_CLK muxed as ALT2 of DISP2_DAT13 */
	mxc_request_iomux(MX51_PIN_DISP2_DAT13, IOMUX_CONFIG_ALT2);
	mxc_iomux_set_pad(MX51_PIN_DISP2_DAT13, CCIMX51_FEC_PAD);

	/* FEC TX_EN muxed as ALT2 of DISP2_DAT9 */
	mxc_request_iomux(MX51_PIN_DISP2_DAT9, IOMUX_CONFIG_ALT2);
	mxc_iomux_set_pad(MX51_PIN_DISP2_DAT9, CCIMX51_FEC_PAD);

	/* FEC TX_ER muxed as ALT2 of DI_GP3 */
	mxc_request_iomux(MX51_PIN_DI_GP3, IOMUX_CONFIG_ALT2);
	mxc_iomux_set_pad(MX51_PIN_DI_GP3, CCIMX51_FEC_PAD);

	/* FEC signals involved in daisy chain should be mapped to DISP2/DI2 pads */
	mxc_iomux_set_input(MUX_IN_FEC_FEC_COL_SELECT_INPUT, INPUT_CTL_PATH1);
	mxc_iomux_set_input(MUX_IN_FEC_FEC_CRS_SELECT_INPUT, INPUT_CTL_PATH1);
	mxc_iomux_set_input(MUX_IN_FEC_FEC_MDI_SELECT_INPUT, INPUT_CTL_PATH1);
	mxc_iomux_set_input(MUX_IN_FEC_FEC_RDATA_0_SELECT_INPUT, INPUT_CTL_PATH1);
	mxc_iomux_set_input(MUX_IN_FEC_FEC_RDATA_1_SELECT_INPUT, INPUT_CTL_PATH1);
	mxc_iomux_set_input(MUX_IN_FEC_FEC_RDATA_2_SELECT_INPUT, INPUT_CTL_PATH1);
	mxc_iomux_set_input(MUX_IN_FEC_FEC_RDATA_3_SELECT_INPUT, INPUT_CTL_PATH1);
	mxc_iomux_set_input(MUX_IN_FEC_FEC_RX_CLK_SELECT_INPUT, INPUT_CTL_PATH1);
	mxc_iomux_set_input(MUX_IN_FEC_FEC_RX_DV_SELECT_INPUT, INPUT_CTL_PATH1);
	mxc_iomux_set_input(MUX_IN_FEC_FEC_RX_ER_SELECT_INPUT, INPUT_CTL_PATH1);
	mxc_iomux_set_input(MUX_IN_FEC_FEC_TX_CLK_SELECT_INPUT, INPUT_CTL_PATH1);
}
#endif

static void pmic_power_init(void)
{
	volatile u32 rev_id, pwrctrl;
	unsigned int val;
#define REV_ATLAS_LITE_1_0	   0x8
#define REV_ATLAS_LITE_1_1	   0x9
#define REV_ATLAS_LITE_2_0	   0x10
#define REV_ATLAS_LITE_2_1	   0x11

	/* Enable 3.3V by switching on the PWGT2SPIEN */
	val = pmic_reg(pmicslv, 34, 0, 0);
	val &= ~0x10000;
	pmic_reg(pmicslv, 34, val, 1);

	/* Allow charger to charge (4.2V and 560mA) */
	pmic_reg(pmicslv, 48, 0x00238033, 1);

	/* Depending on the version of the PMIC, we need
	 * to change the value of GLBRSTENB flag to prevent
	 * the target from resetting after 12 seconds.
	 */
	rev_id = pmic_reg(pmicslv, 7, 0, 0);

	pwrctrl = pmic_reg(pmicslv, 13, 0, 0);
	switch (rev_id & 0x1F) {
	case 0x14:	/* MC13892A, MC13892C have it enabled by default */
		pwrctrl |= (1 << 7); /* disable global reset */
		pmic_reg(pmicslv, 13, pwrctrl, 1);
		break;
	default:
		/* MC13892B, and MC13892D have global reset disabled by default */
		break;
	}

	if (is_soc_rev(CHIP_REV_2_0) >= 0) {
		/* Set core voltage to 1.1V */
		val = pmic_reg(pmicslv, 24, 0, 0);
		val = (val & (~0x1F)) | 0x14;
		pmic_reg(pmicslv, 24, val, 1);

		/* Setup VCC (SW2) to 1.25 */
		val = pmic_reg(pmicslv, 25, 0, 0);
		val = (val & (~0x1F)) | 0x1A;
		pmic_reg(pmicslv, 25, val, 1);

		/* Setup 1V2_DIG1 (SW3) to 1.25 */
		val = pmic_reg(pmicslv, 26, 0, 0);
		val = (val & (~0x1F)) | 0x1A;
		pmic_reg(pmicslv, 25, val, 1);

		udelay(50);
		/* Press the turbo... and increase freq to 800MHz */
		writel(0x0, CCM_BASE_ADDR + CLKCTL_CACRR);
	} else {
		/* TO 3.0 */
		/* Set core voltage to 1.1V */
		val = pmic_reg(pmicslv, 24, 0, 0);
		val = (val & (~0x1f)) | 0x14;
		pmic_reg(pmicslv, 24, val, 1);

		/* Setup VCC (SW2) to 1.225 */
		val = pmic_reg(pmicslv, 25, 0, 0);
		val = (val & (~0x1F)) | 0x19;
		pmic_reg(pmicslv, 25, val, 1);

		/* Setup 1V2_DIG1 (SW3) to 1.2 */
		val = pmic_reg(pmicslv, 26, 0, 0);
		val = (val & (~0x1F)) | 0x18;
		pmic_reg(pmicslv, 26, val, 1);
	}

	if (((pmic_reg(pmicslv, 7, 0, 0) & 0x1F) < REV_ATLAS_LITE_2_0) ||
		(((pmic_reg(pmicslv, 7, 0, 0) >> 9) & 0x3) == 0)) {
		/* Set switchers in PWM mode for Atlas 2.0 and lower */
		/* Setup the switcher mode for SW1 & SW2*/
		val = pmic_reg(pmicslv, 28, 0, 0);
		val = (val & (~0x3C0F)) | 0x401405;
		pmic_reg(pmicslv, 28, val, 1);

		/* Setup the switcher mode for SW3 & SW4 */
		val = pmic_reg(pmicslv, 29, 0, 0);
		val = (val & (~0xF0F)) | 0x505;
		pmic_reg(pmicslv, 29, val, 1);
	} else {
		/* Set switchers in Auto in NORMAL mode & STANDBY mode for Atlas 2.0a */
		/* Setup the switcher mode for SW1 & SW2*/
		val = pmic_reg(pmicslv, 28, 0, 0);
		val = (val & (~0x3C0F)) | 0x402008;
		pmic_reg(pmicslv, 28, val, 1);

		/* Setup the switcher mode for SW3 & SW4 */
		val = pmic_reg(pmicslv, 29, 0, 0);
		val = (val & (~0xF0F)) | 0x808;
		pmic_reg(pmicslv, 29, val, 1);
	}

	/* Set VVIDEO to 2.775V, VAUDIO to 3V, VSD to 3.15V */
	val = pmic_reg(pmicslv, 31, 0, 0);
	val &= ~0x1FC;
	val |= 0x1F4;
	pmic_reg(pmicslv, 31, val, 1);

	/* Configure VGEN3 and VCAM regulators to use external PNP */
	val = 0x208;
	pmic_reg(pmicslv, 33, val, 1);
	udelay(200);

	/* Enable VGEN3, VCAM, VAUDIO, VVIDEO, VSD regulators */
	val = 0x49249;
	pmic_reg(pmicslv, 33, val, 1);
	/* Enable USB OTG regulator */
	val = 0x409;
	pmic_reg(pmicslv, 50, val, 1);

	/* Reset the external devices by clearing GPO2, GPO3 and GPO4 */
	val = pmic_reg(pmicslv, 34, 0, 0);
	val &= ~PMIC_SET_ALL_GPO_LOW_MSK;
	pmic_reg(pmicslv, 34, val, 1);
	udelay(100);

#ifdef CONFIG_MXC_FEC
	val |= (1 << 10);
#endif
#ifdef CONFIG_SMC911X
	val |= (1 << 8);
#endif
	val = pmic_reg(pmicslv, 34, val, 1);
}

int ccimx51_power_init(void)
{
	/* Switch on voltage regulators and reset some peripherals */
	if (!spi_pmic_init(0)) {
		printf("***ERROR: unable to setup SPI for PMIC\n");
		return -1;
	}

	pmic_power_init();
	return 0;
}

#ifdef CONFIG_NET_MULTI
/* TODO, this shouldnt depend on net_multi. Maybe we only have this interface enabled? */
#if defined(CONFIG_SMC911X)
#include <net.h>
#include "../../../drivers/net/smc911x.h"
u32 mx51_io_base_addr;
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

	/* Configure the pin mux for CS line and address and data buses */
	mxc_request_iomux(MX51_PIN_EIM_CS5, IOMUX_CONFIG_ALT0);
	mxc_iomux_set_pad(MX51_PIN_EIM_CS5, PAD_CTL_HYS_ENABLE | PAD_CTL_PUE_KEEPER | PAD_CTL_DRV_MEDIUM);
	mxc_request_iomux(MX51_PIN_EIM_OE, IOMUX_CONFIG_ALT0);
	mxc_request_iomux(MX51_PIN_EIM_DA0, IOMUX_CONFIG_ALT0);
	mxc_request_iomux(MX51_PIN_EIM_DA1, IOMUX_CONFIG_ALT0);
	mxc_request_iomux(MX51_PIN_EIM_DA2, IOMUX_CONFIG_ALT0);
	mxc_request_iomux(MX51_PIN_EIM_DA3, IOMUX_CONFIG_ALT0);
	mxc_request_iomux(MX51_PIN_EIM_DA4, IOMUX_CONFIG_ALT0);
	mxc_request_iomux(MX51_PIN_EIM_DA5, IOMUX_CONFIG_ALT0);
	mxc_request_iomux(MX51_PIN_EIM_DA6, IOMUX_CONFIG_ALT0);
	mxc_request_iomux(MX51_PIN_EIM_DA7, IOMUX_CONFIG_ALT0);
	mxc_request_iomux(MX51_PIN_EIM_D16, IOMUX_CONFIG_ALT0);
	mxc_request_iomux(MX51_PIN_EIM_D17, IOMUX_CONFIG_ALT0);
	mxc_request_iomux(MX51_PIN_EIM_D18, IOMUX_CONFIG_ALT0);
	mxc_request_iomux(MX51_PIN_EIM_D19, IOMUX_CONFIG_ALT0);
	mxc_request_iomux(MX51_PIN_EIM_D20, IOMUX_CONFIG_ALT0);
	mxc_request_iomux(MX51_PIN_EIM_D21, IOMUX_CONFIG_ALT0);
	mxc_request_iomux(MX51_PIN_EIM_D22, IOMUX_CONFIG_ALT0);
	mxc_request_iomux(MX51_PIN_EIM_D23, IOMUX_CONFIG_ALT0);
	mxc_request_iomux(MX51_PIN_EIM_D24, IOMUX_CONFIG_ALT0);
	mxc_request_iomux(MX51_PIN_EIM_D25, IOMUX_CONFIG_ALT0);
	mxc_request_iomux(MX51_PIN_EIM_D26, IOMUX_CONFIG_ALT0);
	mxc_request_iomux(MX51_PIN_EIM_D27, IOMUX_CONFIG_ALT0);
	mxc_request_iomux(MX51_PIN_EIM_D28, IOMUX_CONFIG_ALT0);
	mxc_request_iomux(MX51_PIN_EIM_D29, IOMUX_CONFIG_ALT0);
	mxc_request_iomux(MX51_PIN_EIM_D30, IOMUX_CONFIG_ALT0);
	mxc_request_iomux(MX51_PIN_EIM_D31, IOMUX_CONFIG_ALT0);

	/* Configure the CS timming, bus width, etc */
	/* 16 bit on DATA[31..16], not multiplexed, async */
	writel(0x00420081, WEIM_BASE_ADDR + 0x78 + CSGCR1);
	/* ADH has not effect on non muxed bus */
	writel(0, WEIM_BASE_ADDR + 0x78 + CSGCR2);
	/* RWSC=50, RADVA=2, RADVN=6, OEA=0, OEN=0, RCSA=0, RCSN=0 */
	writel(0x32260000, WEIM_BASE_ADDR + 0x78+ CSRCR1);
	/* APR=0 */
	writel(0, WEIM_BASE_ADDR + 0x78 + CSRCR2);
	/* WAL=0, WBED=1, WWSC=50, WADVA=2, WADVN=6, WEA=0, WEN=0, WCSA=0 */
	writel(0x72080f00, WEIM_BASE_ADDR + 0x78 + CSWCR1);

	mx51_io_base_addr = CS5_BASE_ADDR;

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

struct fsl_esdhc_cfg esdhc_cfg[] = MMC_CFGS;

#ifdef CONFIG_DYNAMIC_MMC_DEVNO
int get_mmc_env_devno()
{
	uint soc_sbmr = readl(SRC_BASE_ADDR + 0x4);
	return (soc_sbmr & 0x00180000) ? 1 : 0;
}
#endif

#define SD_PAD_CFG	(PAD_CTL_DRV_VOT_HIGH |	\
			 PAD_CTL_47K_PU |	\
			 PAD_CTL_DRV_HIGH)
int esdhc_gpio_init(int mmc_index)
{
	s32 status = 0;

	switch (mmc_index) {
	case 0:
		/* SD1_CMD */
		mxc_request_iomux(MX51_PIN_SD1_CMD, IOMUX_CONFIG_ALT0 | IOMUX_CONFIG_SION);
		mxc_iomux_set_pad(MX51_PIN_SD1_CMD, SD_PAD_CFG);

		/* SD1_CLK */
		mxc_request_iomux(MX51_PIN_SD1_CLK, IOMUX_CONFIG_ALT0 | IOMUX_CONFIG_SION);
		mxc_iomux_set_pad(MX51_PIN_SD1_CLK, SD_PAD_CFG);

		/* SD1_DATA0 */
		mxc_request_iomux(MX51_PIN_SD1_DATA0, IOMUX_CONFIG_ALT0 | IOMUX_CONFIG_SION);
		mxc_iomux_set_pad(MX51_PIN_SD1_DATA0, SD_PAD_CFG);

		/* SD1_DATA1 */
		mxc_request_iomux(MX51_PIN_SD1_DATA1, IOMUX_CONFIG_ALT0 | IOMUX_CONFIG_SION);
		mxc_iomux_set_pad(MX51_PIN_SD1_DATA1, SD_PAD_CFG);

		/* SD1_DATA2 */
		mxc_request_iomux(MX51_PIN_SD1_DATA2, IOMUX_CONFIG_ALT0 | IOMUX_CONFIG_SION);
		mxc_iomux_set_pad(MX51_PIN_SD1_DATA2, SD_PAD_CFG);

		/* SD1_DATA3 */
		mxc_request_iomux(MX51_PIN_SD1_DATA3, IOMUX_CONFIG_ALT0 | IOMUX_CONFIG_SION);
		mxc_iomux_set_pad(MX51_PIN_SD1_DATA3, SD_PAD_CFG);
		break;
	case 2:
		/* SD3_CMD */
		mxc_request_iomux(MX51_PIN_NANDF_RDY_INT, IOMUX_CONFIG_ALT5 | IOMUX_CONFIG_SION);
		mxc_iomux_set_pad(MX51_PIN_NANDF_RDY_INT, SD_PAD_CFG);

		/* SD3_CLK */
		mxc_request_iomux(MX51_PIN_NANDF_CS7, IOMUX_CONFIG_ALT5);
		mxc_iomux_set_pad(MX51_PIN_NANDF_CS7, SD_PAD_CFG);

		/* SD3_DATA0 */
		mxc_request_iomux(MX51_PIN_NANDF_D8, IOMUX_CONFIG_ALT5);
		mxc_iomux_set_pad(MX51_PIN_NANDF_D8, SD_PAD_CFG);
		mxc_iomux_set_input(MUX_IN_ESDHC3_IPP_DAT0_IN_SELECT_INPUT, INPUT_CTL_PATH1);

		/* SD3_DATA1 */
		mxc_request_iomux(MX51_PIN_NANDF_D9, IOMUX_CONFIG_ALT5);
		mxc_iomux_set_pad(MX51_PIN_NANDF_D9, SD_PAD_CFG);
		mxc_iomux_set_input(MUX_IN_ESDHC3_IPP_DAT1_IN_SELECT_INPUT, INPUT_CTL_PATH1);

		/* SD3_DATA2 */
		mxc_request_iomux(MX51_PIN_NANDF_D10, IOMUX_CONFIG_ALT5);
		mxc_iomux_set_pad(MX51_PIN_NANDF_D10, SD_PAD_CFG);
		mxc_iomux_set_input(MUX_IN_ESDHC3_IPP_DAT2_IN_SELECT_INPUT, INPUT_CTL_PATH1);

		/* SD3_DATA3 */
		mxc_request_iomux(MX51_PIN_NANDF_D11, IOMUX_CONFIG_ALT5);
		mxc_iomux_set_pad(MX51_PIN_NANDF_D11, SD_PAD_CFG);
		mxc_iomux_set_input(MUX_IN_ESDHC3_IPP_DAT3_IN_SELECT_INPUT, INPUT_CTL_PATH1);
		break;
	case 1:
	case 3:
	default:
		printf("*** ERROR: SD/MMC interface %d not supported\n", mmc_index);
		status = 1;
		break;
	}

	return status;
}

int board_mmc_init(bd_t *bis)
{
	int mmc_devs[] = MMC_DEVICES;
	int i;
	unsigned int val;

	/* Apparently the iMX51 CPU does not correctly isolate the power
	 * signals of the different SD interfaces. It has been empirically
	 * confirmed a feedback voltage coming from NVCC_PER15 (uSD card) to
	 * NVCC_PER17 (WLAN) which causes uSD card detection issues when the
	 * WLAN is disabled by jumper J17 on the JSK. To avoid these problems
	 * the recommendation is to have J17 open (WLAN enabled) and enabling
	 * always the WLAN by setting WLAN_RESET_L/MC13892_GPO4 high in U-Boot
	 * so that 3V3_WLAN always has 3.3V.
	 * The issue does not happen in non-wireless modules
	 */
	if (MACH_TYPE_CCWMX51JS == variant_machid()) {
		val = pmic_reg(pmicslv, 34, 0, 0);
		val |= (1 << 12);
		pmic_reg(pmicslv, 34, val, 1);
	}

	for(i=0; i < ARRAY_SIZE(mmc_devs); i++) {
		if (!esdhc_gpio_init(mmc_devs[i]))
			fsl_esdhc_initialize(gd->bd, &esdhc_cfg[i]);
		else
			return -1;
	}
	return 0;
}
#endif

#ifdef CONFIG_LCD
#include "../include/displays/displays.h"

static unsigned short ipu_display_index;

void lcd_gpio_init(void)
{
#define DISP1_PAD		(PAD_CTL_DRV_HIGH | PAD_CTL_SRE_FAST)
#define DISP1_PAD_DATA		(PAD_CTL_DRV_HIGH | PAD_CTL_SRE_SLOW)

	mxc_iomux_set_pad(MX51_PIN_DI1_DISP_CLK, DISP1_PAD);
	mxc_iomux_set_pad(MX51_PIN_DI1_PIN15, DISP1_PAD);
	mxc_request_iomux(MX51_PIN_DI1_PIN2, IOMUX_CONFIG_ALT0);
	mxc_iomux_set_pad(MX51_PIN_DI1_PIN2, DISP1_PAD);
	mxc_request_iomux(MX51_PIN_DI1_PIN3, IOMUX_CONFIG_ALT0);
	mxc_iomux_set_pad(MX51_PIN_DI1_PIN3, DISP1_PAD);
	mxc_request_iomux(MX51_PIN_DISP1_DAT0, IOMUX_CONFIG_ALT0);
	mxc_iomux_set_pad(MX51_PIN_DISP1_DAT0, DISP1_PAD_DATA);
	mxc_request_iomux(MX51_PIN_DISP1_DAT1, IOMUX_CONFIG_ALT0);
	mxc_iomux_set_pad(MX51_PIN_DISP1_DAT1, DISP1_PAD_DATA);
	mxc_request_iomux(MX51_PIN_DISP1_DAT2, IOMUX_CONFIG_ALT0);
	mxc_iomux_set_pad(MX51_PIN_DISP1_DAT2, DISP1_PAD_DATA);
	mxc_request_iomux(MX51_PIN_DISP1_DAT3, IOMUX_CONFIG_ALT0);
	mxc_iomux_set_pad(MX51_PIN_DISP1_DAT3, DISP1_PAD_DATA);
	mxc_request_iomux(MX51_PIN_DISP1_DAT4, IOMUX_CONFIG_ALT0);
	mxc_iomux_set_pad(MX51_PIN_DISP1_DAT4, DISP1_PAD_DATA);
	mxc_request_iomux(MX51_PIN_DISP1_DAT5, IOMUX_CONFIG_ALT0);
	mxc_iomux_set_pad(MX51_PIN_DISP1_DAT5, DISP1_PAD_DATA);
	mxc_request_iomux(MX51_PIN_DISP1_DAT6, IOMUX_CONFIG_ALT0);
	mxc_iomux_set_pad(MX51_PIN_DISP1_DAT6, DISP1_PAD_DATA);
	mxc_request_iomux(MX51_PIN_DISP1_DAT7, IOMUX_CONFIG_ALT0);
	mxc_iomux_set_pad(MX51_PIN_DISP1_DAT7, DISP1_PAD_DATA);
	mxc_request_iomux(MX51_PIN_DISP1_DAT8, IOMUX_CONFIG_ALT0);
	mxc_iomux_set_pad(MX51_PIN_DISP1_DAT8, DISP1_PAD_DATA);
	mxc_request_iomux(MX51_PIN_DISP1_DAT9, IOMUX_CONFIG_ALT0);
	mxc_iomux_set_pad(MX51_PIN_DISP1_DAT9, DISP1_PAD_DATA);
	mxc_request_iomux(MX51_PIN_DISP1_DAT10, IOMUX_CONFIG_ALT0);
	mxc_iomux_set_pad(MX51_PIN_DISP1_DAT10, DISP1_PAD_DATA);
	mxc_request_iomux(MX51_PIN_DISP1_DAT11, IOMUX_CONFIG_ALT0);
	mxc_iomux_set_pad(MX51_PIN_DISP1_DAT11, DISP1_PAD_DATA);
	mxc_request_iomux(MX51_PIN_DISP1_DAT12, IOMUX_CONFIG_ALT0);
	mxc_iomux_set_pad(MX51_PIN_DISP1_DAT12, DISP1_PAD_DATA);
	mxc_request_iomux(MX51_PIN_DISP1_DAT13, IOMUX_CONFIG_ALT0);
	mxc_iomux_set_pad(MX51_PIN_DISP1_DAT13, DISP1_PAD_DATA);
	mxc_request_iomux(MX51_PIN_DISP1_DAT14, IOMUX_CONFIG_ALT0);
	mxc_iomux_set_pad(MX51_PIN_DISP1_DAT14, DISP1_PAD_DATA);
	mxc_request_iomux(MX51_PIN_DISP1_DAT15, IOMUX_CONFIG_ALT0);
	mxc_iomux_set_pad(MX51_PIN_DISP1_DAT15, DISP1_PAD_DATA);
	mxc_request_iomux(MX51_PIN_DISP1_DAT16, IOMUX_CONFIG_ALT0);
	mxc_iomux_set_pad(MX51_PIN_DISP1_DAT16, DISP1_PAD_DATA);
	mxc_request_iomux(MX51_PIN_DISP1_DAT17, IOMUX_CONFIG_ALT0);
	mxc_iomux_set_pad(MX51_PIN_DISP1_DAT17, DISP1_PAD_DATA);
#if defined(IPU_PIX_FMT_RGB24)
	mxc_request_iomux(MX51_PIN_DISP1_DAT18, IOMUX_CONFIG_ALT0);
	mxc_iomux_set_pad(MX51_PIN_DISP1_DAT18, DISP1_PAD_DATA);
	mxc_request_iomux(MX51_PIN_DISP1_DAT19, IOMUX_CONFIG_ALT0);
	mxc_iomux_set_pad(MX51_PIN_DISP1_DAT19, DISP1_PAD_DATA);
	mxc_request_iomux(MX51_PIN_DISP1_DAT20, IOMUX_CONFIG_ALT0);
	mxc_iomux_set_pad(MX51_PIN_DISP1_DAT20, DISP1_PAD_DATA);
	mxc_request_iomux(MX51_PIN_DISP1_DAT21, IOMUX_CONFIG_ALT0);
	mxc_iomux_set_pad(MX51_PIN_DISP1_DAT21, DISP1_PAD_DATA);
	mxc_request_iomux(MX51_PIN_DISP1_DAT22, IOMUX_CONFIG_ALT0);
	mxc_iomux_set_pad(MX51_PIN_DISP1_DAT22, DISP1_PAD_DATA);
	mxc_request_iomux(MX51_PIN_DISP1_DAT23, IOMUX_CONFIG_ALT0);
	mxc_iomux_set_pad(MX51_PIN_DISP1_DAT23, DISP1_PAD_DATA);
#endif
	mxc_request_iomux(MX51_PIN_DI1_PIN11, IOMUX_CONFIG_GPIO);
	mxc_iomux_set_pad(MX51_PIN_DI1_PIN11, PAD_CTL_PKE_ENABLE |
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
	imx_gpio_set_pin(MX51_PIN_DI1_PIN11, 0);
	imx_gpio_pin_cfg_dir(MX51_PIN_DI1_PIN11, 1);

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

	setup_boot_device();
	setup_soc_rev();

	/* address of boot parameters */
	gd->bd->bi_boot_params = PHYS_SDRAM_1 + 0x100;

#ifdef CONFIG_USER_LED
	setup_user_leds();
#endif
#if defined(CONFIG_SILENT_CONSOLE) && defined(ENABLE_CONSOLE_GPIO)
	init_console_gpio();
#endif
	setup_nfc();
#ifdef CONFIG_MXC_FEC
	setup_fec();
#endif
#ifdef CONFIG_I2C_MXC
	setup_i2c(CONFIG_SYS_I2C_PORT);
	i2c_init(CONFIG_SYS_I2C_SPEED, CONFIG_SYS_I2C_SLAVE);
#endif
#ifdef CONFIG_USER_KEY
	setup_user_keys();
#endif

	ccimx51_power_init();

	return 0;
}

#ifdef CONFIG_PRINT_SYS_INFO
extern void mxc_dump_clocks(void);
extern u32 __get_mcu_main_clk(void);
extern void show_pmic_info(struct spi_slave *slave);

void print_sys_info(void)
{
	char *s;

	if ((s = getenv("bv")) != NULL)
		boot_verb = simple_strtoul(s, NULL, 10);

	if (boot_verb) {

		printf("CPU:   Freescale i.MX51 family %d.%dV at %d MHz\n",
				(get_board_rev() & 0xFF) >> 4,
				(get_board_rev() & 0xF),
				__get_mcu_main_clk() / 1000000);

		show_pmic_info(pmicslv);

		if (is_valid_hw_id(mod_hwid.variant)) {
			puts("Module HW ID  : ");
			printf("%s (%02x)\n", ccimx51_id[mod_hwid.variant].id_string, mod_hwid.variant);
			printf("Module HW Rev : %02x\n", mod_hwid.version);
			printf("Module SN     : %c%d\n", mod_hwid.mloc, mod_hwid.sn);
		}

		mxc_dump_clocks();

		puts("Last reset was: ");
		switch (__REG(SRC_BASE_ADDR + 0x8)) {
		case 0x0001:	printf("POR\n");	break;
		case 0x0009:	printf("RST\n");	break;
		case 0x0010:
		case 0x0011:	printf("WDOG\n");	break;
		default:	printf("unknown\n");
		}

		printf("Boot Device   : ");
		switch (get_boot_device()) {
		case NAND_BOOT:		printf("NAND\n");	break;
		case MMC_BOOT:		printf("MMC\n");	break;
		case UNKNOWN_BOOT:
		default:		printf("UNKNOWN\n");	break;
		}
	}
}
#endif

#ifdef BOARD_LATE_INIT
int board_late_init(void)
{
	nv_critical_t *pNvram;

	setup_cpu_freq();

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
	printf("Board: CCIMX51 JSK Development Board\n");
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

	if ((reg + count - 1) > 63) {
		printf ("*** ERROR: invalid parameters, last addr should be less than 64\n");
		return 1;
	}

	if (!pmicslv) {
		printf ("*** ERROR: PMIC device not initialized yet\n");
		return 1;
	}

	for (i = reg; i < (reg + count); i++) {
		if (!((i - reg) % 4))
			printf("%s%02d:", (i - reg) ? "\n" : "", (int)i);
		printf(" %08x", pmic_reg(pmicslv, i, 0, 0));
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

	if (reg > 63) {
		printf ("*** ERROR: invalid parameters, reg addr should be less than 64\n");
		return 1;
	}

	if (!pmicslv) {
		printf ("*** ERROR: PMIC device not initialized yet\n");
		return 1;
	}

	pmic_reg(pmicslv, reg, val, 1);
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

#if defined(CONFIG_CMD_NAND_RFMT_SWAPBI) && defined(CONFIG_MXC_NAND_SWAP_BI)
#include <linux/mtd/mtd.h>
#include <nand.h>
static int nand_block_bad_scrub(struct mtd_info *mtd, loff_t ofs, int getchip)
{
	return 0;
}

static int do_nand_reformat_swapbi(cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
	nand_info_t *nand = &nand_info[0];
	struct nand_chip *chip = nand->priv;
	struct erase_info erase;
	ulong addr = 0, len, left;
	int ret = 0, percent, lastpercent = 0, format;
	unsigned long long val;
	int (*nand_block_bad_old)(struct mtd_info *, loff_t, int) = NULL;
	u_char dbuf[nand->erasesize];
	u_char oobbuf[nand->oobsize * (nand->erasesize / nand->writesize)];
	u_char *pdbuf, *poobbuf, byte1, byte2;
	mtd_oob_mode_t mode;
	int main_bb_pos = IS_4K_PAGE_NAND ?
			  MX5X_SWAP_BI_MAIN_BB_POS_4K :
			  MX5X_SWAP_BI_MAIN_BB_POS
	int spare_bb_pos = IS_4K_PAGE_NAND ?
			   MX5X_SWAP_BI_SPARE_BB_POS_4K :
			   MX5X_SWAP_BI_SPARE_BB_POS

	ret = WaitForYesPressed("Confirm that you want to reformat the flash? ",
				"nandrfmt");
	if (!ret)
		return 1;

	memset(&erase, 0, sizeof(erase));
	erase.mtd = nand;
	erase.len  = nand->erasesize;
	erase.callback = 0;

	nand_block_bad_old = chip->block_bad;
	chip->block_bad = nand_block_bad_scrub;

	while (addr < chip->chipsize) {

		if (unlikely(skip_swap_bi(addr >> chip->page_shift))) {
			addr += nand->erasesize;
			continue;
		}

		erase.addr = addr;
		poobbuf = oobbuf;
		pdbuf = dbuf;
		mode = chip->ops.mode;
		chip->ops.mode = MTD_OOB_RAW;
		len = nand->writesize;
		left = nand->erasesize;

		format = 0;
		/* read the full sector */
		while (left) {
			nand_read(nand, addr, (size_t *)&len, (u_char *)pdbuf);
			memcpy(poobbuf, chip->oob_poi, nand->oobsize);

			if ((*poobbuf != 0xff) && (left == nand->erasesize)) {
				/* if alredy marked in the other byte, skip */
				if (*(poobbuf + spare_bb_pos) != 0xff) {
					addr += nand->erasesize;
					goto update_percent;
				}
				*(poobbuf + spare_bb_pos) = *poobbuf;
				format = 1;
			}

			byte1 = *(pdbuf + main_bb_pos);
			byte2 = *(poobbuf + spare_bb_pos);

			if (byte1 != byte2) {
				*(pdbuf + main_bb_pos) = byte2;
				*(poobbuf + spare_bb_pos) = byte1;
		  		format = 1;
			}

			pdbuf += len;
			poobbuf += nand->oobsize;
			left -= len;
			addr += len;
		}

		if (!format)
			goto update_percent;

		chip->ops.mode = mode;
		poobbuf = oobbuf;
		pdbuf = dbuf;
		addr = erase.addr;
		left = nand->erasesize;

		/* erase the flash sector */
		ret = nand->erase(nand, &erase);
		if (ret != 0) {
			printf("\n%s: MTD Erase failure: %d\n",
			       nand->name, ret);
			ret = 1;
			goto restore_bad_ret;
		}

		/*write block with modification and keep oob*/
		while (left) {
			memcpy(chip->oob_poi, poobbuf, nand->oobsize);
			chip->cmdfunc(nand, NAND_CMD_SEQIN, 0x00, addr >> chip->page_shift);
			chip->ecc.write_page_raw(nand, chip, pdbuf);
			chip->cmdfunc(nand, NAND_CMD_PAGEPROG, -1, -1);
			chip->waitfunc(nand, chip);
#if CONFIG_NAND_RFMT_VERIFY
			chip->cmdfunc(nand, NAND_CMD_READ0, 0, addr >> chip->page_shift);
			if (chip->verify_buf(nand, pdatbuf, nand->writesize)) {
				printf("\n%s: MTD Verify failure\n", nand->name);
				ret = 1;
				goto restore_bad_ret;
			}
#endif
			pdbuf += len;
			poobbuf += nand->oobsize;
			left -= len;
			addr += len;
		}
update_percent:
		val = (unsigned long long)addr * 100;
		do_div(val, chip->chipsize);
		percent = (int)val;

		if (percent != lastpercent) {
			lastpercent = percent;
			printf("\rReformating @ 0x%08x -- %3d%% complete",
				(u_int)addr, percent);
		}
	}
	puts("\n");

restore_bad_ret:
	chip->block_bad = nand_block_bad_old;
	return ret;
}

U_BOOT_CMD(
	nandrfmt,	1,	0,	do_nand_reformat_swapbi,
	"nandrfmt",
	" - Reformats the flash changing the Bad Block byte to a new location\n"
	"            to better use the capabilities of the nand flash controller\n"
);
#endif /* CONFIG_CMD_NAND_RFMT_SWAPBI */

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

#define SIZE_128MB	0x8000000

/*
 * This function returns the 'loadaddr', the RAM address to use to transfer
 * files to, before booting or updating firmware.
 */
unsigned int get_platform_loadaddr(char *loadaddr)
{
	/* We distinguish two RAM memory variants: 128MB and bigger
	 * (256MB, 512MB)
	 * U-Boot is loaded at 120MB offset.
	 */
	if (!strcmp(loadaddr, "loadaddr")) {
		if (gd->bd->bi_dram[0].size > SIZE_128MB)
			/* For RAM > 128MB: 128MB */
			return (gd->bd->bi_dram[0].start + SIZE_128MB);
		else
			/* For RAM == 128MB: CONFIG_LOADADDR */
			return CONFIG_LOADADDR;
	} else if (!strcmp(loadaddr, "wceloadaddr")) {
		/* WinCE images must be located in RAM and then uncompressed
		 * to their final location, so we need to set a load address
		 * half-way through the available RAM
		 */
		if (gd->bd->bi_dram[0].size > SIZE_128MB)
			/* For RAM > 128MB: 128MB */
			return (gd->bd->bi_dram[0].start + SIZE_128MB);
		else
			/* For RAM == 128MB: 64MB (half the memory) */
			return (gd->bd->bi_dram[0].start +
				(gd->bd->bi_dram[0].size / 2));
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
	unsigned int la_off = loadaddr - gd->bd->bi_dram[0].start;

	/* We distinguish two RAM memory variants: 128MB and bigger
	 * (256MB, 512MB)
	 * U-Boot is loaded at 120MB offset.
	 */
	if (gd->bd->bi_dram[0].size > SIZE_128MB) {
		if (la_off >= SIZE_128MB)
			/*  total RAM - loadaddr offset */
			return (get_total_ram() - la_off);
		else
			/* 128M - UBOOT_RESERVED - loadaddr offset */
			return (SIZE_128MB - CONFIG_UBOOT_RESERVED_RAM -
				la_off);
	} else
		/*  For 128MB: total ram - UBOOT_RESERVED - loadaddr offset */
		return (get_total_ram() - CONFIG_UBOOT_RESERVED_RAM - la_off);
}
