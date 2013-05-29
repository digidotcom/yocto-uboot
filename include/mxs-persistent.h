/*
 * Freescale STMP378X Persistent bits manipulation driver
 *
 * Author: Pantelis Antoniou <pantelis@embeddedalley.com>
 *
 * Copyright 2011 Digi International, Inc.
 * Copyright 2008-2010 Freescale Semiconductor, Inc.
 * Copyright 2008 Embedded Alley Solutions, Inc All Rights Reserved.
 */

/*
 * The code contained herein is licensed under the GNU General Public
 * License. You may obtain a copy of the GNU General Public License
 * Version 2 or later at the following locations:
 *
 * http://www.opensource.org/licenses/gpl-license.html
 * http://www.gnu.org/copyleft/gpl.html
 */
#ifndef _MXS_PERSISTENT_H_
#define _MXS_PERSISTENT_H_

u32 persistent_reg_read(int index);
void persistent_reg_wait_settle(int index);
void persistent_reg_write(int index, u32 val);
void persistent_reg_set(int index, u32 val);
void persistent_reg_clr(int index, u32 val);
void persistent_reg_tog(int index, u32 val);

#endif /* _MXS_PERSISTENT_H_ */
