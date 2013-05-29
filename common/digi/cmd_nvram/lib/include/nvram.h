/*
 *  nvram/include/nvram.h
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
 *  !Descr:      Provides the NVRAM functions to access MAC, IP etc.
 */

#ifndef NVRAM_H
#define NVRAM_H

#include "nvram_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ********** constants ********** */

#define NV_MAGIC_CRITICAL	"NVRAMC"
#define NV_MAX_PARTITIONS	10
#define NV_MAX_ETH_DEVICES	2
#define NV_PART_NAME_LENGTH	20
#define NV_PRODUCT_LENGTH	20
#define NV_SERIAL_LENGTH	12
#define NV_RESERVED_CRITICAL_SIZE 2048

#define WCD_MAGIC		"WCALDATA"
#define WCD_MAX_CAL_POINTS	8
#define WCD_CHANNELS_BG		14
#define WCD_CHANNELS_A		35

/* Default system partition names (OS or RootFS) */
#define PART_NAME_LINUX		"Kernel"
#define PART_NAME_WINCE		"Kernel"
#define PART_NAME_NETOS		"NET+OS-Kernel"
#define PART_NAME_ROOTFS	"RootFS"

/* ********** typedefs ********** */

/*! \brief Image types supported. */
typedef enum {
        NVCL_ORIGINAL = 0,          /*! original image */
        NVCL_MIRROR   = 1,          /*! mirror image  */
        NVCL_WORKCOPY,              /*! for setting data */
        NVCL_LAST,                  /*! for defining array sizes for nv_critical_loc_e */
} nv_critical_loc_e;

/*! Originally: The operating system for which an operation should be performed
 *  though it can also define the functionality of a section in the NVRAM */
typedef enum {
        NVOS_NONE,              /*! no operating system */
        NVOS_CRITICAL,          /*! for all (critical parameter) */
        NVOS_OS_META,           /*! metainfo for OS configuration table */
        NVOS_UBOOT,             /*! U-Boot Environment variables  */
        NVOS_LINUX,             /*! Linux settings */
        NVOS_EBOOT,             /*! WinCE EBoot Loader */
        NVOS_WINCE,
        NVOS_NETOS,
        NVOS_UNKNOWN,           /*! Operating System not known, store it in flash  */
        NVOS_APPL,              /*! so user can store his data */
        NVOS_NETOS_LOADER,      /*! NET+OS rom.bin */
        NVOS_USER_DEFINED,      /*! User defined partition table */
        NVOS_WCAL_DATA,         /*! Wireless calibration data */
        NVOS_ANDROID,		/*! Android settings */
        NVOS_DUAL_BOOT,		/*! Dual boot parameters */
        NVOS_PROD_INFO,		/*! Product info parameters */
	NVOS_LCD_CONFIG,	/*! LCD configuration */
	NVOS_LAST               /*! for defining array sizes for nv_os_type_e */
} nv_os_type_e;

/*! \brief Defines the usage of the partition. */
/*! Enhanced checks can be performed if the type is known*/
typedef enum {
        NVPT_UBOOT,             /*! U-Boot image */
        NVPT_NVRAM,             /*! NVRAM table for U-Boot environment. The
                                 *  first one is used. */
        NVPT_FPGA,              /*! FPGA image needed by U-Boot  */
        NVPT_LINUX,             /*! Linux kernel  */
        NVPT_EBOOT,             /*! WinCE eboot  */
        NVPT_WINCE,             /*! WinCE kernel */
	NVPT_NETOS,             /*! NetOS kernel  */
        NVPT_FILESYSTEM,        /*! Linux rootfs (rootfs or initrd)  */
        NVPT_WINCE_REGISTRY,    /*! WindCE Registry  */
        NVPT_UNKNOWN,           /*! Other usage */
        NVPT_SPLASH_SCREEN,     /*! splash screen contents  */
        NVPT_NETOS_LOADER,      /*! NET+OS rom.bin loader */
        NVPT_NETOS_NVRAM,       /*! NET+OS NVRAM partition */
        NVPT_RESCUE_LINUX,      /*! Rescue Linux Kernel: DEPRECATED DO NOT USE! */
        NVPT_RESCUE_FILESYSTEM, /*! Rescue rootfs or initrd: DEPRECATED DO NOT USE! */
        NVPT_RESCUE_WINCE,      /*! Rescue WinCE kernel: DEPRECATED DO NOT USE! */
        NVPT_BOOTSTREAM,	/*! Bootstream partition */
        NVPT_LAST               /*! for defining array sizes for nv_part_type_e  */
} nv_part_type_e;

/*! \brief Defines the filesystem that is used on that partition. */
/*! Enhanced checks can be performed if the type is known*/
typedef enum {
        NVFS_NONE,              /*! Not used for filesystem storage. */
        NVFS_JFFS2,             /*! JFFS2 */
        NVFS_CRAMFS,            /*! CRAMFS */
        NVFS_INITRD,            /*! InitRD */
        NVFS_FLASHFX,           /*! FlashFX */
	NVFS_UNKNOWN,           /*! Unknown */
        NVFS_YAFFS2,            /*! YAFFS/YAFFS2 */
        NVFS_EXFAT,		/*! Extended FAT */
        NVFS_SQUASHFS,          /*! SQUASHFS */
	NVFS_ROMFS,             /*! ROMFS */
	NVFS_UBIFS,             /*! UBIFS */
	NVFS_LAST               /*! for defining array sizes for nv_fs_type_e  */
} nv_fs_type_e;

/*! \brief Error codes of the library */
typedef enum {
        NVE_GOOD = 0,           /*! no error happened */
        NVE_NO_DEV,             /*! no device */
        NVE_IO,                 /*! I/O error */
        NVE_CRC,                /*! CRC error */
        NVE_MIRROR,             /*! Mirror image failure */
        NVE_NO_SPACE,           /*! no space for operation */
        NVE_NOT_INITIALIZED,    /*! not initialized yet */
        NVE_NOT_IMPLEMENTED,    /*! feature not implemented yet */
        NVE_VERSION,            /*! for defining array sizes for nv_error_e  */
        NVE_MAGIC,              /*! wrong magic */
        NVE_MATCH,              /*! image doesn't match */
        NVE_INVALID_ARG,        /*! invalid/unknown argument */
        NVE_WRONG_VALUE,        /*! wrong value */
        NVE_NOT_FOUND  ,        /*! not found */
        NVE_LAST                /*! for defining array sizes for nv_error_e  */
} nv_error_e;

/*! \brief How image errors should be corrected. */
typedef enum {
        NVR_MANUAL,             /* no repair */
        NVR_AUTO,               /* automatically repair by copying good image.*/
        NVR_IGNORE,             /* ignore state of NVRAM */
} nv_repair_e;

/* big endian. setting it to 0.0.0.0 means undefined. */
typedef uint32_t nv_ip_t;

#pragma pack(push)
typedef struct {
        union {
                uchar_t c[6];
                uint32_t for_alignment[2];
        } u;
} nv_mac_t;

/* All parameters are stored in NVRAM, so don't
 * let the compiler mess around with sizes and alignments.
 * Alinment is 64bit, so it works even on the x86 host.*/

/*! \brief The module static parameters. */
/*! These parameters should be set only in production*/
typedef struct nv_param_module_id {
        /*! \brief Product Type, e.g. 0363 or 55001167- */
        char_t  szProductType[ NV_PRODUCT_LENGTH ];
        /*! \brief Serial Number, e.g. B06500001*/
        char_t  szSerialNr[ NV_SERIAL_LENGTH ];
        /*! \brief Revision of the module. Hex Coded  */
        uint32_t uiRevision;
        /*! \brief Patchlevel of the module  */
        uint32_t uiPatchLevel;
        /*! \brief MAC address. 0 is internal ethernet,
            1 is WiFi or reserved  */
        nv_mac_t axMAC[ NV_MAX_ETH_DEVICES ];
        /*! \brief who knows what comes?  */
        uint32_t auiReserved[ 2 ];
} nv_param_module_id_t;

/*! \brief The module device dependant ip parameters. */
typedef struct nv_param_ip_device {
        /*! \brief the IP address, e.g. 192.168.42.30 */
        nv_ip_t uiIP;
        /*! \brief the netmaks, e.g. 255.255.255.0 */
        nv_ip_t uiNetMask;
        struct {
                /*! \brief 1 if DHCP should be used on this device */
                uint8_t bDHCP;
                uint8_t acReserved[ 3 ];
        } flags;
        /*! \brief who knows what comes? IPv6? Reserve at least 128 bit */
        uint32_t  auiReserved[ 5 ];
} nv_param_ip_device_t;

/*! \brief The module ip parameters that are not device dependant. */
typedef struct nv_param_ip {
        /*! \brief the gateway's IP */
        nv_ip_t uiIPGateway;
        /*! \brief the DNS IP addresses */
        nv_ip_t auiIPDNS[ 2 ];
        /*! \brief for the TFTP/NFS server. */
        nv_ip_t uiIPServer;
        /*! \brief for a NetConsole server. */
        nv_ip_t uiIPNetConsole;
        /*! \brief 0 for the internal, a second one for the external */
        nv_param_ip_device_t axDevice[ NV_MAX_ETH_DEVICES ];
        /*! \brief who knows what comes? IPv6? reserve at least one. */
        uint32_t  auiReserved[ 5 ];
} nv_param_ip_t;

/*! \brief The parameters for one partition. Overlapping is allowed. */
typedef struct nv_param_part {
        /*! \brief the name that is used e.g. in /proc/mtd. */
        char_t   szName[ NV_PART_NAME_LENGTH ];

        /*! \brief the flash chip this partition entry is for. */
        /*! 0 is the onboard boot flash */
        uint32_t uiChip;
        /*! \brief the start address of the partition on this chip. */
        uint64_t ullStart;
        /*! \brief the size of the partition. */
        uint64_t ullSize;
        /*! \brief the type of the partition. */
        nv_part_type_e eType;

        struct {
                /*! \brief this entry is fixed and can't be changed. */
                /*! Library ensures that bFixed entries are not overwritten.
                 */
                uint8_t  bFixed;

                /*! \brief The partition itself is read-only.
                  Only true if partition is NVPT_FILESYSTEM */
                uint8_t  bReadOnly;

                uint8_t  aucReserved[ 6 ];

                /* the filesystem flags */
                struct {
                        /*! \brief The filesystem is mounted read-only. */
                        uint8_t  bMountReadOnly;
                        /*! \brief 1 if this is the rootfilesystem */
                        uint8_t  bRoot;

                        uint8_t  aucReserved2[ 2 ];

                        /*! \brief the filesystem type of the partition. */
                        /* Is NVFS_NONE if the partition type is not NVPT_FILESYSTEM.*/
                        nv_fs_type_e eType;

                        /*! \brief the file systems version. 0 if not used. */
                        uint32_t  uiVersion;
                } fs;
        } flags;
        uint32_t  auiReserved[ 6 ];
} nv_param_part_t;

/*! \brief The partition table. */
typedef struct nv_param_part_table {
        /*! \brief the number of partitions valid. Holes are not allowed. */
        uint32_t  uiEntries;
        /*! \brief who knows what comes? */
        uint32_t  auiReserved[ 3 ];
        /*! \brief the partition entries. 64bit aligned. */
        nv_param_part_t axEntries[ NV_MAX_PARTITIONS ];
} nv_param_part_table_t;

/*! \brief The os entry for combining different operating systems
     configuration*/
/*! Multiple OS share config parameters in one NVRAM erase block.
 *  The first NVPT_NVRAM entry is taken. And it must be */
typedef struct nv_param_os_cfg {
        /*! \brief the type of the os. */
        nv_os_type_e eType;
        /*! \brief the start offset inside that partition for the OS.*/
        /*! It's only 32bit because there won't be an NVRAM partition > 4GB.*/
        uint32_t uiStart;
        /*! \brief the size of the os configuration params */
        uint32_t uiSize;
        /*! \brief who knows what comes? */
        uint32_t auiReserved[ 2 ];
} nv_param_os_cfg_t;

/*! \brief The os table for combining different operating systems configuration
 */
typedef struct nv_param_os_cfg_table {
        /*! \brief the number of os configuration parameters used.*/
        /*! Holes are not allowed. */
        uint32_t  uiEntries;
        /*! \brief who knows what comes? */
        uint32_t  auiReserved[ 3 ];
        /*! \brief The table pointing to the os configuration parameters */
        nv_param_os_cfg_t axEntries[ 8 ];
} nv_param_os_cfg_table_t;


/*! \brief The wireless calibration information
 */
typedef struct nv_wcd_header {
	char_t	magic_string[8];	/* WCALDATA */
	char_t	ver_major;		/* Major version in ascii */
	char_t	ver_minor;		/* Minor version in ascii */
	uint16_t hw_platform;		/* Hardware Platform used for calibration */
	uint8_t	numcalpoints;		/* Number of points per curve */
	uint8_t	padding[107];		/* Reserved for future use */
	uint32_t wcd_len;		/* Total length of the data section */
	uint32_t wcd_crc;		/* Data section crc32 */
} nv_wcd_header_t;


typedef struct wcd_point {
	int16_t	 out_power;		/* Output Power */
	uint16_t adc_val;		/* Measured ADC val */
	uint8_t  power_index;		/* Airoha Power Index */
	uint8_t  reserved[3];		/* For future use */
} wcd_point_t;

typedef struct wcd_curve {
	uint8_t  max_power_index;	/* Airoha Max Power Index */
	uint8_t  reserved[3];		/* Resered for future use */
	wcd_point_t points[WCD_MAX_CAL_POINTS];	/* Calibration curve points */
} wcd_curve_t;

typedef struct wcd_data {
	nv_wcd_header_t  header;
	wcd_curve_t cal_curves_bg[WCD_CHANNELS_BG][2];
        wcd_curve_t cal_curves_a[WCD_CHANNELS_A];
} wcd_data_t;

#define NV_DUALB_MAGIC		"DUALBOOT"
#define NV_DUALB_VERSION 	1	/* increase number when structure is changed */

#define NV_PRODINFO_MAGIC	"PRODINFO"
#define NV_PRODINFO_VERSION 	1	/* increase number when structure is changed */

/*! \brief The dual boot information
 */
typedef struct nv_dualb_header {
	char_t	magic_string[8];	/* DUALBOOT */
	uint32_t version;		/* This struct version */
	uint8_t padding[68];		/* for future */
} nv_dualb_header_t;

typedef struct nv_dualb_data {
	uint8_t	avail[2];	/* whether there is a firmware available at
				 * the n'th system partition */

	uint8_t verified[2];	/* whether the n'th system partition was
				 * verified after having been updated */

	uint8_t boot_part;	/* The system partition that must be booted */

	uint8_t last_updated;	/* This is updated when a firmware update is
				 * done to match the most recently updated
				 * system partition. By default it is the first
				 * one. */

	uint8_t boot_attempts;	/* This is a placeholder for storing the boot
				 * attempts counter in platforms that don't
				 * have persistent memory other than Flash.
				 * This field is supposed to be used in the
				 * platform specific implementation for handling
				 * the boot attempts counter.
				 */

	uint8_t mode_peer;	/* This value is set by U-Boot configuration.
				 * This is just a placeholder for the kernel
				 * to consult whether in mode peer or rescue
				 */

	uint8_t guarantee_perboot;	/* This value is set by U-Boot configuration.
				 * This is just a placeholder for the kernel
				 * to consult whether in mode per boot guarantee
				 * or after update
				 */
	uint8_t reserved[59];
} nv_dualb_data_t;

typedef struct nv_dualb {
	nv_dualb_header_t	header;
	nv_dualb_data_t		data;
} nv_dualb_t;

#define PROD_INFO_DATA_SIZE		8192

typedef struct nv_prodinfo_t {
	uint32_t		crc;
	char			data[PROD_INFO_DATA_SIZE - sizeof(uint32_t)];
}nv_prodinfo_t;

#define NV_LCD_CONFIG_MAGIC		"LCD_CONF"
#define NV_LCD_CONFIG_VERSION 		2	/* increase number when structure is changed */

typedef struct nv_lcd_config_header_t {
	char_t		magic_string[8];	/* LCD_CONF */
	uint32_t	version;		/* This struct version */
	uint32_t	crc;			/* Crc of configuration sections */
	uint8_t		lcd1_valid;		/* LCD 1's configuration valid */
	uint8_t		lcd2_valid;		/* LCD 2's configuration valid */
	uint8_t		padding[110];
} nv_lcd_config_header_t;

typedef struct nv_lcd_config_data_t {
	struct {				/* Exact copy of the struct fb_videomode */
		const char	*name;		/* not used */
		uint32_t	refresh;	/* optional */
		uint32_t	xres;
		uint32_t	yres;
		uint32_t	pixclock;
		uint32_t	left_margin;
		uint32_t	right_margin;
		uint32_t	upper_margin;
		uint32_t	lower_margin;
		uint32_t	hsync_len;
		uint32_t	vsync_len;
		uint32_t	sync;
		uint32_t	vmode;
		uint32_t	flag;
	} video_mode;
	uint8_t			is_bl_enable_func;
	uint8_t			padding1[3];
	struct {
		uint32_t	pix_data_offset;
		uint32_t	pix_clk_up;
		uint32_t	pix_clk_down;
	} pix_cfg;
	uint8_t			padding2[56];
} nv_lcd_config_data_t;

typedef struct nv_lcd_config_t {
	nv_lcd_config_header_t	header;
	nv_lcd_config_data_t	lcd1;
	nv_lcd_config_data_t	lcd2;
} nv_lcd_config_t;

/*! \brief The NVRAM structure. */
typedef struct nv_critical {
        struct {
                /*! \brief NV_MAGIC_CRITICAL */
                char_t   szMagic[ 8 ];
                /*! \brief the major version of these parameters. */
                /*! There are incompatible changes if major version is increased. */
                uint32_t  uiVerMajor;
                /*! \brief the minor version of these parameters. */
                /*! Only reserved entries are defined. */
                uint32_t  uiVerMinor;

                /* \brief everything in p can change from major version to
                   major version. */
                struct {
                        /*! \brief the module identification */
                        nv_param_module_id_t  xID;
                        /*! \brief the partition table */
                        nv_param_part_table_t xPartTable;
                        /*! \brief the ip configuration */
                        nv_param_ip_t         xIP;
                        /*! \brief the os configuration block */
                        nv_param_os_cfg_table_t xOSTable;
                        /*! \brief who knows what comes? (64 bytes remaining )*/
			nv_mac_t eth1addr; /*! \brief 8 bytes */
			nv_param_ip_device_t eth1dev; /*! \brief 32 bytes */
			nv_mac_t btaddr1; /*! \brief 8 bytes */
                        /*! \brief who knows what comes? (16 bytes remaining )*/
                        uint32_t  auiReserved[ 4 ];
                } p;
        } s;

        /* \brief the checksum for s */
        crc32_t  uiCRC32;
        uint32_t uiPadding;
} nv_critical_t;

/*! \brief The information of the OS table. */
typedef struct nv_os_meta {
        /*! \brief the checksum for begin to end of erase block*/
        crc32_t  uiCRC32;
        uint32_t auiReserved[ 31 ];
} nv_os_meta_t;

#pragma pack(pop)

typedef struct nv_info {
        /*! \brief 1 if at least ORIGINAL or MIRROR is good */
        uint8_t bGood;
        uint8_t bAnyBad;
        uint8_t bFlashOriginalGood;
        uint8_t bFlashMirrorGood;
        uint8_t bFlashMirrorMatch;
        uint8_t bOriginalGood;
        uint8_t bMirrorGood;
        uint8_t bMirrorMatch;
        uint8_t bOSGood;
        uint8_t bOSOriginalGood;
        uint8_t bDefault;       /* enabled if default's are used */
} nv_info_t;

extern char g_markBadBlocks;

/* ********** prototypes ********** */

/*@-exportlocal@*/ // this is a library include filed. some code may be unused
extern int NvInit( nv_repair_e eRepair );
extern int NvFinish( void );
extern int NvEnableOutput( char bEnable );
extern nv_error_e NvError( void );
extern nv_error_e NvErrorMsg( /*@out@*/ /*@null@*/ const char** pszError,
                           /*@out@*/ /*@null@*/ const char** pszWhat,
                           /*@out@*/ /*@null@*/ const char** pszFunc,
                           /*@out@*/ /*@null@*/ const char** pszFile,
                           /*@out@*/ /*@null@*/ int* piLine );
extern void NvGetLibVersion( /*@out@*/ uint32_t* puiVerMajor,
                          /*@out@*/ uint32_t* puiVerMinor );
extern void NvGetVersion( /*@out@*/ uint32_t* puiVerMajor,
                          /*@out@*/ uint32_t* puiVerMinor );

extern const struct nv_info* NvInfo( void );
extern int NvPrintStatus( void );
extern int NvSave( void );
extern int NvWorkcopyUpdateCRC32( void );

extern int NvWorkcopyReset( void );
extern int NvCriticalGet(
        /*@out@*/ struct nv_critical** ppCritical );
extern int NvParamPartFind(
        /*@out@*/ const struct nv_param_part** ppPart,
        nv_part_type_e ePartType,
        nv_fs_type_e   eFSType,
        char           bRoot,
        int            iCount );
extern int NvCriticalPartAdd( const struct nv_param_part* pNewPart );
extern int NvCriticalPartDelete( uint32_t uiPartition );
extern int NvCriticalPartReset( nv_os_type_e eForOS );

extern int NvOSCfgAdd( nv_os_type_e eType, size_t iSize );
extern int NvOSCfgGet( nv_os_type_e eType,/*@out@*/  void* pvBuf,
                       size_t iMaxSize, /*@out@*/ size_t* piSize );
extern int NvOSCfgSet( nv_os_type_e eType, const void*  pvBuf,  size_t  iSize );
extern int NvOSCfgFind( /*@out@*/ struct nv_param_os_cfg* pCfg,
                        nv_os_type_e eType );
extern int NvCmdLine( int argc, const char* argv[] );
extern int NvPrintHelp( void );

extern const char* NvToStringMAC( const nv_mac_t xMAC );
extern int NvToMAC( /*@out@*/ nv_mac_t* pMAC, const char* szMAC );
extern const char* NvToStringIP( nv_ip_t uiIP );
extern int NvToIP( /*@out@*/ nv_ip_t* pIP, const char* szIP );
extern const char* NvToStringPart( nv_part_type_e ePart );
extern int NvToPart( /*@out@*/ nv_part_type_e* pePart, const char* szOS );
extern const char* NvToStringFS( nv_fs_type_e eFS );
extern int NvToFS( /*@out@*/ nv_fs_type_e* peFS, const char* szFS );
extern const char* NvToStringOS( nv_os_type_e eOS );
extern int NvToOS( /*@out@*/ nv_os_type_e* peOS, const char* szOS );
extern const char* NvToStringLoc( nv_critical_loc_e eLoc );
/*@+exportlocal@*/
extern const char* NvToStringSize64( const uint64_t ullVal );
extern int NvToSize64( /*@out@*/ uint64_t* pullVal, const char* szVal );
extern int NvPrivAnyFlashImageGood(void);
extern size_t NvOSDefaultSectionSize(nv_os_type_e eos);
extern int NvOSResetDefaults(nv_param_os_cfg_t *xCfg);
extern int NvInitSection(int argc, const char* argv[]);

#ifdef CFG_HAS_WIRELESS
extern int NvPrivWirelessCalDataCheck( wcd_data_t *pWCal );
extern int NvPrivWCDGetFromFlashAndSetInNvram( void );
extern int NvPrivWCDSaveInFlash( wcd_data_t *pWCal );
extern int NvPrivWCDSetInNvram( wcd_data_t *pWCal );
#endif

/* Dual boot */
extern int NvPrivDualBootRead(nv_dualb_t *db);
extern int NvPrivDualBootWrite(nv_dualb_t *db);
extern void DualBootReset(nv_dualb_t *dualb);
/* Product info */
extern void ProdinfoReset(nv_prodinfo_t *prodinfo);

/* LCD configuration */
extern void LCDConfigReset(nv_lcd_config_t *lcdconfig);
extern void LCDConfigAdd(void);
extern int LCDConfigRead(nv_lcd_config_t *lcdconfig);
extern int LCDConfigWrite(nv_lcd_config_t *lcdconfig);

#ifdef __cplusplus
}
#endif

#endif /* NVRAM_H */
