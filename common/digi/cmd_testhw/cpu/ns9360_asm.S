/*
 *  common/digi/cmd_testhw/cpu/ns9360_asm.S
 *
 *  Copyright (C) 2007 by Digi International Inc.
 *  All rights reserved.
 *
 *  This program is free software; you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License version2  as published by
 *  the Free Software Foundation.
*/
/*
 *  !Revision:   $Revision$
 *  !Author:     Markus Pietrek
 *  !Descr:      Provides
 *                   void testhw_bigendian_watchdog_reset_switch( void ).
 *  !References: [1] spi.S Rev. 1.5
*/

#include <config.h>
#if (CONFIG_COMMANDS & CFG_CMD_BSP && \
     defined(CONFIG_UBOOT_CMD_BSP_TESTHW) &&    \
     defined(CONFIG_NS9360))

#include <ns9750_sys.h>
#include <ns9750_mem.h>
        
/*! \brief switches to big-endian and performs watchdog-reset */
/*! void testhw_bigendian_watchdog_reset_switch( void );
    Will never return.
    r0 will be used as a base register, r1 as a data register
 */
.globl testhw_bigendian_watchdog_reset_switch
testhw_bigendian_watchdog_reset_switch:
        stmdb 	sp!, {r4, r5, r6, r7, r8, r9, r10, r11, ip, lr}

switch_to_big:
        /* see [1], EndianSwitchBig */

        /* set memory controller to big endian */
	ldr	r0, =( NS9750_MEM_MODULE_BASE + NS9750_MEM_CFG )
	ldr	r1, [r0]
	orr	r1, r1, #NS9750_MEM_CFG_N   /* force big endian */
	str	r1, [r0]

        /* set system controller to big endian */
	ldr	r0, =( NS9750_SYS_MODULE_BASE + NS9750_SYS_MISC )
	ldr	r1, [r0]
	orr	r1, r1, #NS9750_SYS_MISC_ENDM /* force big endian */
	str	r1, [r0]

	/* set the strapped endian bits in BBus Endian Config Register */
	ldr	r0, =( NS9750_BBUS_MODULE_BASE + NS9750_BBUS_ENDIAN_CFG )
	ldr	r1, [r0]
	ldr	r2, =( NS9750_BBUS_ENDIAN_CFG_AHBM | \
                       NS9750_BBUS_ENDIAN_CFG_USBHOST | \
                       NS9750_BBUS_ENDIAN_CFG_DMA ) /* force big endian */
	orr	r1,r1,r2
	str	r1, [r0]

	/* set endian bit in CP15 R1.*/
	ldr	r1, =0x00001005
	mrc	p15, 0, r0, c1, c0, 0	/* read value from control register (CP15 r1 into r0) */
	orr	r0, r0, #0x00000080
	mcr	p15, 0, r0, c1, c0, 0	/* write back */

        /* return */
        ldmia 	sp!, {r4, r5, r6, r7, r8, r9, r10, r11, ip, lr}
        mov     pc, lr
        
#endif /*(CONFIG_COMMANDS & CFG_CMD_BSP && defined(CONFIG_UBOOT_CMD_BSP_TESTHW) && defined(CONFIG_NS9360))*/
