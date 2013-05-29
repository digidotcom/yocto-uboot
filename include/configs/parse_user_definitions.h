#ifndef __PARSE_USER_DEFINITIONS_H
#define __PARSE_USER_DEFINITIONS_H

#include <configs/userconfig.h>

#if 0
#if defined(CONFIG_ENABLE_CONSOLE_GPIO) && defined(CONFIG_CONSOLE_ENABLE_GPIO_STATE)
#define ENABLE_CONSOLE_GPIO 		CONFIG_ENABLE_CONSOLE_GPIO
#define CONSOLE_ENABLE_GPIO_STATE 	CONFIG_CONSOLE_ENABLE_GPIO_STATE
#endif

#ifdef CONFIG_PARTITION

#ifdef CONFIG_PARTITION_FIXED_2
#define PARTITION_FIXED_2       1
#else
#define PARTITION_FIXED_2       0
#endif

#ifdef CONFIG_PARTITION_READONLY_2
#define PARTITION_READONLY_2    1
#else
#define PARTITION_READONLY_2    0
#endif

#ifdef CONFIG_PARTITION_ROOTFS_2
#define PARTITION_ROOTFS_2      1
#else
#define PARTITION_ROOTFS_2      0
#endif

#if ((CONFIG_PARTITION_SIZE_2 != 0 && defined(CONFIG_PARTITION_NAME_2) && \
	defined(CONFIG_PARTITION_2)) || defined(CONFIG_HAVE_FPGA))
#define CONFIG_HAVE_PARTITION_2
#endif
/* ------------------- */

#ifdef CONFIG_PARTITION_FIXED_3
#define PARTITION_FIXED_3       1
#else
#define PARTITION_FIXED_3       0
#endif

#ifdef CONFIG_PARTITION_READONLY_3
#define PARTITION_READONLY_3    1
#else
#define PARTITION_READONLY_3    0
#endif

#ifdef CONFIG_PARTITION_ROOTFS_3
#define PARTITION_ROOTFS_3      1
#else
#define PARTITION_ROOTFS_3      0
#endif

#if CONFIG_PARTITION_SIZE_3 != 0 && defined(CONFIG_PARTITION_NAME_3 ) && \
	defined(CONFIG_PARTITION_3) && defined(CONFIG_HAVE_PARTITION_2)
#define CONFIG_HAVE_PARTITION_3
#endif
/* ------------------- */

#ifdef CONFIG_PARTITION_FIXED_4
#define PARTITION_FIXED_4       1
#else
#define PARTITION_FIXED_4       0
#endif

#ifdef CONFIG_PARTITION_READONLY_4
#define PARTITION_READONLY_4    1
#else
#define PARTITION_READONLY_4    0
#endif

#ifdef CONFIG_PARTITION_ROOTFS_4
#define PARTITION_ROOTFS_4      1
#else
#define PARTITION_ROOTFS_4      0
#endif

#if CONFIG_PARTITION_SIZE_4 != 0 && defined(CONFIG_PARTITION_NAME_4) && \
	defined(CONFIG_PARTITION_4) && defined(CONFIG_HAVE_PARTITION_3)
#define CONFIG_HAVE_PARTITION_4
#endif
/* ------------------- */

#ifdef CONFIG_PARTITION_FIXED_5
#define PARTITION_FIXED_5       1
#else
#define PARTITION_FIXED_5       0
#endif

#ifdef CONFIG_PARTITION_READONLY_5
#define PARTITION_READONLY_5    1
#else
#define PARTITION_READONLY_5    0
#endif

#ifdef CONFIG_PARTITION_ROOTFS_5
#define PARTITION_ROOTFS_5      1
#else
#define PARTITION_ROOTFS_5      0
#endif

#if CONFIG_PARTITION_SIZE_5 != 0 && defined(CONFIG_PARTITION_NAME_5) && \
	defined(CONFIG_PARTITION_5) && defined(CONFIG_HAVE_PARTITION_4)
#define CONFIG_HAVE_PARTITION_5
#endif
/* ------------------- */

#ifdef CONFIG_PARTITION_FIXED_6
#define PARTITION_FIXED_6       1
#else
#define PARTITION_FIXED_6       0
#endif

#ifdef CONFIG_PARTITION_READONLY_6
#define PARTITION_READONLY_6    1
#else
#define PARTITION_READONLY_6    0
#endif

#ifdef CONFIG_PARTITION_ROOTFS_6
#define PARTITION_ROOTFS_6      1
#else
#define PARTITION_ROOTFS_6      0
#endif

#if CONFIG_PARTITION_SIZE_6 != 0 && defined(CONFIG_PARTITION_NAME_6) && \
	defined(CONFIG_PARTITION_6) && defined(CONFIG_HAVE_PARTITION_5)
#define CONFIG_HAVE_PARTITION_6
#endif

#endif /*PARTITION*/

/* ---------------------------------- */
/* COMMANDS			      */
#ifndef CONFIG_CMD_BDI
#define CONFIG_CMD_BDI		0
#endif
#ifndef CONFIG_CMD_LOADS
#define CONFIG_CMD_LOADS	0
#endif
#ifndef CONFIG_CMD_LOADB
#define CONFIG_CMD_LOADB	0
#endif
#ifndef CONFIG_CMD_IMI             
#define CONFIG_CMD_IMI		0
#endif
#ifndef CONFIG_CMD_CACHE
#define CONFIG_CMD_CACHE	0
#endif
#ifndef CONFIG_CMD_FLASH
#define CONFIG_CMD_FLASH	0
#endif
#ifndef CONFIG_CMD_MEMORY
#define CONFIG_CMD_MEMORY	0
#endif
#ifndef CONFIG_CMD_NET
#define CONFIG_CMD_NET		0
#endif
#ifndef CONFIG_CMD_ENV
#define CONFIG_CMD_ENV		0
#endif
#ifndef CONFIG_CMD_KGDB
#define CONFIG_CMD_KGDB		0
#endif
#ifndef CONFIG_CMD_PCMCIA
#define CONFIG_CMD_PCMCIA	0
#endif
#ifndef CONFIG_CMD_IDE
#define CONFIG_CMD_IDE		0
#endif
#ifndef CONFIG_CMD_PCI
#define CONFIG_CMD_PCI		0
#endif
#ifndef CONFIG_CMD_IRQ      
#define CONFIG_CMD_IRQ		0
#endif
#ifndef CONFIG_CMD_BOOTD
#define CONFIG_CMD_BOOTD	0
#endif
#ifndef CONFIG_CMD_CONSOLE
#define CONFIG_CMD_CONSOLE	0
#endif
#ifndef CONFIG_CMD_EEPROM
#define CONFIG_CMD_EEPROM	0
#endif
#ifndef CONFIG_CMD_ASKENV
#define CONFIG_CMD_ASKENV	0
#endif
#ifndef CONFIG_CMD_RUN
#define CONFIG_CMD_RUN		0
#endif
#ifndef CONFIG_CMD_ECHO
#define CONFIG_CMD_ECHO		0
#endif
#ifndef CONFIG_CMD_I2C
#define CONFIG_CMD_I2C		0
#endif
#ifndef CONFIG_CMD_REGINFO
#define CONFIG_CMD_REGINFO	0
#endif
#ifndef CONFIG_CMD_IMMAP
#define CONFIG_CMD_IMMAP	0
#endif
#ifndef CONFIG_CMD_DATE
#define CONFIG_CMD_DATE		0
#endif
#ifndef CONFIG_CMD_DHCP
#define CONFIG_CMD_DHCP		0
#endif
#ifndef CONFIG_CMD_BEDBUG
#define CONFIG_CMD_BEDBUG	0
#endif
#ifndef CONFIG_CMD_FDC
#define CONFIG_CMD_FDC		0
#endif
#ifndef CONFIG_CMD_SCSI
#define	CONFIG_CMD_SCSI		0
#endif
#ifndef CONFIG_CMD_AUTOSCRIPT
#define CONFIG_CMD_AUTOSCRIPT	0
#endif
#ifndef CONFIG_CMD_MII
#define CONFIG_CMD_MII		0
#endif
#ifndef CONFIG_CMD_SETGETDCR
#define CONFIG_CMD_SETGETDCR	0
#endif
#ifndef CONFIG_CMD_BSP
#define CONFIG_CMD_BSP		0
#endif

#ifndef CONFIG_CMD_ELF
#define CONFIG_CMD_ELF		0
#endif
#ifndef CONFIG_CMD_MISC
#define CONFIG_CMD_MISC		0
#endif
#ifndef CONFIG_CMD_USB
#define CONFIG_CMD_USB		0
#endif
#ifndef CONFIG_CMD_DOC
#define CONFIG_CMD_DOC		0
#endif
#ifndef CONFIG_CMD_JFFS2
#define CONFIG_CMD_JFFS2	0
#endif
#ifndef CONFIG_CMD_DTT
#define CONFIG_CMD_DTT		0
#endif
#ifndef CONFIG_CMD_SDRAM
#define CONFIG_CMD_SDRAM	0
#endif
#ifndef CONFIG_CMD_DIAG
#define CONFIG_CMD_DIAG		0
#endif
#ifndef CONFIG_CMD_FPGA
#define CONFIG_CMD_FPGA		0
#endif
#ifndef CONFIG_CMD_HWFLOW
#define CONFIG_CMD_HWFLOW	0
#endif
#ifndef CONFIG_CMD_SAVES
#define CONFIG_CMD_SAVES	0
#endif
#ifndef	CONFIG_CMD_CHGMAC
#define CONFIG_CMD_CHGMAC	0
#endif
#ifndef CONFIG_CMD_SPI
#define CONFIG_CMD_SPI		0
#endif
#ifndef CONFIG_CMD_FDOS
#define CONFIG_CMD_FDOS		0
#endif
#ifndef CONFIG_CMD_VFD
#define CONFIG_CMD_VFD		0
#endif
#ifndef CONFIG_CMD_NAND
#define CONFIG_CMD_NAND		0
#endif
#ifndef CONFIG_CMD_BMP
#define CONFIG_CMD_BMP		0
#endif
#ifndef CONFIG_CMD_PORTIO
#define CONFIG_CMD_PORTIO	0
#endif
#ifndef CONFIG_CMD_PING
#define CONFIG_CMD_PING		0
#endif
#ifndef CONFIG_CMD_MMC
#define CONFIG_CMD_MMC		0
#endif
#ifndef CONFIG_CMD_FAT
#define CONFIG_CMD_FAT		0
#endif
#ifndef CONFIG_CMD_IMLS
#define CONFIG_CMD_IMLS		0
#endif
#ifndef CONFIG_CMD_ITEST
#define CONFIG_CMD_ITEST	0
#endif
#ifndef CONFIG_CMD_NFS
#define CONFIG_CMD_NFS		0
#endif
#ifndef CONFIG_CMD_REISER
#define CONFIG_CMD_REISER	0
#endif
#ifndef CONFIG_CMD_CDP
#define CONFIG_CMD_CDP		0
#endif
#ifndef CONFIG_CMD_XIMG
#define CONFIG_CMD_XIMG		0
#endif
#ifndef CONFIG_CMD_UNIVERSE
#define CONFIG_CMD_UNIVERSE	0
#endif
#ifndef CONFIG_CMD_EXT2
#define CONFIG_CMD_EXT2		0
#endif
#ifndef CONFIG_CMD_SNTP
#define CONFIG_CMD_SNTP		0
#endif
#ifndef CONFIG_CMD_DISPLAY
#define CONFIG_CMD_DISPLAY	0
#endif

#define	USER_DEFINED_COMMANDS ( 0					| \
				CONFIG_CMD_BDI 			 	| \
				CONFIG_CMD_LOADS << 1 			| \
				CONFIG_CMD_LOADB << 2			| \
				CONFIG_CMD_IMI << 3			| \
				CONFIG_CMD_CACHE << 4			| \
				CONFIG_CMD_FLASH << 5			| \
				CONFIG_CMD_MEMORY << 6			| \
				CONFIG_CMD_NET << 7			| \
				CONFIG_CMD_ENV << 8			| \
				CONFIG_CMD_KGDB << 9			| \
				CONFIG_CMD_PCMCIA << 10			| \
				CONFIG_CMD_IDE << 11			| \
				CONFIG_CMD_PCI << 12			| \
				CONFIG_CMD_IRQ << 13			| \
				CONFIG_CMD_BOOTD << 14			| \
				CONFIG_CMD_CONSOLE << 15		| \
				CONFIG_CMD_EEPROM << 16			| \
				CONFIG_CMD_ASKENV << 17			| \
				CONFIG_CMD_RUN << 18			| \
				CONFIG_CMD_ECHO << 19			| \
				CONFIG_CMD_I2C << 20			| \
				CONFIG_CMD_REGINFO << 21		| \
				CONFIG_CMD_IMMAP << 22			| \
				CONFIG_CMD_DATE << 23			| \
				CONFIG_CMD_DHCP << 24			| \
				CONFIG_CMD_BEDBUG << 25			| \
				CONFIG_CMD_FDC << 26			| \
				CONFIG_CMD_SCSI << 27			| \
				CONFIG_CMD_AUTOSCRIPT << 28         	| \
				CONFIG_CMD_MII << 29 			| \
				CONFIG_CMD_SETGETDCR << 30		| \
				CONFIG_CMD_BSP << 31			| \
				CONFIG_CMD_ELF << 32			| \
				CONFIG_CMD_MISC << 33			| \
				CONFIG_CMD_USB << 34			| \
				CONFIG_CMD_DOC << 35			| \
				CONFIG_CMD_JFFS2 << 36			| \
				CONFIG_CMD_DTT << 37			| \
				CONFIG_CMD_SDRAM << 38			| \
				CONFIG_CMD_DIAG << 39			| \
				CONFIG_CMD_FPGA << 40			| \
				CONFIG_CMD_HWFLOW << 41			| \
				CONFIG_CMD_SAVES << 42			| \
				CONFIG_CMD_CHGMAC << 43			| \
				CONFIG_CMD_SPI << 44			| \
				CONFIG_CMD_FDOS << 45			| \
				CONFIG_CMD_VFD << 46			| \
				CONFIG_CMD_NAND << 47			| \
				CONFIG_CMD_BMP << 48			| \
				CONFIG_CMD_PORTIO << 49			| \
				CONFIG_CMD_PING << 50			| \
				CONFIG_CMD_MMC << 51			| \
				CONFIG_CMD_FAT << 52			| \
				CONFIG_CMD_IMLS << 53			| \
				CONFIG_CMD_ITEST << 54			| \
				CONFIG_CMD_NFS << 55			| \
				CONFIG_CMD_REISER << 56			| \
				CONFIG_CMD_CDP << 57			| \
				CONFIG_CMD_XIMG << 58			| \
				CONFIG_CMD_UNIVERSE << 59		| \
				CONFIG_CMD_EXT2 << 60			| \
				CONFIG_CMD_SNTP << 61			| \
				CONFIG_CMD_DISPLAY << 62		| \
				0 )

#if USER_DEFINED_COMMANDS != 0
#define CONFIG_COMMANDS USER_DEFINED_COMMANDS
#endif
#endif /* if 0 */
#endif /*__PARSE_USER_DEFINITIONS_H*/
