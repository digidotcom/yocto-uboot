/*
 *  /targets/U-Boot.cvs/common/digi/cmd_bsp.c
 *
 *  Copyright (C) 2006 by Digi International Inc.
 *  All rights reserved.
 *
 *  This program is free software; you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License version2  as published by
 *  the Free Software Foundation.
*/

/*
 *  !Revision:   $Revision$
 *  !Author:     Markus Pietrek
 *  !References: [1] http://www.linux-mtd.infradead.org/doc/nand.html
*/

/*
 * Digi CC specific functions
 */

#include <common.h>
#include <asm/io.h>
#include <linux/ctype.h>        /* is_digit */
#include <image.h>
#include <linux/time.h>
#include <rtc.h>
#ifdef CONFIG_CMD_BSP
#include <command.h>
#ifdef CONFIG_CMD_NAND
# include <nand.h>
#endif
#ifdef CONFIG_CMD_BOOTSTREAM
# include "cmd_bootstream.h"
#endif
#include <jffs2/jffs2.h>
#include <net.h>                /* DHCP */
#include <u-boot/zlib.h>        /* inflate */

#include "cmd_bsp.h"
#include "cmd_video.h"
#include "env.h"
#include "nvram.h"
#include "partition.h"          /* MtdGetEraseSize */
#include "mtd.h"
#include "helper.h"
#include "safe_strcat.h"

/* ------------*/
#include <configs/digi_common_post.h>
/* ----------- */
#define SNFS	"snfs"
#define SMTD	"smtd"
#define NPATH	"npath"
#define RIMG    "rimg"
#define ARIMG   "arimg"
#define USRIMG  "usrimg"
#define CONSOLE "console"
#define LPJ     "lpj"

/* Constants for bootscript download timeouts */
#define AUTOSCRIPT_TFTP_MSEC	100
#define AUTOSCRIPT_TFTP_CNT	15
#define AUTOSCRIPT_START_AGAIN	100

/* from cmd_bootm */
#define HEAD_CRC		2
#define EXTRA_FIELD		4
#define ORIG_NAME		8
#define COMMENT			0x10
#define RESERVED		0xe0
#define DEFLATED		8

#define PATH_MAXLEN		256

/* to transform a value into a string, from environment.c */

#define CE( sCmd ) \
        do { \
                if( !(sCmd) )                   \
                        goto error; \
        } while( 0 )

#define CEN( sCmd ) \
        do { \
                if( sCmd <= 0 )                  \
                        goto error; \
        } while( 0 )

#ifdef CONFIG_CONS_INDEX_SUB_1
# define CONSOLE_INDEX ((CONFIG_CONS_INDEX)-1)
#else
# define CONSOLE_INDEX (CONFIG_CONS_INDEX)
#endif

#if defined( CONFIG_APPEND_CRC32 )
# define UBOOT_IMG_HAS_CRC32 1
#else
# define UBOOT_IMG_HAS_CRC32 0
#endif

#ifdef CONFIG_NETOS_SWAP_ENDIAN
# define NETOS_SWAP_ENDIAN 1
#else
# define NETOS_SWAP_ENDIAN 0
#endif

DECLARE_GLOBAL_DATA_PTR;

/* ********** local typedefs ********** */

typedef struct {
        nv_os_type_e   eOSType;
        const char*    szName;       /*! short OS name */
        const char*    szEnvVar;
        nv_part_type_e ePartType;
        char           bForBoot;
        /* a compressed image can't be used for booting from TFTP/USB/NFS
         * because we can't decompress it on the fly */
        char           bForBootFromFlashOnly;
        char           bRootFS;
        char           bCRC32Appended;
        char           bSwapEndian;
} part_t;

typedef struct {
	char* szEnvVar;
	char* szEnvDflt;
} env_default_t;

typedef struct {
        image_source_e eType;
        const char*    szName;       /*! short image name */
} image_source_t;

/* ********** local functions ********** */

static int do_digi_dboot(cmd_tbl_t* cmdtp, int flag, int argc, char * const argv[]);
static int do_digi_update(cmd_tbl_t* cmdtp, int flag, int argc, char * const argv[]);
static int do_digi_verify(cmd_tbl_t* cmdtp, int flag, int argc, char * const argv[]);
#if defined(CONFIG_CMD_UBI)
extern int ubi_volume_verify(char *volume, char *buf, loff_t offset, size_t size, char skipUpdFlagCheck);
extern int ubi_volume_get_leb_size(char *volume);
extern int ubi_volume_off_write_break(char *volume);
static int RecreateUBIVolume ( const nv_param_part_t* pPartEntry );
#endif
#if defined(CONFIG_CMD_NAND) && !defined(CONFIG_NAND_OBB_ATOMIC)
static int jffs2_mark_clean (long long offset, long long size);
#endif
#if defined(CONFIG_DUAL_BOOT)
# if defined(CONFIG_DUAL_BOOT_SILENT)
#  define DUALB_PRINT(fmt, args...)	while(0) {}
#else
#  define DUALB_PRINT(fmt, args...)	printf(fmt, ##args)
#endif
#ifdef CONFIG_DUAL_BOOT_WDT_ENABLE
extern unsigned long board_wdt_enable(unsigned long seconds);
#endif
extern void dualb_increment_bootattempts(void);
extern int dualb_get_bootattempts(void);
extern void dualb_reset_bootattempts(void);
extern void dualb_nosystem_panic(void);
extern int do_reset (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[]);		/* for do_reset() prototype */
int dualb_save(void);
int dualb_toggle(void);
static void start_wdt(void);
#endif /* CONFIG_DUAL_BOOT */

extern int variant_has_wireless(void);

static int append_calibration(void);
static int WhatPart(
        const char* szPart, char bForBoot,
        const part_t** ppPart,
        const nv_param_part_t** ppPartEntry,
        char bPartEntryRequired,
	nv_os_type_e *os_type,
	int count);
static const image_source_t* WhatImageSource( const char* szSrc );
static int RunCmd( const char* szCmd );
static int GetIntFromEnvVar( /*@out@*/ int* piVal, const char* szVar,
                             char bSilent );
static int AppendPadding( const nv_param_part_t* pPart, void* pvImage,
                          /*@out@*/ int* piFileSize );
static size_t GetEraseSize( const nv_param_part_t* pPart );
static size_t GetPageSize( const nv_param_part_t* pPart );
static const nv_param_part_t* FindPartition( const char* szName );
static int GetDHCPEnabled( char* pcEnabled );
static int DoDHCP( void );
static int check_image_header(ulong img_addr);
static int VerifyCRC( uchar *addr, ulong len, ulong crc );
int findpart_tableentry(  const part_t **ppPart,
	const nv_param_part_t *pPartEntry,
        int            iCount );

#ifdef CONFIG_COMPUTE_LPJ
extern unsigned int get_preset_lpj(void);
#endif

#ifdef CONFIG_BOARD_SETUP_BEFORE_OS
extern void board_setup_before_os_jump(nv_os_type_e eOSType, image_source_e eType);
#endif

extern int NetSilent;		/* Whether to silence the net commands output */
extern ulong TftpRRQTimeoutMSecs;
extern int TftpRRQTimeoutCountMax;
extern unsigned long NetStartAgainTimeout;
int DownloadingAutoScript = 0;
int RunningAutoScript = 0;
int LoadFileSilent = 1;
nv_os_type_e eOSType = NVOS_NONE;
nv_dualb_t dualb, original_dualb;
nv_dualb_data_t *dualb_data = &dualb.data;

#ifdef CONFIG_TFTP_UPDATE_ONTHEFLY
/* external variables for tftp_direct_to_flash(), shared with TftpHandler */
extern char bTftpToFlashStatus;				/* Signaling flags for TftpHandler */
extern size_t iFlashEraseSize;
extern size_t iFlashPageSize;
extern uint64_t iPartitionStartAdress;
extern uint64_t iPartitionSize;
extern const struct nv_param_part* pPartToWrite;	/* pointer to partition */
#endif /* CONFIG_TFTP_UPDATE_ONTHEFLY */

extern void netboot_update_env (void);
extern unsigned int get_tftp_available_ram(unsigned int loadaddr);

/* ********** local variables ********** */

static char modulename[PLATFORMNAME_MAXLEN];
static char platformname[PLATFORMNAME_MAXLEN];
static int dynvars = 0;
static env_default_t l_axEnvDynamic[MAX_DYNVARS];	/* Dynamic variables */

static const part_t l_axPart[] = {
        { NVOS_LINUX, "linux",  "kimg",   NVPT_LINUX,      .bForBoot = 1 },
        { NVOS_ANDROID, "android",  "aimg",   NVPT_LINUX,      .bForBoot = 1 },

#ifdef CONFIG_IS_NETSILICON
# ifdef PART_NETOS_LOADER_SIZE
        { NVOS_NETOS_LOADER, "netos_loader", "nloader", NVPT_NETOS_LOADER,
          .bSwapEndian = NETOS_SWAP_ENDIAN },
# endif
        { NVOS_NETOS, "netos",  "nimg",   NVPT_NETOS,
          .bSwapEndian = NETOS_SWAP_ENDIAN, .bForBoot = 1 },
#endif  /* CONFIG_IS_NETSILICON */

#ifdef CONFIG_CMD_BOOTSTREAM
	{ NVOS_UBOOT, "uboot", "uimg", NVPT_BOOTSTREAM,		},
#else
        { NVOS_UBOOT, "uboot",  "uimg",   NVPT_UBOOT,
          .bCRC32Appended = UBOOT_IMG_HAS_CRC32                          },
#endif
#ifdef CONFIG_FPGA_SIZE
        { NVOS_NONE,  "fpga",   "fimg",   NVPT_FPGA,                     },
#endif
#ifdef CONFIG_UBOOT_SPLASH
	{ NVOS_NONE,  "splash",  "simg",  NVPT_SPLASH_SCREEN,            },
#endif
        { NVOS_NONE,  "rootfs", RIMG,     NVPT_FILESYSTEM, .bRootFS = 1  },
        { NVOS_ANDROID,  "androidfs", ARIMG, NVPT_FILESYSTEM, .bRootFS = 1  },
        { NVOS_NONE,  "userfs", USRIMG,   NVPT_FILESYSTEM,               },
	{ NVOS_FDT,  "fdt", "fdtimg",   NVPT_FDT,               },
};

/* use MK because we later use l_axImgSrc[ IS_FLASH ] */
#define MK(x,y) [x] = { .eType = x, .szName = y }
static const image_source_t l_axImgSrc[] = {
        MK( IS_TFTP,  "tftp"  ),
        MK( IS_NFS,   "nfs"   ),
        MK( IS_FLASH, "flash" ),
        MK( IS_USB,   "usb"   ),
        MK( IS_MMC,   "mmc"   ),
        MK( IS_HSMMC, "hsmmc" ),
        MK( IS_RAM,   "ram"   ),
        MK( IS_SATA,   "sata"   ),
};
#undef MK

/* ********** local functions ********** */

/* Converts a null terminated string to upper case */
static char *convert2upper(char *str)
{
	char *newstr, *p;

	p = newstr = strdup(str);
	do {
		*p = toupper(*p);
	}while (*p++);

	return newstr;
}

#if defined(CONFIG_CMD_UBI)
static int is_ubi(char *filename)
{
	int len = strlen(filename);

	if (len > 4) {
		if (!strcmp(filename + len - 4, ".ubi"))
			return 1;
	}

	return 0;
}
#endif

static size_t GetEraseSize( const nv_param_part_t* pPart )
{
        if( NULL == pPart )
                /* no specific sector, take the first one which might be wrong
                 * for NOR */
                return MtdGetEraseSize( 0, 0 );
        else
                return MtdGetEraseSize( pPart->uiChip, pPart->ullStart );
}

static size_t GetPageSize( const nv_param_part_t* pPart )
{
        size_t iPageSize;

#ifdef CONFIG_CMD_NAND
        iPageSize = nand_info[ 0 ].writesize;
#else
        /* NOR doesn't know anthing about pages */
        iPageSize = GetEraseSize( pPart );
#endif

        return iPageSize;
}

static int do_image_load(int iLoadAddr, image_source_e source,
			 char *loadcmd, char *part, char *img, int len,
			 const nv_param_part_t *pPartEntry)
{
	char cmd[PATH_MAXLEN];

	switch (source) {
	case IS_USB:
	case IS_MMC:
	case IS_HSMMC:
	case IS_SATA:
		sprintf(cmd, "%s %s 0x%x %s", loadcmd, part, iLoadAddr, img);
		if (len)
			sprintf(cmd, "%s %d", cmd, len);
		break;
	case IS_TFTP:
		sprintf(cmd, "tftp 0x%x %s", iLoadAddr, img);
		break;
	case IS_NFS:
		sprintf(cmd, "nfs 0x%x %s/%s", iLoadAddr, GetEnvVar(NPATH, 0), img);
                break;
        case IS_FLASH:
		return PartRead(pPartEntry, (void*)iLoadAddr, len, 1);
	case IS_RAM:
		/* Image is already in RAM */
		return 1;
	}

	return RunCmd(cmd);
}

static int do_digi_dboot(cmd_tbl_t* cmdtp, int flag, int argc, char * const argv[])
{
        int iLoadAddr = -1, iLoadAddrInitRD = 0, iLoad;
        int iLoadAddrFdt = -1;
        const part_t* pPart = NULL;
        const char*   szTmp           = NULL;
        const image_source_t* pImgSrc = NULL;
        const nv_param_part_t* pPartEntry = NULL, *pPartRootfs = NULL;
        const nv_param_part_t* pBootPart = NULL;
        const nv_param_part_t* pDeviceTreePart = NULL;
	char szCmd[ PATH_MAXLEN ]  = "";
	char szImg[ PATH_MAXLEN ]  = "";
	char szImgFdt[ PATH_MAXLEN ]  = "";
        char bIsNFSRoot   = 0;
        char bDHCPEnabled = 0;
	char bGotBootImage = 0;
	char kdevpart[8];
	char kfs[8];
	char loadcmd[20];
	char tmp[40];
	int dev = 0, ret, verify, iSize;
	image_header_t *pHeader;
	int nth_part = 0;	/* partition number to look from on WhatPart() calls */
	int hasfdt = 0;
#ifdef CONFIG_DUAL_BOOT
	int max_attempts = CONFIG_DUAL_BOOT_RETRIES;
	int i;
	int used_abbr_name = 0;
	int dualb_skip = 0;	/* Skip the dual boot functionality */
	int check_timestamp = 0;
	uint32_t timestamp[2] = {0, 0};
	int tsi = 0;
#endif
	int initrdSize = 0;

        if( ( argc < 2 ) || ( argc > 8 ) )
                return CMD_RET_USAGE;

        clear_ctrlc();

        /* check what to boot */
        if( argc > 2 )
                pImgSrc = ( ( NVOS_EBOOT != pPart->eOSType ) ?
                            WhatImageSource( argv[ 2 ] ) :
                            &l_axImgSrc[ IS_FLASH ] );
        else
                pImgSrc = &l_axImgSrc[ IS_TFTP ];

        if( NULL == pImgSrc )
                return CMD_RET_USAGE;

#ifdef CONFIG_DUAL_BOOT
_getpart:
        if (IS_FLASH == pImgSrc->eType) {
		/* get max number of boot retries */
		max_attempts = simple_strtol(GetEnvVar("dualb_retries", 1), NULL, 10);
		if (max_attempts <= 0) {
			DUALB_PRINT("Invalid value of 'dualb_retries'\n");
			max_attempts = CONFIG_DUAL_BOOT_RETRIES;
			DUALB_PRINT("Setting boot retries to default: %d\n",
				    max_attempts);
		}

		/* read dual_boot section from NVRAM */
		if (!NvPrivDualBootRead(&dualb)) {
			DUALB_PRINT("Error reading dual boot information. Defaults will be written to flash.\n");
			DualBootReset(&dualb);
			/* There was an error reading dual boot data.
			 * Use default values and write them to NVRAM */
			if (!NvPrivDualBootWrite(&dualb)) {
				DUALB_PRINT("Dual boot data could not be initialized\n");
			}
			/* reset boot attempts */
			dualb_reset_bootattempts();
#ifdef CONFIG_DUAL_BOOT_MODE_PEER
			/* If in PEER mode, boot the image with the latest timestamp */
			DUALB_PRINT("Checking timestamp of firmware images (newest one will be booted)...\n");
			check_timestamp = 1;
#endif
		}
		/* save the original dualb_data, to compare later
		 * if it has been changed during the process.
		 */
		memcpy(&original_dualb, &dualb, sizeof(dualb));

		/* Check if an abbreviated name was used instead of
		 * a particular partition name.
		 */
		for (i=0; i < ARRAY_SIZE(l_axPart); i++)
			if (!strcmp(argv[1], l_axPart[i].szName))
				used_abbr_name = 1;

		if (used_abbr_name) {
			/* The current boot partition should be the most recent, too
			 * (unless it was marked as not valid) */
			if ((dualb_data->last_updated != dualb_data->boot_part) &&
			    dualb_data->avail[dualb_data->last_updated]){
#if defined(CONFIG_DUAL_BOOT_MODE_PEER)
				/* It is not the most recent. Change the partition
				 * to boot to match the most recent and reset
				 * boot attempts counter */
				dualb_data->boot_part = dualb_data->last_updated;
				dualb_reset_bootattempts();
#elif defined(CONFIG_DUAL_BOOT_MODE_RESCUE)
				/* only if the last updated was the primary */
				if (0 == dualb_data->last_updated) {
					dualb_data->boot_part = dualb_data->last_updated;
					dualb_reset_bootattempts();
				}
#endif
			}

#ifdef CONFIG_DUAL_BOOT_STARTUP_GUARANTEE_AFTER_UPDATE
			if (dualb_data->verified[dualb_data->boot_part]) {
				dualb_skip = 1;
			}
			else
#endif
			{
				/* read the number of boot attempts done already */
				if (dualb_get_bootattempts() >= max_attempts) {
					DUALB_PRINT("Reached max number of attempts (%d)\n",
						    max_attempts);
					/* Reached max number of attempts. Try alternate system
					 * if there is one available. Otherwise, reset the counter
					 * and keep booting this system FOREVER */
					if (dualb_data->avail[!dualb_data->boot_part]) {
						DUALB_PRINT("Trying alternate system...\n");
						if (dualb_toggle()) {
							/* Call per platform hook when no system is bootable */
							dualb_nosystem_panic();
							dualb_skip = 1;
							goto error;
						}
					}
					else {
#ifdef CONFIG_DUAL_BOOT_RETRY_FOREVER
						DUALB_PRINT("No alternate system available. Keep trying this one...\n");
#else
						dualb_nosystem_panic();
						dualb_skip = 1;
						goto error;
#endif
					}
					/* Whether toggling the partition or staying in this
					 * one FOREVER, reset the attempts counter */
					dualb_reset_bootattempts();
				}

				DUALB_PRINT("Boot attempt %d/%d\n", dualb_get_bootattempts() + 1, max_attempts);
			}
			nth_part = dualb_data->boot_part;
		}
		else {
			/* A specific partition name was provided.
			 * Go on and boot this partition but do not
			 * start the watchdog or increment the
			 * boot attempts (skip dual boot functionality) */
			dualb_skip = 1;
		} /* !used_abbr_name */
	}
#endif /* CONFIG_DUAL_BOOT */

        /* determine OS and/or partition */
        CE( WhatPart( argv[ 1 ], 1, &pPart, &pPartEntry, 0, &eOSType, nth_part ) );
        pBootPart = pPartEntry; /* pPartEntry will be used in other calls and
				 * may be reset */

        if( NVOS_NONE == eOSType ) {
                eprintf( "OS Type not detected for %s\n", argv[ 1 ] );
                goto error;
        }

        /* determine file to boot */
        if (IS_RAM == pImgSrc->eType) {
		/* command contains a download address */
		if (argc >= 4) {
			iLoadAddr = get_input(argv[3]);
			if( iLoadAddr == -1 )
			{
				eprintf( "Invalid kernel address\n" );
				goto error;
			}
		}
		else if ( argc != 3 )
			return CMD_RET_USAGE;
        }
        else if ((IS_USB == pImgSrc->eType ||
		  IS_MMC == pImgSrc->eType ||
		  IS_HSMMC == pImgSrc->eType ||
		  IS_SATA == pImgSrc->eType) && (argc >= 6)) {
			SAFE_STRCAT(szImg, argv[5]);	/* kernel image filename */
			if (argc >= 7)
				SAFE_STRCAT(szImgFdt, argv[6]);	/* DTB image filename */
        }
	else if ((IS_TFTP == pImgSrc->eType ||
		  IS_NFS  == pImgSrc->eType) && (argc >= 4)) {
			SAFE_STRCAT(szImg, argv[3]);	/* kernel image filename */
			if (argc >= 5)
				SAFE_STRCAT(szImgFdt, argv[4]);	/* DTB image filename */
	}
	else {
		if( NULL != pPart ) {
			/* filename not present, but we have a partition definition */
			szTmp = GetEnvVar( pPart->szEnvVar, 0 );
			CE( NULL != szTmp );
		} else {
			eprintf( "Filename required\n" );
			goto error;
		}
		SAFE_STRCAT(szImg, szTmp);	/* kernel image filename */
	}

        CE( GetDHCPEnabled( &bDHCPEnabled ) );

	verify = getenv_yesno ("verify");

        /* user input processed, determine addresses */
	if ( iLoadAddr == -1 )
	{
		switch( eOSType ) {
		case NVOS_LINUX:
		case NVOS_ANDROID:
			CE(GetIntFromEnvVar(&iLoadAddr, "loadaddr", 0));
			GetIntFromEnvVar(&iLoadAddrFdt, "fdtaddr", 0);
			break;
		default:
			(void) GetIntFromEnvVar( &iLoadAddr, "loadaddr", 1 );
			eprintf( "Operating system not supported\n" );
		}

		if( -1 == iLoadAddr ) {
			eprintf( "variable loadaddr does not exist\n" );
			goto error;
		}
	}

	if( bDHCPEnabled &&
	    (( IS_TFTP == pImgSrc->eType ) || ( IS_NFS == pImgSrc->eType )) )
		/* makes no sense for USB or FLASH download to need DHCP */
		CE( DoDHCP() );

	if (pImgSrc->eType == IS_USB || pImgSrc->eType == IS_MMC || pImgSrc->eType == IS_HSMMC ||
		pImgSrc->eType == IS_SATA) {
		if (argc >= 4)
			strncpy(kdevpart, argv[3], ARRAY_SIZE(kdevpart));
		else
			strncpy(kdevpart, DEFAULT_KERNEL_DEVPART, ARRAY_SIZE(kdevpart));
		kdevpart[ARRAY_SIZE(kdevpart) - 1] = 0;	/* zero-terminate string */

		dev = (int)simple_strtoul(kdevpart, NULL, 16);

		if (argc >= 5)
			strncpy(kfs, argv[4], ARRAY_SIZE(kfs));
		else
			strncpy(kfs, DEFAULT_KERNEL_FS, ARRAY_SIZE(kfs));
		kfs[ARRAY_SIZE(kfs) - 1] = 0;	/* zero-terminate string */

		if (!strcmp(kfs, "fat") || !strcmp(kfs, "vfat"))
			strcpy(loadcmd, "fatload ");
		else if (!strcmp(kfs, "ext2") || !strcmp(kfs, "ext3"))
			strcpy(loadcmd, "ext2load ");
		else {
			printf("error: invalid value for filesystem (must be 'fat', 'vfat', 'ext2', 'ext3')\n");
			goto error;
		}
	}

	/* Preload actions */
	switch (pImgSrc->eType) {
	case IS_USB:
		CE(RunCmd("usb reset"));
		SAFE_STRCAT(loadcmd, "usb ");
		break;
	case IS_MMC:
		sprintf(tmp, "mmc rescan %d", dev);
		CE(RunCmd(tmp));
		SAFE_STRCAT(loadcmd, "mmc ");
		break;
	case IS_HSMMC:
		sprintf(tmp, "mmc rescan %d", dev);
		CE(RunCmd(tmp));
		SAFE_STRCAT(loadcmd, "hsmmc ");
		break;
	case IS_SATA:
		CE(RunCmd("sata init"));
		SAFE_STRCAT(loadcmd, "sata ");
		break;
	case IS_FLASH:
		if (NULL == pBootPart) {
			eprintf("No partition to boot from\n");
			goto error;
		}
		break;
	case IS_RAM:
		/* If additional address is provided we asume
		 * we are booting an initrd image. Otherwise
		 * we asume we are booting an initramfs image
		 * that contains both the kernel and rootfs
		 */
		if ( (argc == 5) || (argc == 6) )
		{
			iLoadAddrInitRD = get_input(argv[4]);
			if( iLoadAddrInitRD == -1 )
			{
				eprintf( "Invalid rootfs address\n" );
				goto error;
			}
		}
		break;
	case IS_TFTP:
	case IS_NFS:
		bIsNFSRoot = 1;
		break;
	}

	iLoad = iLoadAddr;
	pHeader = (image_header_t *)iLoadAddr;
	/* Load only 512 bytes, whenever possible */
	ret = do_image_load(iLoad, pImgSrc->eType, loadcmd, kdevpart, szImg, 512, pBootPart);
	if (ret && check_image_header((ulong)iLoad)) {
		/* Load the full image, if not already done... */
		if (IS_RAM == pImgSrc->eType) {
			bGotBootImage = 1;
		}
		else {
			if (!bIsNFSRoot) {
				iSize = ntohl(pHeader->ih_size) + sizeof(image_header_t);
				/* If the image is uncompressed, put it directly
				 * on the expected load address specified in the
				 * image header, to skip relocation during bootm.
				 * NOTE: This does not work with mx5 USB driver
				 * because it seems to require an address aligned
				 * in a 4K boundary.
				 */
				if (IS_USB != pImgSrc->eType) {
					iLoad = (pHeader->ih_comp == IH_COMP_GZIP) ? iLoadAddr :
										     (ntohl(pHeader->ih_load) - sizeof(image_header_t));
				}
				ret = do_image_load(iLoad, pImgSrc->eType, loadcmd, kdevpart, szImg, iSize, pBootPart);
			}
			if (ret) {
				bGotBootImage = verify ? VerifyCRC((uchar*)(iLoad + sizeof(image_header_t)),
						ntohl(pHeader->ih_size), ntohl(pHeader->ih_dcrc)) : 1;
			}
		}
	}

	/* Try to load Device Tree Blob file */
	if (NVOS_LINUX == eOSType || NVOS_ANDROID == eOSType) {
		switch(pImgSrc->eType) {
		case IS_FLASH:
			/* Determine DeviceTree partition */
			CE(WhatPart("fdt", 0, &pPart, &pDeviceTreePart, 0, NULL, 0));
			if (NULL == pDeviceTreePart) {
				printf("No Device Tree partition found\n");
				goto error;
			}
			/* Size parameter is only needed for FLASH media.
			 * A DTB file is usually small, so let's read a
			 * big enough amount of bytes.
			 */
			ret = do_image_load(iLoadAddrFdt, pImgSrc->eType,
					    NULL, NULL, NULL,
					    CONFIG_FDT_MAXSIZE,
					    pDeviceTreePart);
			break;
		case IS_RAM:
			/* do nothing. FDT is already in RAM */
			ret = 1;
			break;
		default:
			/* Device Tree blob in external media */
			if (!strcmp(szImgFdt, "")) {
				/* Use default filename if none provided */
				szTmp = GetEnvVar("fdtimg", 0);
				if (NULL != szTmp)
					/* DTB image filename */
					SAFE_STRCAT(szImgFdt, szTmp);
				else
					goto error;
			}
			/* Don't need to provide file size */
			ret = do_image_load(iLoadAddrFdt, pImgSrc->eType,
					    loadcmd, kdevpart, szImgFdt,
					    0, pDeviceTreePart);
			break;
		}
		if (ret) {
			hasfdt = 1;	/* DTB is now available in RAM */
#if defined(CONFIG_OF_LIBFDT)
			/*
			 * The following command will set the working FDT addr
			 * and set its size to a value large enough that it
			 * allows us to introduce some modifications.
			 */
			sprintf(tmp, "fdt addr %x %x", iLoadAddrFdt,
				CONFIG_FDT_MAXSIZE);
			if (!RunCmd(tmp))
				eprintf("Could not set the Device Tree address!\n");
#endif /* CONFIG_OF_LIBFDT */
		}
	}

	if (!bGotBootImage) {
#ifndef CONFIG_DBOOT_LECAGY_WINCE_IMAGES
		eprintf("incorrect image (no header or corrupted)\n");
#ifdef CONFIG_DUAL_BOOT
		/* Invalid system. To account for potential transient
		 * NAND read problems, increment the bootattempts counter and
		 * reset the board before trying the alternate system.
		 * One iteration before reaching the retries limit, switch to
		 * the alternate partition or else call the panic hook.
		 */
		dualb_increment_bootattempts();
		if (dualb_get_bootattempts() < max_attempts) {
			DUALB_PRINT("Bad image read (%d/%d).\n",
				    dualb_get_bootattempts(), max_attempts);
			DUALB_PRINT("Reset board to account for potential "
				    "transient read error...\n");
			do_reset(NULL, 0, 0, NULL);
		}
		else {
			/* Try alternate system
			 * if there is one available. Otherwise, call the panic
			 * action hook or reset the counter and keep booting this
			 * system FOREVER */
			if (dualb_data->avail[!dualb_data->boot_part]) {
				DUALB_PRINT("Trying alternate system...\n");
				if (dualb_toggle()) {
					/* Call per platform hook when no system is bootable */
					dualb_nosystem_panic();
					dualb_skip = 1;
					goto error;
				}
			}
			else {
#ifdef CONFIG_DUAL_BOOT_RETRY_FOREVER
				DUALB_PRINT("No alternate system available. Keep trying this one...\n");
#else
				dualb_nosystem_panic();
				dualb_skip = 1;
				goto error;
#endif /* CONFIG_DUAL_BOOT_RETRY_FOREVER */
			}
		}

		/* Successfully toggled to alternate partition
		 * (or keep booting the current one forever) */
		dualb_reset_bootattempts();
		goto _getpart;
#endif
		goto error;
#else
		CE( GetIntFromEnvVar( &iLoadAddr, "legacyloadaddr", 0 ) );

		/**
		  * Not valid images or corrupted... boot from primary partition for backward
		  * compatibility, when images have not header
		  */
		if (bIsNFSRoot) {
			if (!ret)	/* The load command failed, then skip... */
				goto error;
			/* For network booting, just relocate the image we already loaded */
			CE( GetIntFromEnvVar( &iSize, "filesize", 1) );
			memcpy((void *)iLoadAddr, (uchar *)iLoad, iSize);
		} else {
			ret = do_image_load(iLoadAddr, pImgSrc->eType, loadcmd, kdevpart, szImg, 0, pBootPart);
			if (!ret)
				goto error;
		}

		iLoad = iLoadAddr;
		pHeader = NULL;
		iSize = 0;
#endif
	}

#ifdef CONFIG_DUAL_BOOT
	if ((IS_FLASH == pImgSrc->eType) && !dualb_skip) {
		if (check_timestamp) {
			/* Save timestamp of image */
			if (bGotBootImage)
				timestamp[tsi] = ntohl(pHeader->ih_time);
			else
				timestamp[tsi] = 0;
			if (0 == tsi) {
				/* Read the other image */
				tsi++;
				nth_part = tsi;
				goto _getpart;
			}
			else if (1 == tsi) {
				/* Compare timestamps of images. If the current one
				 * is newer, go on, otherwise reread the first one. */
				check_timestamp = 0;
				if (timestamp[tsi] < timestamp[0]) {
					nth_part = 0;
					dualb_data->boot_part = 0;
					goto _getpart;
				}
				dualb_data->boot_part = tsi;
			}
		}
#if defined(CONFIG_DUAL_BOOT_STARTUP_GUARANTEE_PER_BOOT)
		/* 1.a. Increment the counter of boot attempts */
		dualb_increment_bootattempts();
#elif defined(CONFIG_DUAL_BOOT_STARTUP_GUARANTEE_AFTER_UPDATE)
		/* 1.b. Increment the counter of boot attempts
		 *      only if system partition was not verified
		 *      after update.
		 */
		if (!dualb_data->verified[(int)dualb_data->boot_part])
			dualb_increment_bootattempts();
#endif /* startup guarantee mode */
	}
#endif /* CONFIG_DUAL_BOOT */

	if (eOSType == NVOS_LINUX || NVOS_ANDROID == eOSType) {
		if (bGotBootImage) {
			/* Disable the verification (already done if enabled) */
			setenv("verify", "no");
		}
		/* And load initrd if neeeded... */
		/* !TODO. Should also be able to use uncompression and images with header */

		if (NvParamPartFind(&pPartEntry, NVPT_FILESYSTEM,
				    NVFS_INITRD, 1, 0)) {
			/* we have a bootable initrd, copy it, too */
			if (GetIntFromEnvVar(&iLoadAddrInitRD,
					     "loadaddr_initrd", 0)) {
				CE(PartRead(pPartEntry, (void*)iLoadAddrInitRD, 0, 0));
			}
		}
	}

        /* prepare for booting operating system */
        switch( eOSType ) {
		case NVOS_LINUX:
		case NVOS_ANDROID:
		{
			char szBootargs[ 2048 ];
			const char *vp;
			strcpy( szBootargs, "setenv bootargs " CONFIG_BOOTARGS);

			if(bIsNFSRoot ) {
				/* ip values must only be passed for auto IP configuration
				 * which is only needed if booting from NFS */
				SAFE_STRCAT(szBootargs, " ");
				SAFE_STRCAT(szBootargs, GetEnvVar( "ip", 0 ));
			}

			SAFE_STRCAT(szBootargs, " ");
			SAFE_STRCAT(szBootargs, GetEnvVar( "console", 0 ));

			SAFE_STRCAT(szBootargs, " ");
			if( bIsNFSRoot ) {
				SAFE_STRCAT(szBootargs, GetEnvVar( SNFS, 0 ));
				SAFE_STRCAT(szBootargs, GetEnvVar( NPATH, 0 ));
			} else {
				SAFE_STRCAT(szBootargs, GetEnvVar( SMTD, 0 ));
			}

			/*
			* When booting from USB or MMC the rootfs is expected in
			* a partition of that media. The partition number to use
			* can be given as a parameter to 'dboot' command, or else
			* must be stored in variable 'rpart', otherwise the
			* default partition is used (DEFAULT_ROOTFS_PARTITION)
			*/
			if (pImgSrc->eType == IS_MMC || pImgSrc->eType == IS_HSMMC || pImgSrc->eType == IS_USB ||
				pImgSrc->eType == IS_SATA) {
				SAFE_STRCAT(szBootargs, " root=");
				if (argc == 7) {
					SAFE_STRCAT(szBootargs, argv[6]);
				}
				else {
					if (pImgSrc->eType == IS_USB) {
						if ((vp = GetEnvVar("usb_rpart", 1)) != NULL)
							SAFE_STRCAT(szBootargs, vp);
						else
							SAFE_STRCAT(szBootargs, DEFAULT_ROOTFS_USB_PART);
					} else if (pImgSrc->eType == IS_SATA) {
						if ((vp = GetEnvVar("sata_rpart", 1)) != NULL)
							SAFE_STRCAT(szBootargs, vp);
						else
							SAFE_STRCAT(szBootargs, DEFAULT_ROOTFS_SATA_PART);
					} else {
						if ((vp = GetEnvVar("mmc_rpart", 1)) != NULL)
							SAFE_STRCAT(szBootargs, vp);
						else
							SAFE_STRCAT(szBootargs, DEFAULT_ROOTFS_MMC_PART);
					}
				}

				/*
				 * A delay is needed to allow the media to be properly initialized
				 * before being able to mount the rootfs
				 */
				SAFE_STRCAT(szBootargs, " rootdelay=");
				if ((vp = GetEnvVar("rootdelay", 1)) != NULL)
					SAFE_STRCAT(szBootargs, vp);
				else
					sprintf(szBootargs, "%s%d", szBootargs, ROOTFS_DELAY);
			} else if (pImgSrc->eType == IS_RAM) {
				/* Only append parameters if booting an initrd,
				* not if booting an initramfs.
				*/
				if (iLoadAddrInitRD) {
					SAFE_STRCAT(szBootargs, " root=/dev/ram");

					/* Get ramdisk size argument to pass it to the kernel */
					if ( argc == 6 ) {
						/* Ramdisk size is passed in KB (decimal) */
						initrdSize = simple_strtol(argv[5], NULL, 10);
						if( initrdSize == -1 )
						{
							eprintf( "Invalid initrd max size\n" );
							goto error;
						}

						sprintf(szBootargs, "%s ramdisk_size=%d", szBootargs, initrdSize);
					}
				}
			} else {
				if (pImgSrc->eType == IS_FLASH) {
					int boot_part = 0;

					/* rootfs partition may be passed as argument */
					if (argc == 4) {
						pPartRootfs = FindPartition(argv[3]);
						if (NULL != pPartRootfs) {
							CE(append_rootfs_parameters(szBootargs, sizeof(szBootargs), pPartRootfs));
						}
					}
					else {
#ifdef CONFIG_DUAL_BOOT
						boot_part = dualb_data->boot_part;
#endif
						SAFE_STRCAT(szBootargs, " ");
						CE(PartStrAppendRoot(szBootargs, sizeof(szBootargs), boot_part));
					}
				}
			}
			/* MTD partitions */
			SAFE_STRCAT(szBootargs, " ");
			CE( PartStrAppendParts( szBootargs, sizeof( szBootargs ) ) );

			if((vp = GetEnvVar( VIDEO_VAR, 1 )) != NULL) {
				SAFE_STRCAT(szBootargs, " "VIDEO_VAR"=");
				SAFE_STRCAT(szBootargs, vp);
			}
			if((vp = GetEnvVar( VIDEO2_VAR, 1 )) != NULL) {
				SAFE_STRCAT(szBootargs, " "VIDEO2_VAR"=");
				SAFE_STRCAT(szBootargs, vp);
			}
			if((vp = GetEnvVar( LDB_VAR, 1 )) != NULL) {
				SAFE_STRCAT(szBootargs, " "LDB_VAR"=");
				SAFE_STRCAT(szBootargs, vp);
			}
			if((vp = GetEnvVar( FBPRIMARY_VAR, 1 )) != NULL) {
				SAFE_STRCAT(szBootargs, " "FBPRIMARY_VAR"=");
				SAFE_STRCAT(szBootargs, vp);
			}
#ifdef CONFIG_COMPUTE_LPJ
			if ((vp = (char *)GetEnvVar(LPJ, 1)) != NULL) {
				SAFE_STRCAT(szBootargs, " lpj=");
				SAFE_STRCAT(szBootargs, vp);
			}
#endif

			/* android */
			if (NVOS_ANDROID == eOSType) {
				char console[20];

				SAFE_STRCAT(szBootargs, " androidboot.hardware=");
				SAFE_STRCAT(szBootargs, platformname);
				/* Get console out of the console variable, by searching for the
				* platform console name pattern
				*/
				vp = strstr(GetEnvVar("console", 0), CONFIG_CONSOLE);
				if (NULL != vp) {
					int i=0;

					strncpy(console, vp, sizeof(console));
					while (console[i] != 0 && console[i] != ',' &&
						i < sizeof(console))
						i++;
					console[i] = 0; /* trim string to match only the console */
					SAFE_STRCAT(szBootargs, " androidboot.console=");
					SAFE_STRCAT(szBootargs, console);
				}

				SAFE_STRCAT(szBootargs, " init=/init");

				/* Touch screen calibration */
				if (append_calibration()) {
					SAFE_STRCAT(szBootargs, " calibration");
				}
			}

			/* Append user defined cmdline via sdt_bootarg */
			SAFE_STRCAT(szBootargs, " ");
			SAFE_STRCAT(szBootargs, GetEnvVar("std_bootarg", 0 ));

			CE( RunCmd( szBootargs ) );
			if(iLoadAddrInitRD) {
				if (hasfdt && iLoadAddrFdt != -1)
					sprintf( szCmd, "bootm 0x%x 0x%x 0x%x", iLoad, iLoadAddrInitRD, iLoadAddrFdt);
				else
					sprintf( szCmd, "bootm 0x%x 0x%x", iLoad, iLoadAddrInitRD);
			}
			else{
				if (hasfdt && iLoadAddrFdt != -1)
					sprintf( szCmd, "bootm 0x%x - 0x%x", iLoad, iLoadAddrFdt);
				else
					sprintf( szCmd, "bootm 0x%x", iLoad);
			}
			break;
		}

		case NVOS_WINCE:
		case NVOS_EBOOT:
			/* will be handled after handling dualboot */
			break;

		case NVOS_NETOS:
			sprintf( szCmd, "bootm 0x%x", iLoadAddr );
			break;

		default:
			eprintf( "Operating system not supported\n" );
        } /* switch( eOSType ) */

#ifdef CONFIG_DUAL_BOOT
	/* Before giving control to the OS there are several tasks to do */
        if ((IS_FLASH == pImgSrc->eType) && !dualb_skip) {
		/*  1. Save dualb_data to NVRAM if it changed */
		CE(dualb_save());

		/*  2. Enable the watchdog timer */
		start_wdt();
        } /* is_flash && !dualb_skip */
#endif /* CONFIG_DUAL_BOOT */

	if (IS_FLASH == pImgSrc->eType)
		printf("Booting partition '%s'\n", pBootPart->szName);

	/* boot operating system */
	/* Run bootm command */
	return !RunCmd( szCmd );

error:
#ifdef CONFIG_DUAL_BOOT
	/* In case of error during dual boot, save dual_boot data and start the watchdog */
	if ((IS_FLASH == pImgSrc->eType) && !dualb_skip) {
		/*  1. Save dualb_data to NVRAM if it changed */
		dualb_save();
		/*  2. Enable the watchdog timer */
		start_wdt();
	}
#endif
        return 1;
}

/*
 * Puts a specified file from a specified media into RAM
 */
static int put_file_into_ram(image_source_e media,
			     int iLoadAddr,
			     char *szImg,
			     char *kdevpart,
			     char *kfs)
{
	char szCmd[ PATH_MAXLEN ] = "";

	switch (media) {
#ifdef CONFIG_CMD_NET
	case IS_TFTP:
		sprintf( szCmd, "tftp 0x%x %s", iLoadAddr, szImg );
		CEN( RunCmd( szCmd ) );
		break;

	case IS_NFS:
		sprintf( szCmd, "nfs 0x%x %s/%s",
			iLoadAddr, GetEnvVar( NPATH, 0 ), szImg );
		CEN( RunCmd( szCmd ) );
		break;
#endif

#ifdef CONFIG_CMD_USB
	case IS_USB:
		if (!strcmp(kfs, "fat") || !strcmp(kfs, "vfat"))
			sprintf(szCmd, "fatload usb %s 0x%x %s", kdevpart, iLoadAddr, szImg);
		else if (!strcmp(kfs, "ext2") || !strcmp(kfs, "ext3"))
			sprintf(szCmd, "ext2load usb %s 0x%x %s", kdevpart, iLoadAddr, szImg);
		else {
			printf("error: invalid value for filesystem (must be 'fat', 'vfat', 'ext2', 'ext3'\n");
			goto error;
		}

		CE(RunCmd("usb reset"));
		CE(RunCmd(szCmd));
		break;
#endif

#ifdef CONFIG_CMD_MMC
	case IS_MMC:
	case IS_HSMMC:
	{
		int dev=0;

		dev = (int)simple_strtoul (kdevpart, NULL, 16);
		sprintf(szCmd, "mmc rescan %d", dev);
		CE(RunCmd(szCmd));

		if (!strcmp(kfs, "fat") || !strcmp(kfs, "vfat"))
			sprintf(szCmd, "fatload %s %s 0x%x %s",
				(IS_HSMMC == media) ? "hsmmc" : "mmc",
				kdevpart, iLoadAddr, szImg);
		else if (!strcmp(kfs, "ext2") || !strcmp(kfs, "ext3"))
			sprintf(szCmd, "ext2load %s %s 0x%x %s",
				(IS_HSMMC == media) ? "hsmmc" : "mmc",
				kdevpart, iLoadAddr, szImg);
		else {
			printf("error: invalid value for filesystem (must be 'fat', 'vfat', 'ext2', 'ext3'\n");
			goto error;
		}
		CE(RunCmd(szCmd));
		break;
	}
#endif

#ifdef CONFIG_CMD_SATA
	case IS_SATA:
	{
		if (!strcmp(kfs, "fat") || !strcmp(kfs, "vfat"))
			sprintf(szCmd, "fatload sata %s 0x%x %s", kdevpart, iLoadAddr, szImg);
		else if (!strcmp(kfs, "ext2") || !strcmp(kfs, "ext3"))
			sprintf(szCmd, "ext2load sata %s 0x%x %s", kdevpart, iLoadAddr, szImg);
		else {
			printf("error: invalid value for filesystem (must be 'fat', 'vfat', 'ext2', 'ext3'\n");
			goto error;
		}

		CE(RunCmd("sata init"));
		CE(RunCmd(szCmd));
		break;
	}
#endif

	case IS_FLASH:
	default:
		/* makes really no sense in the update func */
		goto error;
	}

	return 1;

error:
	return 0;
}

static int do_digi_update(cmd_tbl_t* cmdtp, int flag, int argc, char * const argv[])
{
        int iLoadAddr  = -1;
#ifdef CONFIG_TFTP_UPDATE_ONTHEFLY
        const char* szUpdateOnTheFly = NULL;
        int otf_disabled = 0;
#endif
        const image_source_t* pImgSrc = NULL;
        const part_t* pPart           = NULL;
        const char*   szTmp           = NULL;
        const nv_param_part_t* pPartEntry = NULL;
        int iFileSize  = 0;
        int iCRCSize   = 0;

#ifdef CONFIG_TFTP_UPDATE_ONTHEFLY
        /* Reset flags on every update */
        bTftpToFlashStatus = 0;
        /* Check if update should be on the fly */
        szUpdateOnTheFly = GetEnvVar( "otf-update", 1 );
        if (!strcmp(szUpdateOnTheFly, "on") ||
            !strcmp(szUpdateOnTheFly, "yes") ||
            !strcmp(szUpdateOnTheFly, "1")) {
		/* Signaling flag (also for for tftp.c) to write directly to flash */
		bTftpToFlashStatus |= B_WRITE_IMG_TO_FLASH;
        }
        else if (!strcmp(szUpdateOnTheFly, "off") ||
            !strcmp(szUpdateOnTheFly, "no") ||
            !strcmp(szUpdateOnTheFly, "0")) {
		/* Manually forced to disabled */
		otf_disabled = 1;
        }
#endif

        crc32_t uiCRC;
#if defined(CONFIG_CMD_UBI) || defined(CONFIG_TFTP_UPDATE_ONTHEFLY)
        char szCmd[ PATH_MAXLEN ] = "";
#endif
	char szImg[ PATH_MAXLEN ] = "";
        char bDHCPEnabled = 0;
        static const char* szUpdating = "Updating";
#ifdef CFG_HAS_WIRELESS
	wcd_data_t *pWCal;
#endif
	char kdevpart[8];
	char kfs[8];
	int nth_part = 0;	/* partition number to look from on WhatPart() calls */
#ifdef CONFIG_DUAL_BOOT
	int i;
	int used_abbr_name = 0;
#endif
	long sizeTmp;

        if( ( argc < 2 ) || ( argc > 6 ) )
                return CMD_RET_USAGE;

        clear_ctrlc();

        if( argc >= 3 )
                pImgSrc = WhatImageSource( argv[ 2 ] );
        else
                pImgSrc = &l_axImgSrc[ IS_TFTP ];

        if( NULL == pImgSrc )
                return CMD_RET_USAGE;

#ifdef CONFIG_DUAL_BOOT
        DualBootReset(&dualb);

	/* Check if an abbreviated name was used instead of
	 * a particular partition name.
	 */
	for (i=0; i < ARRAY_SIZE(l_axPart); i++) {
		if (!strcmp(argv[1], l_axPart[i].szName)) {
			used_abbr_name = 1;
		}
	}

	/* read dual_boot section from NVRAM */
	if (!NvPrivDualBootRead(&dualb)) {
		/* There was an error reading dual boot data.
		 * Use default values and write them to NVRAM */
		if (!NvPrivDualBootWrite(&dualb)) {
			DUALB_PRINT("Dual boot data could not be initialized\n");
		}
		/* reset boot attempts */
		dualb_reset_bootattempts();
	}
	/* save the original dualb_data, to compare later
	 * if it has been changed during the process.
	 */
	memcpy(&original_dualb, &dualb, sizeof(dualb));

	/* On rescue mode, if a partition name is not given, updates
	 * occur to the primary partition, but on peer mode, updates
	 * occur to the alternate partition.
	 */
#if defined(CONFIG_DUAL_BOOT_MODE_PEER)
	if (used_abbr_name) {
		/* Are we updating a system partition? */
	        /* do a first pass to get the first partition with that
	         * associated abbreviated name to get the type */
	        CE( WhatPart(argv[1], 0, &pPart, &pPartEntry, 1, NULL, 0));

	        if (is_system_partition(pPartEntry)) {
			/* On peer mode, updates always occur to the alternate partition */
			nth_part = !dualb_data->boot_part;
		}
	}
#endif /* CONFIG_DUAL_BOOT_MODE_PEER */
#endif /* CONFIG_DUAL_BOOT */

        /* find the partition to update */
        CE( WhatPart( argv[ 1 ], 0, &pPart, &pPartEntry, 1, &eOSType, nth_part ) );

	if( pPartEntry->flags.bReadOnly || pPartEntry->flags.bFixed ) {
		/* U-Boot is always marked read-only, but has been checked
		 * above already  */
		CE( WaitForYesPressed( "Partition marked read-only / fixed."
			" Do you want to continue?", szUpdating ) );
	}

        /* determine file to update */
	if ( pImgSrc->eType != IS_RAM )
	{
		if( argc == 4 ){
			szTmp = argv[ 3 ];
		} else if (argc == 6) {
			szTmp = argv[5];
		} else if( NULL != pPart ) {
			/* not present, but we have a partition definition */
			szTmp = GetEnvVar( pPart->szEnvVar, 0 );
			CE( NULL != szTmp );
		} else {
			eprintf( "Require filename\n" );
			goto error;
		}
		SAFE_STRCAT(szImg, szTmp);
	}

        CE( GetDHCPEnabled( &bDHCPEnabled ) );

#if defined(CONFIG_CMD_UBI)
	/* Recreate UBI partition, if the "modified flag" is set */
	if( ( NVPT_FILESYSTEM == pPartEntry->eType ) &&
            ( NVFS_UBIFS      == pPartEntry->flags.fs.eType ) &&
            (!is_ubi(szImg))) {
	        /* Disable UBI messages */
		sprintf( szCmd, "ubi silent 1" );
		CEN( RunCmd(szCmd) );

		CE( RecreateUBIVolume( pPartEntry ) );
		printf( "UBI volume ready\n\n" );
	}
#endif
        /* user input processed, determine load addresses */
	CE(GetIntFromEnvVar(&iLoadAddr, "loadaddr", 0));

        /* we require it being set from download tool */
        setenv( "filesize", "" );

	if( bDHCPEnabled &&
	    (( IS_TFTP == pImgSrc->eType ) || ( IS_NFS == pImgSrc->eType )) ){
		/* makes no sense for USB or FLASH download to need DHCP */
		CE( DoDHCP() );
	}

	if (pImgSrc->eType == IS_USB || pImgSrc->eType == IS_MMC ||
	    pImgSrc->eType == IS_HSMMC || pImgSrc->eType == IS_SATA) {
		/* kernel device:partition can be given as argument
		* Otherwise, default value is used */
		if (argc >= 4)
			strncpy(kdevpart, argv[3], ARRAY_SIZE(kdevpart));
		else
			strncpy(kdevpart, DEFAULT_KERNEL_DEVPART, ARRAY_SIZE(kdevpart));
		kdevpart[ARRAY_SIZE(kdevpart) - 1] = 0;	/* zero-terminate string */

		/* Kernel file system can be given as argument
		* Otherwise, default value is used */
		if (argc >= 5)
			strncpy(kfs, argv[4], ARRAY_SIZE(kfs));
		else
			strncpy(kfs, DEFAULT_KERNEL_FS, ARRAY_SIZE(kfs));
		kfs[ARRAY_SIZE(kfs) - 1] = 0;	/* zero-terminate string */
	}

#ifdef CONFIG_TFTP_UPDATE_ONTHEFLY
	if (IS_TFTP == pImgSrc->eType) {
		/* If updating a partition that is larger than the available
		 * RAM for a TFTP transfer turn on on-the-fly update by default
		 * unless manually forced to disabled.
		 * The reason is that the expected size of the firmware image
		 * (notice the size is unknown until the TFTP transfer
		 * completes) would not fit into RAM and could overwrite U-Boot
		 * or exceed the available RAM memory.
		 */
		unsigned int avail = get_tftp_available_ram(iLoadAddr);
		unsigned int part_size = (unsigned int)PartSize(pPartEntry);

		if (part_size > avail && !otf_disabled) {
			printf("Partition to update is larger than the RAM "
			       "available to do TFTP transfers (%d MiB).\n",
			       avail / (1024 * 1024));
			printf("Activating On-the-fly update mechanism.\n");
			bTftpToFlashStatus |= B_WRITE_IMG_TO_FLASH;
		}

		/* If updating U-Boot turn off on-the-fly update by default.
		 * The reason is that U-Boot is a critical partition and using OTF mechanism
		 * would first erase it before transferring the image. In case of a network
		 * cut or failure, U-Boot would be erased but not updated, leaving a non bootable
		 * system. Besides, U-Boot is a small partition and OTF is not needed.
		 * U-Boot bootstream images cannot be updated using OTF either because
		 * they must be updated with a different mechanism than RAW write.
		 */
		if (((bTftpToFlashStatus & B_WRITE_IMG_TO_FLASH) == B_WRITE_IMG_TO_FLASH) &&
		    ((pPart->ePartType == NVPT_UBOOT) || (NVPT_BOOTSTREAM == pPart->ePartType))) {
			printf("Deactivating on-the-fly TFTP update due to critical partition\n");
			bTftpToFlashStatus &= ~B_WRITE_IMG_TO_FLASH;
		}

		if ((bTftpToFlashStatus & B_WRITE_IMG_TO_FLASH) == B_WRITE_IMG_TO_FLASH) {
			/* Do the OTF-Update */

			/* Get/Set EraseSize and PageSize of flash for calculation in TftpHandler()*/
			iFlashEraseSize = GetEraseSize( pPartEntry );
			iFlashPageSize = GetPageSize( pPartEntry );

			if (NVPT_FILESYSTEM == pPartEntry->eType) {
				if( (NVFS_JFFS2 == pPartEntry->flags.fs.eType) &&
					((iFlashPageSize - (iFlashEraseSize % iFlashPageSize)) >= sizeof(struct jffs2_unknown_node)) ) {
					/* this is an JFFS2 partition. Padding differs for JFFS2.
					* Therefore we want to remember the type of partition for padding in tftp.c*/
					bTftpToFlashStatus |= B_PARTITION_IS_JFFS2;
				}
#if defined(CONFIG_CMD_UBI)
				else if ( (NVFS_UBIFS == pPartEntry->flags.fs.eType) &&
					  !is_ubi(szImg) ){
					/* this is a UBIFS partition.
					 * The write function differs for this FS type. */
					bTftpToFlashStatus |= B_PARTITION_IS_UBIFS;
					iFlashEraseSize = ubi_volume_get_leb_size((char *)pPartEntry->szName);
					/* iFlashPageSize is not used in this case */
				}
#endif
			}

			/* Set start address and size of partition for TftpHandler */
			iPartitionStartAdress = pPartEntry->ullStart;
			iPartitionSize = pPartEntry->ullSize;

			switch( pPartEntry->eType ) {
			case NVPT_UBOOT:
				CE( WaitForYesPressed( "Do you really want to overwrite U-Boot flash partition", szUpdating ) );
				break;
			default: break;  /* avoid compiler warnings */
			}

			if ( !(bTftpToFlashStatus & B_PARTITION_IS_UBIFS) ) {
				printf("Erasing partition '%s'.\n", pPartEntry->szName);
				/* erase complete partition */
				CE( PartErase( pPartEntry ) );
			}

			/* writing and verifying is now handled by TftpHandler() */
			printf("Updating partition '%s'. On-the-fly update is ON!\n", pPartEntry->szName);
			/* Give partition pointer to TftpHandler() */
			pPartToWrite = pPartEntry;

			/* Call standard TFTP function, but we have set flag B_WRITE_IMG_TO_FLASH == TRUE */
			sprintf( szCmd, "tftp 0x%x %s", iLoadAddr, szImg );
			CEN( RunCmd( szCmd ) );

			/* should be set by download tool */
			CE( GetIntFromEnvVar( &iFileSize, "filesize", 0 ) );

#if defined(CONFIG_CMD_UBI)
			/* Enable UBI messages */
			if( ( NVPT_FILESYSTEM == pPartEntry->eType ) &&
				( NVFS_UBIFS == pPartEntry->flags.fs.eType ) ) {
				/* Enable UBI messages */
				sprintf( szCmd, "ubi silent 0" );
				CEN( RunCmd(szCmd) );
			}
#endif

			goto done;
		}
	}
#endif /* CONFIG_TFTP_UPDATE_ONTHEFLY */

	if (IS_RAM == pImgSrc->eType) {
		/* command requires a download address and a size */
		if ( argc < 5 )
		{
			eprintf( "Missing arguments: update <os> ram <load_address> <size>\n" );
			goto error;
		}

		iLoadAddr = get_input(argv[3]);
		if( iLoadAddr == -1 )
		{
			eprintf( "Invalid image address\n" );
			goto error;
		}

		sizeTmp = get_input(argv[4]);
		if ( sizeTmp == -1 )
		{
			eprintf( "Invalid image size\n" );
			goto error;
		}
		sprintf(szImg, "%lX", (ulong)sizeTmp);
		setenv("filesize", szImg);
	}
	else {
		/* put file into RAM */
		CEN(put_file_into_ram(pImgSrc->eType,
				      iLoadAddr,
				      szImg,
				      kdevpart,
				      kfs));
	}

        /* should be set by download tool */
        CE( GetIntFromEnvVar( &iFileSize, "filesize", 0 ) );


#ifdef CONFIG_CMD_BOOTSTREAM
        /* update of bootstream image is handled differently */
	if (NVPT_BOOTSTREAM == pPartEntry->eType) {
		return(write_bootstream(pPartEntry, iLoadAddr, iFileSize));
	}
#endif

	/* TODO:
	 * crc32() must be adjusted to read sector by sector
	 * to calculate a check sum out of flash for OTF-Update
	 */

	/* do crc32 calculation */
        iCRCSize = iFileSize - ( (NULL != pPart && pPart->bCRC32Appended) ?
		   sizeof( uiCRC ) : 0 );
	if (iCRCSize <= 0) {
		printf("Partition requires checksum but provided file is to small \
			to contain a checksum\n");
		goto error;
	}
        uiCRC = crc32( 0, (uchar*) iLoadAddr, iCRCSize );
        printf( "Calculated checksum = 0x%x\n", (uint32_t)uiCRC );

        if (NULL != pPart && pPart->bCRC32Appended) {
                /* check CRC32 in File */
                crc32_t uiCRCFile;
                /* works independent whether the CRC is aligned or not. We
                   * don't know what the image does. */
                memcpy( &uiCRCFile, (const crc32_t*) ( iLoadAddr + iCRCSize ),
                        sizeof( uiCRCFile ) );
                if( uiCRCFile != uiCRC ) {
                        eprintf( "CRC32 mismatch: Image reports 0x%0x - ",
                                 (uint32_t)uiCRCFile );
                        CE( WaitForYesPressed( "Continue", szUpdating ) );
                }
        }

        /* run some checks based on the image */

        switch( pPartEntry->eType ) {
            case NVPT_UBOOT:
                CE( WaitForYesPressed( "Do you really want to overwrite U-Boot flash partition", szUpdating ) );
#ifdef CONFIG_CCXMX53
		iFlashPageSize = GetPageSize( pPartEntry );
		if (iFlashPageSize == 4096) {
			u32 *pData = (u32 *)(iLoadAddr + 0x7c);

			/* Update the NAND flash configuration block for a 4K page nand.
			 * This is necessary when the bad block marker is swapped (swap enabled).
			 * The offsets that have to be updated are the bad block marker offset
			 * inside the data block (0x7c) and the offset in the spare area where the
			 * data byte was swapped (0xb0) */
			*pData = MX5X_SWAP_BI_MAIN_BB_POS_4K;
			pData = (u32 *)(iLoadAddr + 0xb0);
			*pData = 4096 + MX5X_SWAP_BI_SPARE_BB_POS_4K;
		}
#endif
                break;
            case NVPT_NVRAM:
#ifdef CFG_HAS_WIRELESS
                if( !strcmp( argv[ 1 ], "wifical" ) ) {
                        CE( WaitForYesPressed(
			    "Do you really want to update the Wireless Calibration Information", szUpdating ) );
                        pWCal = (wcd_data_t *)iLoadAddr;
                        if ( !NvPrivWCDSetInNvram( pWCal ) ) {
                                eprintf( "Invalid calibration data file\n" );
                                goto error;
                        }
                        saveenv();
                }
#endif
                goto done;
                break;
            case NVPT_FPGA:
            {
#ifdef CONFIG_FPGA_SIZE
                    if( CONFIG_FPGA_SIZE != ( iFileSize - 4 ) ) {
                            /* +4 because of checksum added */
                            eprintf( "Expecting FPGA to have size 0x%x\n",
                                     CONFIG_FPGA_SIZE );
                            goto error;
                    }

                    printf( "Updating FPGA firmware ...\n" );
                    if( LOAD_FPGA_FAIL == fpga_checkbitstream( (uchar*) iLoadAddr, iFileSize ) ) {
                            eprintf( "Updating FPGA firmware failed\n" );
                            goto error;
                    }

                    /* fpga_checkbitstream doesn't print \0 */
                    printf( "\n" );
                    CE( WaitForYesPressed( "Do you really want to overwrite FPGA firmware", szUpdating ) );
#else  /* CONFIG_FPGA_SIZE */
                    printf( "No FPGA available\n" );
#endif  /* CONFIG_FPGA_SIZE */
                    break;
            }
            default: break;  /* avoid compiler warnings */
        }

#ifdef CONFIG_PARTITION_SWAP
        if( ( NULL != pPart ) && pPart->bSwapEndian ) {
                /* swapping is done on 16/32bit. Ensure that there is space */
                int iFileSizeAligned = ( iFileSize + 0x3 ) & ~0x3;
                /* fill swapped areas with empty character */
                memset( (void*) iLoadAddr + iFileSize, 0xff, iFileSizeAligned - iFileSize );
                iFileSize = iFileSizeAligned;
        }
#endif

        /* update images */

        /* fit we into it? */
        if( PartSize( pPartEntry ) < iFileSize ) {
                /* 0 means to end of flash */
                eprintf( "Partition too small\n" );
                goto error;
        }

        CE( PartProtect( pPartEntry, 0 ) );

#if defined(CONFIG_CMD_UBI)
        if( ( NVPT_FILESYSTEM == pPartEntry->eType ) &&
            ( NVFS_UBIFS      == pPartEntry->flags.fs.eType ) &&
            ( !is_ubi(szImg))) {
                printf( "\nWriting UBI image\n" );
		sprintf( szCmd, "ubi write %x %s %x", iLoadAddr, pPartEntry->szName, iFileSize );
		CEN( RunCmd(szCmd) );

		/* Enable UBI messages */
		sprintf( szCmd, "ubi silent 0" );
		CEN( RunCmd(szCmd) );

		/* Verify it */
		CE( !ubi_volume_verify((char *)pPartEntry->szName, (char *)iLoadAddr, 0, iFileSize, 0) );
	}
        else
#endif
	{
		printf("Updating partition '%s'\n", pPartEntry->szName);
		/* erase complete partition */
		CE( PartErase( pPartEntry ) );
		CE( AppendPadding( pPartEntry, (void*) iLoadAddr, &iFileSize ) );

#ifdef CONFIG_PARTITION_SWAP
		if( ( NULL != pPart ) && pPart->bSwapEndian )
			/* do it before writing. Swapping not supported for user
			* named partitions */
			CE( PartSwap( pPartEntry, (void*) iLoadAddr, iFileSize ) );
#endif

		/* write partition */
		CE( PartWrite( pPartEntry, 0, (void*) iLoadAddr, iFileSize, 0) );
		/* verify it */
		CE( PartVerify( pPartEntry, 0, (void*) iLoadAddr, iFileSize, 0) );

#if defined(CONFIG_CMD_NAND) && !defined(CONFIG_NAND_OBB_ATOMIC)
		if( pPartEntry->uiChip )
			/* !TODO */
			eprintf( "*** Chip %i not supported yet\n", pPartEntry->uiChip );

		if( ( NVPT_FILESYSTEM == pPartEntry->eType ) &&
		( NVFS_JFFS2      == pPartEntry->flags.fs.eType ) ) {
			printf( "Writing cleanmarkers\n" );
			CE( jffs2_mark_clean( pPartEntry->ullStart,
					pPartEntry->ullSize ) );
		}
#endif
	}

        if( pPartEntry->flags.bFixed || pPartEntry->flags.bReadOnly)
                CE( PartProtect( pPartEntry, 1 ) );

done:
        switch( pPartEntry->eType ) {
            case NVPT_FPGA:
            case NVPT_UBOOT:
                /* be user friendly. fpga is not loaded */
                printf( "Reboot system so update takes effect\n" );
                break;
            default:
                break;
        }

#ifdef CONFIG_DUAL_BOOT
	/* partition was updated, which one?
	 * If abbreviated name provided:
	 *   If in peer mode: the alternate one
	 *   If in rescue mode: the primary
	 * Else
	 *   Find if the updated partition is of system type
	 *   and if so, which index among the system partitions */
	if (used_abbr_name) {
#if defined(CONFIG_DUAL_BOOT_MODE_PEER)
		/* The last updated partition is the opposite
		 * of the current boot partition */
		dualb_data->last_updated = !dualb_data->boot_part;
#elif defined(CONFIG_DUAL_BOOT_MODE_RESCUE)
		/* In Rescue mode, only the primary partition is updated with abbr name */
		dualb_data->last_updated = 0;
#endif
		/* Mark it as available and updated */
		dualb_data->avail[(int)dualb_data->last_updated] = 1;
		dualb_data->verified[(int)dualb_data->last_updated] = 0;
		/* The system partition was updated so reset the boot attempts counter */
		dualb_reset_bootattempts();
	}
	else {
		/* A specific partition name was provided.
		 * Is it a system partition (OS or RootFS)?
		 */
		if (is_system_partition(pPartEntry)) {
			/* Get the index (last char in name)
			 */
			char index = argv[1][strlen(argv[1])-1];

			if (isdigit(index)) {
				/* Convert to digit */
				index -= '0';
			}
			else {
				/* Not a digit so it is not a dual_boot
				 * managed partition */
				goto success;
			}
			if (index == 0 || index == 1) {
				/* Mark it as available and updated */
				dualb_data->avail[(int)index] = 1;
				dualb_data->verified[(int)index] = 0;

				/* The system partition was updated */
				/* Set the last updated partition as this one */
				dualb_data->last_updated = index;
#if defined(CONFIG_DUAL_BOOT_MODE_PEER)
				/* reset the boot attempts counter */
				dualb_reset_bootattempts();
#elif defined(CONFIG_DUAL_BOOT_MODE_RESCUE)

				/* This will reset the attempts counter if
				 *  a) The primary system was updated
				 *  b) The rescue system was updated and the
				 *     device was trying to boot this one.
				 */
				if (dualb_data->boot_part == index)
					dualb_reset_bootattempts();
#endif /* CONFIG_DUAL_BOOT_MODE_? */
			} /* index 0 or 1 */
		} /* system partition */
	} /* !used_abbr_name */

success:
	/*  Save dualb_data to NVRAM if it changed */
	if (memcmp(&original_dualb, &dualb, sizeof(dualb))) {
		if (!NvPrivDualBootWrite(&dualb)) {
			eprintf("Dual boot data could not be saved!\n");
			goto error;
		}
		if (!CW(NvSave())) {
			eprintf("Dual boot data could not be saved!\n");
			goto error;
		}
	}
#endif /* CONFIG_DUAL_BOOT */

#ifdef CONFIG_TFTP_UPDATE_ONTHEFLY
	if (((IS_TFTP == pImgSrc->eType) &&
	     ((bTftpToFlashStatus & B_WRITE_IMG_TO_FLASH) == B_WRITE_IMG_TO_FLASH))) {
		printf( "On-the-fly Update successful\n" );
		/* reset the flags used for OTF-Update */
		bTftpToFlashStatus &= ~B_PARTITION_IS_JFFS2;
		bTftpToFlashStatus &= ~B_WRITE_IMG_TO_FLASH;
	}
	else
#endif /* CONFIG_TFTP_UPDATE_ONTHEFLY */
	{
		printf( "Update successful\n" );
	}
        return 0;

error:
	if( ( NULL != pPartEntry ) &&
            ( pPartEntry->flags.bFixed || pPartEntry->flags.bReadOnly ) )
                /* it might have been unprotected */
                PartProtect( pPartEntry, 1 );
#if defined(CONFIG_CMD_UBI)
	if( ( NVPT_FILESYSTEM == pPartEntry->eType ) &&
            ( NVFS_UBIFS      == pPartEntry->flags.fs.eType ) &&
            (!is_ubi(szImg))) {
		/* Break the actual UBI update to free up memory */
		ubi_volume_off_write_break((char *)pPartEntry->szName);

		sprintf( szCmd, "ubi silent 0" );
		RunCmd(szCmd);
	}
#endif /* CONFIG_CMD_UBI */

#ifdef CONFIG_TFTP_UPDATE_ONTHEFLY
	/* Reset OTF update flag */
	bTftpToFlashStatus = 0;
#endif

	return 1;
}

static int do_digi_verify(cmd_tbl_t* cmdtp, int flag, int argc, char * const argv[])
{
        int iLoadAddr  = -1;
        const image_source_t* pImgSrc = NULL;
        const part_t* pPart           = NULL;
        const char*   szTmp           = NULL;
        const nv_param_part_t* pPartEntry = NULL;
        int iFileSize  = 0;
        int iCRCSize   = 0;

        crc32_t uiCRC;
#if defined(CONFIG_CMD_UBI)
        char szCmd[ PATH_MAXLEN ] = "";
#endif
	char szImg[ PATH_MAXLEN ] = "";
        char bDHCPEnabled = 0;
        static const char* szUpdating = "Updating";
	char kdevpart[8];
	char kfs[8];
	int nth_part = 0;	/* partition number to look from on WhatPart() calls */
	long sizeTmp;

        if( ( argc < 2 ) || ( argc > 6 ) )
                return CMD_RET_USAGE;

        clear_ctrlc();

        if( argc >= 3 )
                pImgSrc = WhatImageSource( argv[ 2 ] );
        else
                pImgSrc = &l_axImgSrc[ IS_TFTP ];

        if( NULL == pImgSrc )
                return CMD_RET_USAGE;

        /* find the partition to verify */
        CE( WhatPart( argv[ 1 ], 0, &pPart, &pPartEntry, 1, &eOSType, nth_part ) );

        /* determine file to verify */
	if ( pImgSrc->eType != IS_RAM )
	{
		if( argc == 4 ){
			szTmp = argv[ 3 ];
		} else if (argc == 6) {
			szTmp = argv[5];
		} else if( NULL != pPart ) {
			/* not present, but we have a partition definition */
			szTmp = GetEnvVar( pPart->szEnvVar, 0 );
			CE( NULL != szTmp );
		} else {
			eprintf( "Require filename\n" );
			goto error;
		}
		SAFE_STRCAT(szImg, szTmp);
	}

        CE( GetDHCPEnabled( &bDHCPEnabled ) );

        /* user input processed, determine load addresses */
	CE(GetIntFromEnvVar(&iLoadAddr, "loadaddr", 0));

        /* we require it being set from download tool */
        setenv( "filesize", "" );

	if( bDHCPEnabled &&
	    (( IS_TFTP == pImgSrc->eType ) || ( IS_NFS == pImgSrc->eType )) ){
		/* makes no sense for USB or FLASH download to need DHCP */
		CE( DoDHCP() );
	}

	if (pImgSrc->eType == IS_USB || pImgSrc->eType == IS_MMC ||
	    pImgSrc->eType == IS_HSMMC || pImgSrc->eType == IS_SATA) {
		/* kernel device:partition can be given as argument
		* Otherwise, default value is used */
		if (argc >= 4)
			strncpy(kdevpart, argv[3], ARRAY_SIZE(kdevpart));
		else
			strncpy(kdevpart, DEFAULT_KERNEL_DEVPART, ARRAY_SIZE(kdevpart));
		kdevpart[ARRAY_SIZE(kdevpart) - 1] = 0;	/* zero-terminate string */

		/* Kernel file system can be given as argument
		* Otherwise, default value is used */
		if (argc >= 5)
			strncpy(kfs, argv[4], ARRAY_SIZE(kfs));
		else
			strncpy(kfs, DEFAULT_KERNEL_FS, ARRAY_SIZE(kfs));
		kfs[ARRAY_SIZE(kfs) - 1] = 0;	/* zero-terminate string */
	}

	if (IS_RAM == pImgSrc->eType) {
		/* command requires a download address and a size */
		if ( argc < 5 )
		{
			eprintf( "Missing arguments: verify <os> ram <load_address> <size>\n" );
			goto error;
		}

		iLoadAddr = get_input(argv[3]);
		if( iLoadAddr == -1 )
		{
			eprintf( "Invalid image address\n" );
			goto error;
		}

		sizeTmp = get_input(argv[4]);
		if ( sizeTmp == -1 )
		{
			eprintf( "Invalid image size\n" );
			goto error;
		}
		sprintf(szImg, "%lX", (ulong)sizeTmp);
		setenv("filesize", szImg);
	}
	else {
		/* put file into RAM */
		CEN(put_file_into_ram(pImgSrc->eType,
				      iLoadAddr,
				      szImg,
				      kdevpart,
				      kfs));
	}

        /* should be set by download tool */
        CE( GetIntFromEnvVar( &iFileSize, "filesize", 0 ) );


#ifdef CONFIG_CMD_BOOTSTREAM
        /* verify of bootstream image is handled differently */
	if (NVPT_BOOTSTREAM == pPartEntry->eType) {
		//TODO
		printf("Verify of bootstream partitions is not supported!\n");
	}
#endif

	/* TODO:
	 * crc32() must be adjusted to read sector by sector
	 * to calculate a check sum out of flash for OTF-Update
	 */

	/* do crc32 calculation */
        iCRCSize = iFileSize - ( (NULL != pPart && pPart->bCRC32Appended) ?
		   sizeof( uiCRC ) : 0 );
	if (iCRCSize <= 0) {
		printf("Partition requires checksum but provided file is to small \
			to contain a checksum\n");
		goto error;
	}
        uiCRC = crc32( 0, (uchar*) iLoadAddr, iCRCSize );
        printf( "Calculated checksum = 0x%x\n", (uint32_t)uiCRC );

        if (NULL != pPart && pPart->bCRC32Appended) {
                /* check CRC32 in File */
                crc32_t uiCRCFile;
                /* works independent whether the CRC is aligned or not. We
                   * don't know what the image does. */
                memcpy( &uiCRCFile, (const crc32_t*) ( iLoadAddr + iCRCSize ),
                        sizeof( uiCRCFile ) );
                if( uiCRCFile != uiCRC ) {
                        eprintf( "CRC32 mismatch: Image reports 0x%0x - ",
                                 (uint32_t)uiCRCFile );
                        CE( WaitForYesPressed( "Continue", szUpdating ) );
                }
        }

#ifdef CONFIG_PARTITION_SWAP
        if( ( NULL != pPart ) && pPart->bSwapEndian ) {
                /* swapping is done on 16/32bit. Ensure that there is space */
                int iFileSizeAligned = ( iFileSize + 0x3 ) & ~0x3;
                /* fill swapped areas with empty character */
                memset( (void*) iLoadAddr + iFileSize, 0xff, iFileSizeAligned - iFileSize );
                iFileSize = iFileSizeAligned;
        }
#endif

        /* verify images */

        /* fit we into it? */
        if( PartSize( pPartEntry ) < iFileSize ) {
                /* 0 means to end of flash */
                eprintf( "Partition too small\n" );
                goto error;
        }

#if defined(CONFIG_CMD_UBI)
        if( ( NVPT_FILESYSTEM == pPartEntry->eType ) &&
            ( NVFS_UBIFS      == pPartEntry->flags.fs.eType ) &&
            ( !is_ubi(szImg))) {
                printf( "\nVerifying UBI image\n" );

		/* Enable UBI messages */
		sprintf( szCmd, "ubi silent 0" );
		CEN( RunCmd(szCmd) );

		/* Verify it */
		CE( !ubi_volume_verify((char *)pPartEntry->szName, (char *)iLoadAddr, 0, iFileSize, 0) );
	}
        else
#endif
	{
		printf("Verifying partition '%s'\n", pPartEntry->szName);
		CE( AppendPadding( pPartEntry, (void*) iLoadAddr, &iFileSize ) );

#ifdef CONFIG_PARTITION_SWAP
		if( ( NULL != pPart ) && pPart->bSwapEndian )
			/* do it before writing. Swapping not supported for user
			* named partitions */
			CE( PartSwap( pPartEntry, (void*) iLoadAddr, iFileSize ) );
#endif

		/* verify partition */
		CE( PartVerify( pPartEntry, 0, (void*) iLoadAddr, iFileSize, 0) );
	}

	printf( "Verify successful\n" );
        return 0;

error:
        return 1;
}


#if defined(CONFIG_CMD_UBI)
static int RecreateUBIVolume ( const nv_param_part_t* pPartEntry )
{
	char szCmd[ PATH_MAXLEN ];

	/* Select UBI partition */
	sprintf( szCmd, "ubi part %s", pPartEntry->szName );
	if (!RunCmd(szCmd)) {
		printf( "\nErasing UBI partition\n" );
		/* Erase partition sectors, and try again */
		CE( PartErase( pPartEntry ) );
		CEN( RunCmd( szCmd ) );
	}
	else {
		printf( "\nRemoving all user volume\n" );
		/* Partition is not clean, delete all user volume */
		sprintf( szCmd, "ubi remove all" );
		CEN( RunCmd( szCmd ) );
	}

	/* Creating user volume with the name of the partition */
	/* Creating a dynamic UBI partition to support OTF updating */
	sprintf( szCmd, "ubi create %s 0 d", pPartEntry->szName );
	CEN( RunCmd( szCmd ) );

	return 1;
error:
	return 0;
}
#endif /* CONFIG_CMD_UBI */

#if defined(CONFIG_CMD_NAND) && !defined(CONFIG_NAND_OBB_ATOMIC)
static int jffs2_mark_clean (long long offset, long long size)
{
	struct mtd_info *nand;
	struct mtd_oob_ops oob_ops;
	int i, magic_ofs, magic_len;
	long long end;
	unsigned char magic[] = {0x85, 0x19, 0x03, 0x20, 0x08, 0x00, 0x00, 0x00};

	for (i = 0; i < CONFIG_SYS_MAX_NAND_DEVICE; i++) {
		if (nand_info[i].name)
			break;
	}
	nand = &nand_info[i];
        /* !see [1] */
	switch(nand->oobsize) {
		case 8:
			magic_ofs = 0x06;
			magic_len = 0x02;
                        break;
		case 16:
			magic_ofs = 0x08;
			magic_len = 0x08;
			break;
		case 64:
			magic_ofs = 0x10;
			magic_len = 0x08;
			break;
		default:
			printf("Cannot set markers on this oobsize!\n");
                        goto error;
	}

	oob_ops.mode = MTD_OOB_PLACE;
	oob_ops.ooblen = magic_len;
	oob_ops.oobbuf = magic;
	oob_ops.datbuf = NULL;
	oob_ops.ooboffs = magic_ofs;

	/* calculate end */
	for(end = offset + size; offset < end; offset += nand->erasesize) {
		/* skip if block is bad */
		if(nand->block_isbad(nand, offset))
			continue;

		/* modify oob */
		 if(nand->write_oob(nand, offset, &oob_ops))
			 goto error;

	} /* for( end=offset) */

	return 1;

error:
	return 0;
}
#endif  /* CONFIG_CMD_NAND */

static int do_envreset(cmd_tbl_t* cmdtp, int flag, int argc, char * const argv[])
{
	switch( argc ) {
		case 1:
		break;

            default:
		printf( "Usage:\n%s\n", cmdtp->usage );
		return -1;
	}

        NvEnvUseDefault("Environment will be set to Default now!\n");

        return saveenv();
}

static int do_printenv_dynamic(cmd_tbl_t* cmdtp, int flag, int argc, char * const argv[])
{
        int i;

        for (i = 0; i < dynvars; i++) {
                const char* szVar = l_axEnvDynamic[ i ].szEnvVar;
                printf( "%s=%s\n", szVar,  GetEnvVar( szVar, 0 ) );
        }

        return 0;
}

static int do_erase_pt(cmd_tbl_t* cmdtp, int flag, int argc, char * const argv[])
{
        const nv_param_part_t* pPartEntry = NULL;
        const part_t* pPart               = NULL;
	int nth_part = 0;	/* partition number to look from on WhatPart() calls */

        if( argc != 2 )
                return CMD_RET_USAGE;

        clear_ctrlc();

        /* determine OS and/or partition */
        CE( WhatPart( argv[ 1 ], 0, &pPart, &pPartEntry, 1, NULL, nth_part ) );

        switch( pPartEntry->eType ) {
            case NVPT_UBOOT:
            case NVPT_NVRAM:
                /* protect against silly stuff. Use update/envreset */
                eprintf( "Can't erase a critical partition\n" );
                goto error;
            default:
                break;
        }

        if( pPartEntry->flags.bReadOnly || pPartEntry->flags.bFixed )
                CE( WaitForYesPressed( "Partition marked read-only /fixed. Do you want to continue?", "Erasing" ) );
        CE( PartProtect( pPartEntry, 0 ) );
        CE( PartErase( pPartEntry ) );
        if( pPartEntry->flags.bFixed || pPartEntry->flags.bReadOnly)
                CE( PartProtect( pPartEntry, 1 ) );

        return 0;

error:
        return 1;
}

static int WhatPart(
        const char* szPart, char bForBoot,
        const part_t** ppPart,
        const nv_param_part_t** ppPartEntry,
        char bPartEntryRequired,
	nv_os_type_e *os_type,
	int count)
{
        int i = 0;
        const part_t* pPart = NULL;
        const nv_param_part_t* pPartEntry = NULL;

	/* Search in l_axPart by the abreviated name */
        while( i < ARRAY_SIZE( l_axPart ) ) {
                if( !strcmp( szPart, l_axPart[ i ].szName ) &&
                    ( !bForBoot || l_axPart[ i ].bForBoot ) ) {
                        pPart = &l_axPart[ i ];

                        /* abbreviated name found, does it exist? */
                        /* find count'th partition */
                        NvParamPartFind( &pPartEntry, pPart->ePartType,
                                         NVFS_NONE, pPart->bRootFS, count );
			/* assign os */
			if (NULL != os_type)
				*os_type = l_axPart[ i ].eOSType;
                        break;
                }
		pPart = NULL;	/* Reset if not found */
                i++;
        } /* !while */

        if( NULL == pPart ) {
               /* internal abbrevation name userfs/rootfs/u-boot/not found,
                   try it with szPart as partition name.
		   wifical lives inside the NVRAM partition. Tweak that if
                   that is what we are looking for */
#ifdef CFG_HAS_WIRELESS
		if( !strcmp( szPart, "wifical" ) )
                        pPartEntry = FindPartition( "NVRAM" );
                else
#endif
                        pPartEntry = FindPartition( szPart );
                if( NULL == pPartEntry ) {
                        goto error;
                }
		else {
			/* Find the first partition in l_axPart array that
			* matches the partition type of the partition
			* located in NVRAM */
			findpart_tableentry(&pPart, pPartEntry, 0);
			/* assign os */
			if (NULL != os_type && NULL != pPart)
				*os_type = pPart->eOSType;
		}
        }

        if( bPartEntryRequired && ( NULL == pPartEntry ) ) {
                goto error;
        }

	if( NULL != pPart )
		*ppPart = pPart;

	if( NULL != pPartEntry)
		*ppPartEntry = pPartEntry;

        return 1;

error:
	if (count > 0)
		eprintf( "Couldn't find a second partition %s (dual boot support enabled)\n", szPart );
	else
		eprintf( "Partition %s not found\n", szPart );
        return 0;
}

static const image_source_t* WhatImageSource( const char* szSrc )
{
        int i = 0;

        while( i < ARRAY_SIZE( l_axImgSrc ) ) {
                if( !strcmp( szSrc, l_axImgSrc[ i ].szName ) )
                        return &l_axImgSrc[ i ];

                i++;
        } /* !while */

        return NULL;
}

/*! \brief Runs command and prints error on failure */
/*! \return 0 on failure otherwise 1
 */
static int RunCmd( const char* szCmd )
{
        int iRes;

        clear_ctrlc();

        iRes = (run_command(szCmd, 0) == 0);

	if( ctrlc() || had_ctrlc() )
                iRes = 0;

        if( !iRes )
                eprintf( "command %s failed\n", szCmd );

        return iRes;
}

/*! \brief Returns dynamic or normal environment variable */
/*! \param bSilent true if not existant should not be reported
 */
const char* GetEnvVar( const char* szVar, char bSilent )
{
        const char* szTmp = NULL;
        static char szTmpBuf[ 128 ];
	static char sEraseSize[ 8 ];
        static char ending[ 10 ];
        static char rootfsbasename[ 30 ];

	szTmp = getenv( (char*) szVar );
        if( ( NULL == szTmp ) ) {
                /* variable not yet defined, try to read it in l_axPart */
                if( !strcmp( szVar, RIMG ) || !strcmp(szVar, ARIMG) || !strcmp( szVar, USRIMG ) ) {
                        char bRIMG = !strcmp( szVar, RIMG ) || !strcmp(szVar, ARIMG);
                        const nv_param_part_t* pPartEntry;
                        size_t iEraseSize = GetEraseSize( NULL );

                        /* find first rootfs partition */
                        if( !NvParamPartFind( &pPartEntry, NVPT_FILESYSTEM,
                                              NVFS_NONE, bRIMG, 0 ) )
                                return "";

                        iEraseSize = GetEraseSize( pPartEntry );
			memset(sEraseSize,0,sizeof(sEraseSize));

			if( bRIMG ) {
				switch( pPartEntry->flags.fs.eType) {
					case NVFS_JFFS2:
						sprintf(sEraseSize, "-%i", iEraseSize / 1024);
						strcpy( ending, "jffs2");
						break;
					case NVFS_CRAMFS:
						strcpy( ending, "cramfs");
						break;
					case NVFS_INITRD:
						strcpy( ending, "initrd");
						break;
					case NVFS_SQUASHFS:
						strcpy( ending, "squashfs");
						break;
					case NVFS_ROMFS :
						strcpy( ending, "romfs");
						break;
					case NVFS_YAFFS2 :
						strcpy( ending, "yaffs2");
						break;
					case NVFS_UBIFS:
						sprintf(sEraseSize, "-%i", iEraseSize / 1024);
						strcpy( ending, "ubifs");
						break;
					default:
						sprintf(sEraseSize, "-%i", iEraseSize / 1024);
						strcpy( ending, "jffs2");
						break;
				}
				if (!strcmp(szVar, ARIMG))
					sprintf(rootfsbasename, "android-%s", platformname);
				else
					sprintf(rootfsbasename, "rootfs-%s", platformname);
				sprintf( szTmpBuf, "%s%s.%s", rootfsbasename, sEraseSize, ending );
			} else
				sprintf( szTmpBuf, "userfs-%s-%i.jffs2",
						platformname, iEraseSize / 1024 );
			szTmp = szTmpBuf;
		} else if( !strcmp( szVar, CONSOLE ) ) {
			char *baudrate;

			sprintf( szTmpBuf, "console="CONFIG_CONSOLE"%i",
					CONSOLE_INDEX);
			baudrate = getenv("baudrate");
			if (NULL == baudrate)
				sprintf(baudrate, "%d", CONFIG_BAUDRATE);
			strcat(szTmpBuf, ",");
			strcat(szTmpBuf, baudrate);
			szTmp = szTmpBuf;
#ifdef CONFIG_COMPUTE_LPJ
		} else if( !strcmp( szVar, LPJ ) ) {
			sprintf( szTmpBuf, "%u", get_preset_lpj() );
			szTmp = szTmpBuf;
#endif
		} else {
			int i = 0;

			while (i < dynvars) {
				if( !strcmp( l_axEnvDynamic[ i ].szEnvVar, szVar ) ) {
					szTmp = l_axEnvDynamic[ i ].szEnvDflt;
					break;
				}
				i++;
			} /* while */
		}

		if( ( NULL == szTmp ) && !bSilent )
			eprintf( "variable %s does not exists\n", szVar );
	}

        return szTmp;
}

/*! \brief Converts an environment variable to an integer */
/*! \return 0 on failure otherwise 1
 */
static int GetIntFromEnvVar( int* piVal, const char* szVar, char bSilent )
{
        int iRes = 0;
        const char* szVal = GetEnvVar( szVar, bSilent );

        if( NULL != szVal ) {
                *piVal = simple_strtoul( szVal, NULL, 16 );
                iRes = 1;
        }

        return iRes;
}

/*! \brief padds the image, also adds JFFS2 Padding if it's a jffs2 partition. */
/*! \return 0 on failure otherwise wise 1
 */
static int AppendPadding( const nv_param_part_t* pPart, void* pvImage,
                          int* piFileSize )
{
        int iPageSize = GetPageSize( pPart );
        int iBytesFreeInBlock;

        if( !iPageSize )
                return 0;

	if (*piFileSize % iPageSize == 0)
		return 1;	/* image file is already aligned to page */

        iBytesFreeInBlock = iPageSize - (*piFileSize % iPageSize);

        if( ( NVPT_FILESYSTEM == pPart->eType ) &&
            ( NVFS_JFFS2      == pPart->flags.fs.eType ) &&
            ( iBytesFreeInBlock >= sizeof( struct jffs2_unknown_node ) ) ) {
                /* JFFS CRC32 starts from 0xfffffff, our crc32 from 0x0 */
                uint32_t uiStart = 0xffffffff;
                /* write padding to avoid Empty block messages.
                   see linux/fs/jffs2/wbuf.c:flush_wbuf */
                struct jffs2_unknown_node* pNode = (struct jffs2_unknown_node*) ( pvImage + *piFileSize );

                printf( "Padding last sector\n" );

                /* 0 for JFFS2 PADDING Type, see wbuf.c */
                memset( pvImage + *piFileSize, 0x0, iBytesFreeInBlock );

                *piFileSize       += iBytesFreeInBlock;
                iBytesFreeInBlock -= sizeof( *pNode );

                CLEAR( *pNode );

                /* no cpu_to_je16, it's always host endianess in U-Boot */
                pNode->magic    = JFFS2_MAGIC_BITMASK;
                pNode->nodetype = JFFS2_NODETYPE_PADDING;

                pNode->totlen   = iBytesFreeInBlock;
                /* don't CRC32 the hdr_crc itself */
                pNode->hdr_crc  = crc32( uiStart, (uchar*) pNode, sizeof( *pNode ) - 4 ) ^ uiStart;
        } else {/* if( JFFS2 ) */
                /* clear remaining stuff */
                memset( pvImage + *piFileSize, 0xff, iBytesFreeInBlock );
        }

        return 1;
}

static const nv_param_part_t* FindPartition( const char* szName )
{
        nv_critical_t*               pCrit      = NULL;
        const nv_param_part_table_t* pPartTable = NULL;
        unsigned int u;

        if( !CW( NvCriticalGet( &pCrit ) ) )
                return NULL;

        pPartTable = &pCrit->s.p.xPartTable;

        for( u = 0; u < pPartTable->uiEntries; u++ )
		if( !strcmp( szName, pPartTable->axEntries[ u ].szName) )
                        return &pPartTable->axEntries[ u ];

        return NULL;
}

static int GetDHCPEnabled( char* pcEnabled )
{
        nv_critical_t* pCrit = NULL;

        *pcEnabled = 0;
        CE( NvCriticalGet( &pCrit ) );

        *pcEnabled = pCrit->s.p.xIP.axDevice[ 0 ].flags.bDHCP;

        return 1;

error:
        return 0;
}

/*! \brief performs a DHCP/BOOTP request without loading any file */
/*! \return 0 on failure otherwise 1
 */
static int DoDHCP( void )
{
	int  iRes = 0;
        char szTmp[ PATH_MAXLEN ], *s;

	/* get current value */
        if ((s = getenv("autoload")) != NULL) {
		strncpy( szTmp, s, sizeof( szTmp ) );
		s = szTmp;
        }

        printf("autoload was %s\n", szTmp);
	/* autoload is used in BOOTP to do TFTP itself.
	 * We do tftp ourself because we don't need bootfile */
	setenv( "autoload", "n" );
#ifdef CONFIG_CMD_NET
        if( NetLoop( DHCP ) >= 0 ) {
                /* taken from netboot_common. But we don't need autostart */
                netboot_update_env();
                iRes = 1;
        }
#endif
        /* restore old autoload */
        setenv( "autoload", s );
	return iRes;
}

static int VerifyCRC( uchar *addr, ulong len, ulong crc )
{
	ulong calc_crc = crc32( 0, addr, len );

	return calc_crc == crc;
}

static int check_image_header(ulong img_addr)
{
	image_header_t *hdr = (image_header_t *)img_addr;

	if (!image_check_magic(hdr))
		return 0;
	if (!image_check_hcrc (hdr))
		return 0;

	return 1;
}

#if defined(CONFIG_SOURCE) && defined(CONFIG_AUTOLOAD_BOOTSCRIPT)
void run_auto_script(void)
{
#ifdef CONFIG_CMD_NET
	int iLoadAddr = -1, ret;
	char szCmd[ PATH_MAXLEN ]  = "";
	char *s, *bootscript;
	/* Save original timeouts */
        ulong saved_rrqtimeout_msecs = TftpRRQTimeoutMSecs;
        int saved_rrqtimeout_count = TftpRRQTimeoutCountMax;
	ulong saved_startagain_timeout = NetStartAgainTimeout;

	/* Check if we really have to try to run the bootscript */
	s = (char *)GetEnvVar("loadbootsc", 1);
	bootscript = (char *)GetEnvVar("bootscript", 1 );
	if (s && !strcmp(s, "yes") && bootscript) {
		CE( GetIntFromEnvVar( &iLoadAddr, "loadaddr", 1 ) );
		printf("Autoscript from TFTP... ");

		/* set timeouts for bootscript */
		TftpRRQTimeoutMSecs = AUTOSCRIPT_TFTP_MSEC;
		TftpRRQTimeoutCountMax = AUTOSCRIPT_TFTP_CNT;
		NetStartAgainTimeout = AUTOSCRIPT_START_AGAIN;

		/* Silence net commands during the bootscript download */
		NetSilent = 1;
		DownloadingAutoScript = 1;
		sprintf( szCmd, "tftp 0x%x %s", iLoadAddr, bootscript );
		ret = run_command( szCmd, 0 );
		/* First restore original values of global variables
		 * and then evaluate the result of the run_command */
		NetSilent = 0;
		DownloadingAutoScript = 0;
		/* Restore original timeouts */
		TftpRRQTimeoutMSecs = saved_rrqtimeout_msecs;
		TftpRRQTimeoutCountMax = saved_rrqtimeout_count;
		NetStartAgainTimeout = saved_startagain_timeout;

		if (ret < 0)
			goto error;

		printf("[ready]\nRunning bootscript...\n");
		RunningAutoScript = 1;
		/* Launch bootscript */
		ret = source( iLoadAddr, NULL );
		RunningAutoScript = 0;
	}
	return;

error:
	printf( "[not available]\n" );
#endif
}
#else
void run_auto_script(void) {}
#endif

/* Looks in the l_axPart table, the iCount'th entry
 * that matches the given partition type */
int findpart_tableentry(
        const part_t **ppPart,
	const nv_param_part_t *pPartEntry,
        int            iCount )
{
	unsigned int u = 0;
	int iFound = 0;

	*ppPart = NULL;

	/* search the iCount'th partition entry */
	while( u < ARRAY_SIZE( l_axPart ) ) {
		if( pPartEntry->eType == l_axPart[u].ePartType ) {
			/* found one occurence */
			if( iFound == iCount ) {
				/* it's the n'th occurence we look for */
				*ppPart = (part_t *)&l_axPart[u];
				return 1;
			}
			iFound++;
		} /* if( pPartEntry->eType */

		u++;
	} /* while( u ) */

	return 0;
}

/**
 * Toggles the boot partition in dual boot systems
 * returns 0 on error, 1 otherwise
 */
#ifdef CONFIG_DUAL_BOOT
int dualb_save(void)
{
	/*  Save dualb_data to NVRAM if it changed */
	if (memcmp(&original_dualb, &dualb, sizeof(dualb))) {
		if (!NvPrivDualBootWrite(&dualb)) {
			DUALB_PRINT("Dual boot data could not be saved!\n");
			return 0;
		}
		if (!CW(NvSave())) {
			DUALB_PRINT("Dual boot data could not be saved!\n");
			return 0;
		}
		/* Save new original to avoid future copies */
		memcpy(&original_dualb, &dualb, sizeof(dualb));
	}
	return 1;
}

int dualb_toggle(void)
{
	/* If we are toggling is because the current system
	 * failed to boot, so mark it as NOT available
	 */
	dualb_data->avail[dualb_data->boot_part] = 0;
#if defined(CONFIG_DUAL_BOOT_MODE_PEER)
	/* toggle the boot partition */
	dualb_data->boot_part = !dualb_data->boot_part;
#elif defined(CONFIG_DUAL_BOOT_MODE_RESCUE)
	/* in rescue mode, we can only toggle to the rescue system */
	dualb_data->boot_part = 1;
#endif
	/* reset the boot attempts */
	dualb_reset_bootattempts();
	/* Save data in NVRAM */
	dualb_save();
	/* check for availability of firmware */
	if (!dualb_data->avail[dualb_data->boot_part])
		return -1;

	return 0;
}
#endif /* CONFIG_DUAL_BOOT */

#if 0
/**
 * This code is deprecated. From now on functions should implement the necessary
 * stuff before jumping to the OS in the proper function (board_cleanup_before_linux)
 * or equivalent, to avoid missconfigurations if the boot command fails (by disabled
 * hardware.
 */

/**
 * Setup function for initialization/deinitialization needed
 * for each platform before jumping to the OS
 **/
void setup_before_os_jump(nv_os_type_e eOSType, image_source_e eType)
{
#if defined(CONFIG_NS9360)
	if (NVOS_LINUX == eOSType) {
		/* If booting Linux stop USB */
		usb_stop();
	}
#endif

#if defined(CONFIG_CC9M2443) || defined(CONFIG_CCW9M2443)
	/* Enable USB_POWEREN line before booting the OS */
	s3c_gpio_setpin(S3C_GPA14, 0);
#endif

#if defined(CONFIG_NS9215)
	eth_halt();
#endif
}
#endif

U_BOOT_CMD(
	dboot,	9,	0,	do_digi_dboot,
	"Digi modules boot commands",
	"<os> [source] [extra-args...]\n"
	" Description: Boots <os> via <source>\n"
	" Arguments:\n"
	"   - os:           a partition name or one of the reserved names: \n"
	"                   linux|android\n"
	"   - [source]:     tftp (default)|flash|nfs|usb|mmc|hsmmc|sata|ram\n"
	"   - [extra-args]: extra arguments depending on 'source'\n"
	"\n"
	"      source=tftp|nfs -> [filename] [fdtfilename]\n"
	"       - filename: kernel file to transfer (required if using a partition name)\n"
	"       - fdtfilename: DTB file to transfer\n"
	"\n"
	"      source=usb|mmc|hsmmc|sata -> [device:part filesystem] [filename] [rootfspart] [fdtfilename]\n"
	"       - device:part: number of device and partition\n"
	"       - filesystem: fat|vfat|ext2|ext3\n"
	"       - filename: kernel file to transfer\n"
	"       - rootfspart: root parameter to pass to the kernel command line.\n"
	"                     If omitted uses the one at variable usb_rpart|mmc_rpart|sata_rpart.\n"
	"       - fdtfilename: DTB file to transfer\n"
	"\n"
	"      source=ram -> [image_address] [initrd_address] [initrd_max_size] [fdt_address]\n"
	"       - image_address: address of image in RAM\n"
	"       - initrd_address: address of initrd image (default: loadaddr_initrd)\n"
	"       - initrd_max_size: max. allowed ramdisk size (in kB) to pass to the kernel (default: kernel default)\n"
	"       - fdt_address: address of DTB image in RAM\n"
	"\n"
);

U_BOOT_CMD(
	update,	6,	0,	do_digi_update,
	"Digi modules update commands",
	"<partition> [source] [extra-args...]\n"
	" Description: updates flash <partition> via <source>\n"
	" Arguments:\n"
	"   - partition:    a partition name or one of the reserved names: \n"
	"                   uboot|linux|android|rootfs|userfs|androidfs|splash|fdt\n"
	"   - [source]:     tftp (default)|nfs|usb|mmc|hsmmc|sata|ram\n"
	"   - [extra-args]: extra arguments depending on 'source'\n"
	"\n"
	"      source=tftp|nfs -> [filename]\n"
	"       - filename: file to transfer (required if using a partition name)\n"
	"\n"
	"      source=usb|mmc|hsmmc|sata -> [device:part filesystem] [filename]\n"
	"       - device:part: number of device and partition\n"
	"       - filesystem: fat|vfat|ext2|ext3\n"
	"       - filename: file to transfer\n"
	"\n"
	"      source=ram -> <image_address> <image_size>\n"
	"       - image_address: address of image in RAM\n"
	"       - image_size: size of image in RAM\n"
);

U_BOOT_CMD(
	verify,	6,	0,	do_digi_verify,
	"Digi modules verify command",
	"<partition> [source] [extra-args...]\n"
	" Description: verifies firmware in flash <partition> against file loaded from <source>\n"
	" Arguments:\n"
	"   - partition:    a partition name or one of the reserved names: \n"
	"                   uboot|linux|android|rootfs|userfs|androidfs|splash|fdt\n"
	"   - [source]:     tftp (default)|nfs|usb|mmc|hsmmc|sata|ram\n"
	"   - [extra-args]: extra arguments depending on 'source'\n"
	"\n"
	"      source=tftp|nfs -> [filename]\n"
	"       - filename: file to transfer (required if using a partition name)\n"
	"\n"
	"      source=usb|mmc|hsmmc|sata -> [device:part filesystem] [filename]\n"
	"       - device:part: number of device and partition\n"
	"       - filesystem: fat|vfat|ext2|ext3\n"
	"       - filename: file to transfer\n"
	"\n"
	"      source=ram -> <image_address> <image_size>\n"
	"       - image_address: address of image in RAM\n"
	"       - image_size: size of image in RAM\n"
);

U_BOOT_CMD(
	envreset,	1,	0,	do_envreset,
	"Sets environment variables to default setting",
	"\n"
	"  - overwrites all current variables\n"
);

U_BOOT_CMD(
	printenv_dynamic,	1,	0,	do_printenv_dynamic,
	"Prints all dynamic variables",
	"\n"
	"  - Prints all dynamic variables\n"
);

U_BOOT_CMD(
	erase_pt,	2,	0,	do_erase_pt,
	"Erases the partition",
	"\npartition - the name of the partition\n"
);

#ifdef CONFIG_DUAL_BOOT
static void start_wdt(void)
{
#ifdef CONFIG_DUAL_BOOT_WDT_ENABLE
	unsigned long wdt_timeout;

	wdt_timeout = simple_strtol(GetEnvVar("dualb_wdt_timeout", 1), NULL, 10);
	wdt_timeout = board_wdt_enable(wdt_timeout);
	DUALB_PRINT("Setting watchdog timeout to %lus\n", wdt_timeout);
#endif /* CONFIG_DUAL_BOOT_WDT_ENABLE */
}

#ifdef CONFIG_DUAL_BOOT_PERSISTENT_IN_NVRAM
/* dual_boot data is read once on dboot or update commands.
 * Since these commands may change the values temporarily
 * we don't want to reread in these functions.
 * We instead will use the temporary data in dualb global
 * variable. */

void dualb_increment_bootattempts(void)
{
	/* increment boot attempts */
	dualb.data.boot_attempts++;

	/* write value to NVRAM */
	if (!NvPrivDualBootWrite(&dualb)) {
		DUALB_PRINT("Dual boot data could not be initialized\n");
	}
}

int dualb_get_bootattempts(void)
{
	return dualb.data.boot_attempts;
}

void dualb_reset_bootattempts(void)
{
	/* reset boot attempts */
	dualb.data.boot_attempts = 0;
	/* write value to NVRAM */
	if (!NvPrivDualBootWrite(&dualb)) {
		DUALB_PRINT("Dual boot data could not be initialized\n");
	}
}
#endif /* CONFIG_DUAL_BOOT_PERSISTENT_IN_NVRAM */

#ifndef CONFIG_DUAL_BOOT_CUSTOM_NOSYSTEM_PANIC
/* Function that is called when no system can be booted.
 * Typically will show an error message or LED code
 */
void dualb_nosystem_panic(void)
{
	eprintf("*** No firmware image succeeded to boot. Aborting\n");
}
#endif /* CONFIG_DUAL_BOOT_CUSTOM_NOSYSTEM_PANIC */

#endif /* CONFIG_DUAL_BOOT */

static int uses_capacitive_touch(const char *video)
{
	/* List capacitive touchscreen LCD displays */
	char *capacitive_touch_lcds[] = {
		"HSD101PFW2",
		"F04B0101",
		"F07A0102",
	};
	int i;
	char *lcd = strstr(video, "@") + 1;	/* point to the LCD name */

	for (i=0; i < ARRAY_SIZE(capacitive_touch_lcds); i++) {
		if (!strcmp(lcd, capacitive_touch_lcds[i]))
			return 1;
	}
	return 0;
}

static int append_calibration(void)
{
	const char *vp;

	/* If calibration is set to "no" skip passing calibration param to kernel */
	vp = GetEnvVar( "calibration", 1 );
	if (!vp || strncmp(vp, "no", 2)) {
		/* Calibration is not disabled */
		/* Get primary display */
		if((vp = GetEnvVar( FBPRIMARY_VAR, 1 )) != NULL) {
			if((vp = GetEnvVar( vp, 1 )) != NULL) {
				/* vp now contains the primary display */
				/* Skip calibration if the video is VGA,
				 * HDMI or some specific displays that use
				 * capacitive touch controllers */
				if ((strncmp(vp, VGA_PREFIX, strlen(VGA_PREFIX)) &&
				     strncmp(vp, HDMI_PREFIX, strlen(HDMI_PREFIX)) &&
				     !uses_capacitive_touch(vp)))
					return 1;
			}
		}
	}
	return 0;

}

/* ********** global functions ********** */

int create_dynvar(char *name, char *value)
{
	/* All variables (names and values) must be allocated */

	/* Name */
	l_axEnvDynamic[dynvars].szEnvVar = (char *)malloc(strlen(name) + 1);
	if (NULL == l_axEnvDynamic[dynvars].szEnvVar)
		return 1;
	strcpy(l_axEnvDynamic[dynvars].szEnvVar, name);

	/* Value */
	l_axEnvDynamic[dynvars].szEnvDflt = (char *)malloc(strlen(value) + 1);
	if (NULL == l_axEnvDynamic[dynvars].szEnvDflt)
		return 1;
	strcpy(l_axEnvDynamic[dynvars].szEnvDflt, value);

	dynvars++;
	return 0;
}

/*
 * This function generates the module name.
 * By default it will use new name 'ccimx51'. If the variable
 * 'legacynames' exists and is 'yes', it will use 'ccmx51' or 'ccwmx51' based
 * on the hwid.
 * If the variable does not exist, but the config option
 * CONFIG_LEGACY_PLATFORM_NAMES is enabled, it will use legacy names.
 */
void generate_modulename(void)
{
#ifdef MODULE_COMPOSED_NAME
	const char *s = getenv("legacynames");
	int legacynames = 0;

#ifdef CONFIG_LEGACY_PLATFORM_NAMES
	if (NULL == s)
		legacynames = 1;
#endif
	if (NULL != s) {
		if (!strcmp(s, "on") || !strcmp(s, "yes") || !strcmp(s, "1"))
			legacynames = 1;
	}

	strncpy(modulename, MODULENAME_PREFIX, PLATFORMNAME_MAXLEN);
	if (legacynames) {
		if (variant_has_wireless())
			SAFE_STRCAT(modulename, "w");
	}
	else
		SAFE_STRCAT(modulename, "i");
	SAFE_STRCAT(modulename, MODULENAME_SUFFIX);
#else
	strncpy(modulename, CONFIG_MODULE_NAME, PLATFORMNAME_MAXLEN);
#endif /* MODULE_COMPOSED_NAME */
}

/*
 * This function generates the platform name by appending the constant
 * PLATFORM (typically 'js') to the module name on platforms with composed
 * names.
 */
void generate_platformname(void)
{
	generate_modulename();
	strncpy(platformname, modulename, PLATFORMNAME_MAXLEN);
#ifdef MODULE_COMPOSED_NAME
	SAFE_STRCAT(platformname, PLATFORM);
#endif
}

/* This function generates the contents of dynamic variables */
int generate_dynamic_vars(void)
{
	int ret = 0;
	char temp[PATH_MAXLEN];
	unsigned char enetaddr[6];

	sprintf(temp, "uImage-android-%s", platformname);
	ret |= create_dynvar("aimg", temp);
#ifdef CONFIG_DUAL_BOOT
	ret |= create_dynvar("dualb_retries", MK_STR(CONFIG_DUAL_BOOT_RETRIES));
#ifdef CONFIG_DUAL_BOOT_WDT_ENABLE
	ret |= create_dynvar("dualb_wdt_timeout",
			     MK_STR(CONFIG_DUAL_BOOT_WDT_TIMEOUT));
#endif /* CONFIG_DUAL_BOOT_WDT_ENABLE */
#endif /* CONFIG_DUAL_BOOT */
#ifdef CONFIG_MODULE_NAME_WCE
	ret |= create_dynvar("ebootaddr", MK_STR(CONFIG_LOADADDR));
	ret |= create_dynvar("eimg", "eboot-"CONFIG_MODULE_NAME_WCE);
#endif
#ifdef CONFIG_CMD_SATA
	ret |= create_dynvar("sata_rpart", DEFAULT_ROOTFS_SATA_PART);
#endif

	/* Generate hostname out of module name and last
	 * three bytes of MAC address
	 */
	eth_getenv_enetaddr("ethaddr", enetaddr);
	sprintf(temp, "%s-%02X%02X%02X", convert2upper(modulename),
		enetaddr[3], enetaddr[4], enetaddr[5]);
	ret |= create_dynvar("hostname",temp);

	ret |= create_dynvar("ip", "ip=${ipaddr}:${serverip}:${gatewayip}:" \
			     "${netmask}:${hostname}:eth0:off");

	/* Calculate loadaddr basing on available RAM */
	sprintf(temp, "0x%x", CONFIG_LOADADDR);
	ret |= create_dynvar("loadaddr", temp);

	/* Calculate loadaddr_initrd as an offset to loadaddr */
	sprintf(temp, "0x%x", CONFIG_LOADADDR +	CONFIG_INITRD_LOADADDR_OFFSET);
	ret |= create_dynvar("loadaddr_initrd", temp);

	sprintf(temp, "uImage-%s", platformname);
	ret |= create_dynvar("kimg", temp);

	sprintf(temp, "/exports/nfsroot-%s", platformname);
	ret |= create_dynvar(NPATH, temp);

#ifdef CONFIG_AUTOLOAD_BOOTSCRIPT
	ret |= create_dynvar("loadbootsc", "yes");

	sprintf(temp, "%s-bootscript", platformname);
	ret |= create_dynvar("bootscript", temp);
#endif
#ifdef CONFIG_CMD_MMC
	ret |= create_dynvar("mmc_rpart",	DEFAULT_ROOTFS_MMC_PART);
#endif
#if defined(CONFIG_CMD_USB) || defined(CONFIG_CMD_MMC)
	ret |= create_dynvar("rootdelay", MK_STR(ROOTFS_DELAY));
#endif
	ret |= create_dynvar(SMTD, "");
	ret |= create_dynvar(SNFS, "root=/dev/nfs nfsroot=${serverip}:");
#ifdef CONFIG_UBOOT_SPLASH
	ret |= create_dynvar("simg", "splash.bmp");
#endif
	ret |= create_dynvar("std_bootarg", "");

	/* U-Boot file name */
#ifdef CONFIG_UBOOT_IMAGE_NAME
	strcpy(temp, CONFIG_UBOOT_IMAGE_NAME);		/* custom name */
#else
	sprintf(temp, "u-boot-%s", platformname);
#if defined(CONFIG_CCIMX5_SDRAM_128MB) || defined(CONFIG_CPX2_SDRAM_128MB)
	SAFE_STRCAT(temp, "_128sdram");
#endif
#ifdef CONFIG_CMD_BOOTSTREAM
#ifdef CONFIG_HAB_ENABLED
	SAFE_STRCAT(temp, "-ivt");
#endif
	SAFE_STRCAT(temp, ".sb");
#else
	SAFE_STRCAT(temp, ".bin");
#endif /* CONFIG_CMD_BOOTSTREAM */
#endif /* CONFIG_UBOOT_IMAGE_NAME */
	ret |= create_dynvar("uimg", temp);

#ifdef CONFIG_CMD_USB
	ret |= create_dynvar("usb_rpart", DEFAULT_ROOTFS_USB_PART);
#endif
#ifdef VIDEO_DISPLAY
	ret |= create_dynvar(VIDEO_VAR, VIDEO_DISPLAY);
	ret |= create_dynvar(FBPRIMARY_VAR, VIDEO_VAR);
#endif
#ifdef VIDEO_DISPLAY_2
	ret |= create_dynvar(VIDEO2_VAR, VIDEO_DISPLAY_2);
#endif

	/* Device Tree */
	ret |= create_dynvar("fdtaddr", MK_STR(CONFIG_FDT_LOADADDR));
	sprintf(temp, "%s%s%s", CONFIG_FDT_PREFIX, platformname,
		CONFIG_FDT_SUFFIX);
	ret |= create_dynvar("fdtimg", temp);

	return ret;
}

void generate_prompt(void)
{
	extern char sys_prompt[];
	char temp[PATH_MAXLEN];

	sprintf(temp, "%s %s ", convert2upper(modulename),
		CONFIG_PROMPT_SEPARATOR);
	strncpy(sys_prompt, temp, CONFIG_PROMPT_MAXLEN);
}

/* This is a function that is called early after doing the low
 * level initialization of the hardware. It is a place holder for
 * executing common initialization code for all Digi platforms
 */
int bsp_init(void)
{
	int ret = 0;

	generate_platformname();
	generate_prompt();
	ret = generate_dynamic_vars();

	return ret;
}

unsigned int get_total_ram(void)
{
	int i;
	unsigned long val = 0;

	for (i=0; i < CONFIG_NR_DRAM_BANKS; i++)
		val += gd->bd->bi_dram[i].size;

	return val;
}
#endif	/* CONFIG_CMD_BSP */
