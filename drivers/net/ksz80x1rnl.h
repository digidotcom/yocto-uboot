/*
 *  Copyright (C) 2011 by Digi International Inc.
 *  All rights reserved.
 *
 *  This program is free software; you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License version2  as published by
 *  the Free Software Foundation.
*/

#ifndef __KSZ80X1RNL_H
#define __KSZ80X1RNL_H

#define PHY_ID_KSZ80x1RNL	0x00221550

/* KSZ80x1RNL registers */
#define KSZ80x1_OPMODE_STRAPOV	0x16
#define KSZ80x1_INT_CTRLSTAT	0x1B
#define KSZ80x1_LINKMD_CTRLSTAT	0x1D
#define KSZ80x1_PHY_CTRL1	0x1E
#define KSZ80x1_PHY_CTRL2	0x1F

/* KSZ80x1_PHY_CTRL2 */
#define HP_AUTO_MDI		(1 << 15)
#define MDI_X_MODE		(1 << 14)
#define DISABLE_AUTO_MDI	(1 << 13)
#define FORCE_LINK_PASS		(1 << 11)
#define ENABLE_POWER_SAVE	(1 << 10)
#define INTERRUPT_ACTIVE_HIGH	(1 << 9)
#define ENABLE_JABBER_COUNTER	(1 << 8)
#define RMII_50MHZ_CLOCK	(1 << 7)
#define LED_MODE_LINK_ONLY	(1 << 4)
#define DISABLE_TRANSMITTER	(1 << 3)
#define ENABLE_REMOTE_LOOPBACK	(1 << 2)
#define DISABLE_SCRAMBLER	(1 << 0)

#endif /* __KSZ80X1RNL_H */
