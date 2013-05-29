/*
 * (C) Copyright 2002-2006
 * Wolfgang Denk, DENX Software Engineering, wd@denx.de.
 *
 * (C) Copyright 2002
 * Sysgo Real-Time Solutions, GmbH <www.elinos.com>
 * Marius Groeger <mgroeger@sysgo.de>
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

/*
 * To match the U-Boot user interface on ARM platforms to the U-Boot
 * standard (as on PPC platforms), some messages with debug character
 * are removed from the default U-Boot build.
 *
 * Define DEBUG here if you want additional info as shown below
 * printed upon startup:
 *
 * U-Boot code: 00F00000 -> 00F3C774  BSS: -> 00FC3274
 * IRQ Stack: 00ebff7c
 * FIQ Stack: 00ebef7c
 */

#include <common.h>
#include <command.h>
#include <malloc.h>
#include <stdio_dev.h>
#include <timestamp.h>
#include <version.h>
#include <net.h>
#include <serial.h>
#include <nand.h>
#include <onenand_uboot.h>
#include <mmc.h>
#include <linux/mtd/mtd.h>

#if defined (CONFIG_MX28)
#include <asm/arch/regsicoll.h>
#endif

#ifdef CONFIG_DRIVER_SMC91111
#include "../drivers/net/smc91111.h"
#endif
#ifdef CONFIG_DRIVER_LAN91C96
#include "../drivers/net/lan91c96.h"
#endif

#if CONFIG_CMD_BSP
extern int get_module_hw_id(void);
extern int variant_has_ethernet(void);
extern void config_after_flash_init(void);
#define UBOOT
#include "../common/digi/cmd_nvram/lib/include/nvram.h"
#undef DEBUG  /* just in case it were defined in nvram.h and friends */
#else
#include <malloc.h>
#endif

extern int do_reset (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[]);	/* for do_reset() prototype */
#if defined (CONFIG_MX28)
extern int mx28_get_bootmode(void);
#endif

DECLARE_GLOBAL_DATA_PTR;

ulong monitor_flash_len;

#ifdef CONFIG_HAS_DATAFLASH
extern int  AT91F_DataflashInit(void);
extern void dataflash_print_info(void);
#endif

#ifdef CONFIG_AUTOLOAD_BOOTSCRIPT
extern void run_auto_script(void);
#endif

#if defined CONFIG_SPLASH_SCREEN && defined CONFIG_VIDEO_MX5
extern void setup_splash_image(void);
#endif

#ifndef CONFIG_IDENT_STRING
#define CONFIG_IDENT_STRING ""
#endif

const char version_string[] =
	U_BOOT_VERSION" - (" U_BOOT_DATE " - " U_BOOT_TIME ")"CONFIG_IDENT_STRING;

#ifdef CONFIG_DRIVER_CS8900
extern void cs8900_get_enetaddr (void);
#endif

#ifdef CONFIG_DRIVER_RTL8019
extern void rtl8019_get_enetaddr (uchar * addr);
#endif

#ifdef CONFIG_LCD
extern int lcd_display_init(void);
#endif

#if defined(CONFIG_HARD_I2C) || \
    defined(CONFIG_SOFT_I2C)
#include <i2c.h>
#endif

/*
 * Begin and End of memory area for malloc(), and current "brk"
 */
static ulong mem_malloc_start = 0;
static ulong mem_malloc_end = 0;
static ulong mem_malloc_brk = 0;

static
void mem_malloc_init (ulong dest_addr)
{
	mem_malloc_start = dest_addr;
	mem_malloc_end = dest_addr + CONFIG_SYS_MALLOC_LEN;
	mem_malloc_brk = mem_malloc_start;

	memset ((void *) mem_malloc_start, 0,
			mem_malloc_end - mem_malloc_start);
}

void *sbrk (ptrdiff_t increment)
{
	ulong old = mem_malloc_brk;
	ulong new = old + increment;

	if ((new < mem_malloc_start) || (new > mem_malloc_end)) {
		return (NULL);
	}
	mem_malloc_brk = new;

	return ((void *) old);
}


/************************************************************************
 * Coloured LED functionality
 ************************************************************************
 * May be supplied by boards if desired
 */
void inline __coloured_LED_init (void) {}
void coloured_LED_init (void) __attribute__((weak, alias("__coloured_LED_init")));
void inline __red_LED_on (void) {}
void red_LED_on (void) __attribute__((weak, alias("__red_LED_on")));
void inline __red_LED_off(void) {}
void red_LED_off(void) __attribute__((weak, alias("__red_LED_off")));
void inline __green_LED_on(void) {}
void green_LED_on(void) __attribute__((weak, alias("__green_LED_on")));
void inline __green_LED_off(void) {}
void green_LED_off(void) __attribute__((weak, alias("__green_LED_off")));
void inline __yellow_LED_on(void) {}
void yellow_LED_on(void) __attribute__((weak, alias("__yellow_LED_on")));
void inline __yellow_LED_off(void) {}
void yellow_LED_off(void) __attribute__((weak, alias("__yellow_LED_off")));
void inline __blue_LED_on(void) {}
void blue_LED_on(void) __attribute__((weak, alias("__blue_LED_on")));
void inline __blue_LED_off(void) {}
void blue_LED_off(void) __attribute__((weak, alias("__blue_LED_off")));

/************************************************************************
 * Init Utilities							*
 ************************************************************************
 * Some of this code should be moved into the core functions,
 * or dropped completely,
 * but let's get it working (again) first...
 */

#if defined(CONFIG_ARM_DCC) && !defined(CONFIG_BAUDRATE)
#define CONFIG_BAUDRATE 115200
#endif
extern int isvalid_baudrate(int baudrate);

#define init_baudrate	init_baudrate_from_env
int init_baudrate_from_env (void)
{
	int baudrate = CONFIG_BAUDRATE;
#ifdef CONFIG_GET_BAUDRATE_FROM_VAR
	char tmp[64];	/* long enough for environment variables */
	int i = getenv_r ("baudrate", tmp, sizeof (tmp));

	if (i > 0) {
		/* Check that the baudrate stored in the variable is supported.
		* U-Boot doesn't allow you to change the variable to a wrong
		* value, but the value could be changed from Linux or the
		* NVRAM could be corrupted for some reason. */
		baudrate = (int) simple_strtoul (tmp, NULL, 10);
	}
#endif
	if (!isvalid_baudrate(baudrate)) {
		baudrate = CONFIG_PLATFORM_BAUDRATE;
	}
	gd->bd->bi_baudrate = gd->baudrate = baudrate;

	return (0);
}

int display_banner (void)
{
	printf ("\n\n%s\n\n", version_string);
	debug ("U-Boot code: %08lX -> %08lX  BSS: -> %08lX\n",
	       _armboot_start, _bss_start, _bss_end);
#ifdef CONFIG_MODEM_SUPPORT
	debug ("Modem Support enabled\n");
#endif
#ifdef CONFIG_USE_IRQ
	debug ("IRQ Stack: %08lx\n", IRQ_STACK_START);
	debug ("FIQ Stack: %08lx\n", FIQ_STACK_START);
#endif

	return (0);
}

/*
 * WARNING: this code looks "cleaner" than the PowerPC version, but
 * has the disadvantage that you either get nothing, or everything.
 * On PowerPC, you might see "DRAM: " before the system hangs - which
 * gives a simple yet clear indication which part of the
 * initialization if failing.
 */
int display_dram_config (void)
{
	int i;

#ifdef DEBUG
	puts ("RAM Configuration:\n");

	for(i=0; i<CONFIG_NR_DRAM_BANKS; i++) {
		printf ("Bank #%d: %08lx ", i, gd->bd->bi_dram[i].start);
		print_size (gd->bd->bi_dram[i].size, "\n");
	}
#else
	ulong size = 0;

	for (i=0; i<CONFIG_NR_DRAM_BANKS; i++) {
		size += gd->bd->bi_dram[i].size;
	}
	puts("DRAM:  ");
	print_size(size, "\n");
#endif

	return (0);
}

#ifndef CONFIG_SYS_NO_FLASH
static void display_flash_config (ulong size)
{
	puts ("Flash: ");
	print_size (size, "\n");
}
#endif /* CONFIG_SYS_NO_FLASH */

#if defined(CONFIG_HARD_I2C) || defined(CONFIG_SOFT_I2C)
static int init_func_i2c (void)
{
	puts ("I2C:   ");
	i2c_init (CONFIG_SYS_I2C_SPEED, CONFIG_SYS_I2C_SLAVE);
	puts ("ready\n");
	return (0);
}
#endif

#if defined(CONFIG_CMD_PCI) || defined (CONFIG_PCI)
#include <pci.h>
static int arm_pci_init(void)
{
	pci_init();
	return 0;
}
#endif /* CONFIG_CMD_PCI || CONFIG_PCI */

/*
 * Breathe some life into the board...
 *
 * Initialize a serial port as console, and carry out some hardware
 * tests.
 *
 * The first part of initialization is running from Flash memory;
 * its main purpose is to initialize the RAM so that we
 * can relocate the monitor code to RAM.
 */

/*
 * All attempts to come up with a "common" initialization sequence
 * that works for all boards and architectures failed: some of the
 * requirements are just _too_ different. To get rid of the resulting
 * mess of board dependent #ifdef'ed code we now make the whole
 * initialization sequence configurable to the user.
 *
 * The requirements for any new initalization function is simple: it
 * receives a pointer to the "global data" structure as it's only
 * argument, and returns an integer return code, where 0 means
 * "continue" and != 0 means "fatal error, hang the system".
 */
typedef int (init_fnc_t) (void);

int print_cpuinfo (void);

init_fnc_t *init_sequence[] = {
#if defined(CONFIG_ARCH_CPU_INIT)
	arch_cpu_init,		/* basic arch cpu dependent setup */
#endif
	board_init,		/* basic board dependent setup */
#if defined(CONFIG_USE_IRQ)
	interrupt_init,		/* set up exceptions */
#endif
	timer_init,		/* initialize timer */
	env_init,		/* initialize environment */
	init_baudrate,		/* initialze baudrate settings */
	serial_init,		/* serial communications setup */
	console_init_f,		/* stage 1 init of console */
	display_banner,		/* say that we are here */
#if defined(CONFIG_DISPLAY_CPUINFO)
	print_cpuinfo,		/* display cpu info (and speed) */
#endif
#if defined(CONFIG_DISPLAY_BOARDINFO)
	checkboard,		/* display board info */
#endif
#if defined(CONFIG_HARD_I2C) || defined(CONFIG_SOFT_I2C)
	init_func_i2c,
#endif
#if defined(CONFIG_CMD_PCI) || defined (CONFIG_PCI)
	arm_pci_init,
#endif
	NULL,
};

void start_armboot (void)
{
	init_fnc_t **init_fnc_ptr;
	char *s;
#if defined(CONFIG_VFD) || defined(CONFIG_LCD)
	unsigned long addr = 0;
#endif

#if defined (CONFIG_MX28)
	// Very early on disable the USB0 IRQ to avoid spurious interrupts
	HW_ICOLL_INTERRUPTn_CLR(93,0x00000004);
#endif

	/* Pointer is writable since we allocated a register for it */
	gd = (gd_t*)(_armboot_start - CONFIG_SYS_MALLOC_LEN - sizeof(gd_t));
	/* compiler optimization barrier needed for GCC >= 3.4 */
	__asm__ __volatile__("": : :"memory");

	memset ((void*)gd, 0, sizeof (gd_t));
	gd->bd = (bd_t*)((char*)gd - sizeof(bd_t));
	memset (gd->bd, 0, sizeof (bd_t));

	gd->flags |= GD_FLG_RELOC;

	monitor_flash_len = _bss_start - _armboot_start;

	for (init_fnc_ptr = init_sequence; *init_fnc_ptr; ++init_fnc_ptr) {
		if ((*init_fnc_ptr)() != 0) {
			hang ();
		}
	}

	/* armboot_start is defined in the board-specific linker script */
	mem_malloc_init (_armboot_start - CONFIG_SYS_MALLOC_LEN);

#ifndef CONFIG_SYS_NO_FLASH
	/* configure available FLASH banks */
	display_flash_config (flash_init ());
#endif /* CONFIG_SYS_NO_FLASH */

#ifdef CONFIG_VFD
#	ifndef PAGE_SIZE
#	  define PAGE_SIZE 4096
#	endif
	/*
	 * reserve memory for VFD display (always full pages)
	 */
	/* bss_end is defined in the board-specific linker script */
	addr = (_bss_end + (PAGE_SIZE - 1)) & ~(PAGE_SIZE - 1);
	vfd_setmem (addr);
	gd->fb_base = addr;
#endif /* CONFIG_VFD */

#if defined(CONFIG_CMD_NAND)
	puts ("NAND:  ");
	nand_init();		/* go init the NAND */
#endif

#if defined(CONFIG_CMD_ONENAND)
	onenand_init();
#endif

#ifdef CONFIG_HAS_DATAFLASH
	AT91F_DataflashInit();
	dataflash_print_info();
#endif

#ifdef CONFIG_AFTER_FLASH_INIT
	config_after_flash_init();
#endif

#if defined(CONFIG_MX28) && defined(CONFIG_CMD_BOOTSTREAM) && defined(CONFIG_NAND_GPMI)
#define FCB_EXPECTED_OFFSET	6
	{
		unsigned char *nandbuf;
		nand_info_t *nand = &nand_info[nand_curr_device];;
		size_t size;
		int ret;
		char *s;
		int i;

		/* When running the CPU at 454MHz there are occasions in which
		 * the NAND flash is not read correctly. There seems to be a problem
		 * with the NAND controller because the data (when read) shows
		 * displaced with a certain offset. As if the NAND was not properly
		 * relocating the ECC metada or something.
		 * This problem can easily be detected by reading the first block
		 * of the NAND (guaranteed to be good) and locating the "FCB" signature
		 * which (if the NAND is properly configured) will appear at offset 6.
		 * Notice this is not the real offset of FCB at the NAND, but after being
		 * processed by the NAND controller and put into the RAM buffer.
		 * If we detect that the NVRAM cannot be read and the FCB signature is
		 * not at the offset we expect it to be, then we hit the condition and
		 * we must reset the CPU, to read the NVRAM correctly.
		 * DIGI_UBOOT-22
		 */
		nandbuf = malloc(nand->writesize);
		s = (char *)nandbuf;
		size = nand->writesize;
		if (NULL != nandbuf) {
			/* Read the first block and check the FCB signature */
			ret = nand_read_skip_bad(nand, 0, &size, (u_char *)nandbuf);
			for (i=0; i < 100; i++) {
				if (s[i] == 'F' && s[i+1] == 'C' && s[i+2] == 'B')
					break;
			}
			if (i < 100 && i != FCB_EXPECTED_OFFSET) {
				int bootmode = mx28_get_bootmode();

				/* FCB was found but not at the expected offset.
				 * The NAND controller went crazy!
				 */
				printf("NAND controller badly initialized\n");
				if (BOOTMODE_USB0 != bootmode) {
					/* Reset CPU unless booting from USB (recover) */
					do_reset(NULL, 0, 0, NULL);
				}
			}
			free(nandbuf);
		}
	}
#endif

#ifdef CONFIG_GENERIC_MMC
	puts ("MMC:   ");
	mmc_initialize (gd->bd);
#endif

	/* initialize environment */
	env_relocate ();

#if defined(CONFIG_CMD_BSP) && defined(CONFIG_PLATFORM_HAS_HWID)
	get_module_hw_id();
#endif
	/* configure available RAM banks */
	dram_init();
	display_dram_config();

	/* now we have a valid environment, even when reading from NAND. */
#if defined(CONFIG_CMD_BSP)
       if (bsp_init())             /* initialize common Digi BSP stuff */
	       printf("Error during BSP initialization!\n");
#endif
	/* Switch to the configured baudrate. */
#ifdef CONFIG_SERIAL_MULTI
	serial_initialize();
#endif
	init_baudrate_from_env();
#if !defined(CONFIG_SILENT_CONSOLE) && !defined(CONFIG_UBOOT_JTAG_CONSOLE)
	serial_setbrg();

	if (gd->bd->bi_baudrate != CONFIG_BAUDRATE)
		display_banner();
#endif

#ifdef CONFIG_CMD_BSP
	gd->bd->fb_base = 0xffffffff;
#endif
#ifdef CONFIG_LCD
	/* board init may have inited fb_base */
	if (!gd->fb_base) {
#		ifndef PAGE_SIZE
#		  define PAGE_SIZE 4096
#		endif
		/*
		 * reserve memory for LCD display (always full pages)
		 */
#if defined(CONFIG_DISPLAY1_ENABLE) || defined(CONFIG_DISPLAY2_ENABLE)
		if (!lcd_display_init()) {
			char* saddr;
			saddr = getenv ("fb_base");
			if(saddr != NULL)
				addr = simple_strtoul( saddr, NULL, 16 );
			addr = lcd_setmem(addr);
			gd->fb_base = addr;
			gd->bd->fb_base = addr;
		}
#else
		/* bss_end is defined in the board-specific linker script */
		addr = (_bss_end + (PAGE_SIZE - 1)) & ~(PAGE_SIZE - 1);
		lcd_setmem (addr);
		gd->fb_base = addr;
#endif
	}
#endif /* CONFIG_LCD */

#if CONFIG_CMD_BSP
	/* print any error messages of NVRAM reading */
	NvEnableOutput(1);
	NvPrintStatus();
#endif  /* CONFIG_CMD_BSP */

#ifdef CONFIG_VFD
	/* must do this after the framebuffer is allocated */
	drv_vfd_init();
#endif /* CONFIG_VFD */

#if CONFIG_CMD_BSP
	/**
	 * baudrate is being read from NvRAM. If one mirror image is wrong,
	 * don't do any output yet in the default baudrate as we will switch
	 * later to the second image.
	 */
	NvEnableOutput(0);
#endif  /* CONFIG_CMD_BSP */

	/* IP Address */
	gd->bd->bi_ip_addr = getenv_IPaddr ("ipaddr");

#if defined CONFIG_SPLASH_SCREEN && defined CONFIG_VIDEO_MX5 && !defined(CONFIG_CCIMX5X)
	setup_splash_image();
#endif

	stdio_init ();	/* get the devices list going. */

	jumptable_init ();

#if defined(CONFIG_API)
	/* Initialize API */
	api_init ();
#endif

	console_init_r ();	/* fully init console as a device */

#if defined(CONFIG_ARCH_MISC_INIT)
	/* miscellaneous arch dependent initialisations */
	arch_misc_init ();
#endif
#if defined(CONFIG_MISC_INIT_R)
	/* miscellaneous platform dependent initialisations */
	misc_init_r ();
#endif

	/* enable exceptions */
	enable_interrupts ();

	/* Perform network card initialisation if necessary */
#ifdef CONFIG_DRIVER_TI_EMAC
	/* XXX: this needs to be moved to board init */
extern void davinci_eth_set_mac_addr (const u_int8_t *addr);
	if (getenv ("ethaddr")) {
		uchar enetaddr[6];
		eth_getenv_enetaddr("ethaddr", enetaddr);
		davinci_eth_set_mac_addr(enetaddr);
	}
#endif

#ifdef CONFIG_DRIVER_CS8900
	/* XXX: this needs to be moved to board init */
	cs8900_get_enetaddr ();
#endif

#if defined(CONFIG_DRIVER_SMC91111) || defined (CONFIG_DRIVER_LAN91C96)
	/* XXX: this needs to be moved to board init */
	if (getenv ("ethaddr")) {
		uchar enetaddr[6];
		eth_getenv_enetaddr("ethaddr", enetaddr);
		smc_set_mac_addr(enetaddr);
	}
#endif /* CONFIG_DRIVER_SMC91111 || CONFIG_DRIVER_LAN91C96 */

#if defined(CONFIG_ENC28J60_ETH) && !defined(CONFIG_ETHADDR)
	extern void enc_set_mac_addr (void);
	enc_set_mac_addr ();
#endif /* CONFIG_ENC28J60_ETH && !CONFIG_ETHADDR*/

	/* Initialize from environment */
	if ((s = getenv ("loadaddr")) != NULL) {
		load_addr = simple_strtoul (s, NULL, 16);
	}
#if defined(CONFIG_CMD_NET)
	if ((s = getenv ("bootfile")) != NULL) {
		copy_filename (BootFile, s, sizeof (BootFile));
	}
#endif

#ifdef BOARD_LATE_INIT
	board_late_init ();
#endif

#ifdef CONFIG_ANDROID_RECOVERY
	check_recovery_mode();
#endif

#if defined(CONFIG_CMD_NET)
#if defined(CONFIG_PLATFORM_HAS_HWID)
	if (variant_has_ethernet())
#endif
	{
#if defined(CONFIG_NET_MULTI)
		puts ("Net:   ");
#endif
		eth_initialize(gd->bd);
#if defined(CONFIG_RESET_PHY_R)
		debug ("Reset Ethernet PHY\n");
		reset_phy();
#endif
	}
#endif
#ifdef CONFIG_AUTOLOAD_BOOTSCRIPT
	run_auto_script();
#endif
#ifdef CONFIG_CMD_BSP
	/* Enable output */
	NvEnableOutput(1);
#endif
#ifdef BOARD_BEFORE_MLOOP_INIT
	board_before_mloop_init ();
#endif
	/* main_loop() can return to retry autoboot, if so just run it again. */
	for (;;) {
		main_loop ();
	}

	/* NOTREACHED - no way out of command loop except booting */
}

void hang (void)
{
	puts ("### ERROR ### Please RESET the board ###\n");
	for (;;);
}
