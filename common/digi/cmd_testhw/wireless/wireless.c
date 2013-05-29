/*
 *  common/digi/cmd_testhw/common/wireless.c
 *
 *  Copyright (C) 2007 by Digi International Inc.
 *  All rights reserved.
 *
 *  This program is free software; you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License version2  as published by
 *  the Free Software Foundation.
*/

#include <common.h>

#if (CONFIG_COMMANDS & CFG_CMD_BSP && \
     defined(CONFIG_UBOOT_CMD_BSP_TESTHW) && \
     defined(CFG_HAS_WIRELESS))

#include <cmd_testhw/testhw.h>
#include "calibration.h"
/*#include "fpArithmetic.h"*/
#include "vBPiper.h"
#include "gui_tst_wifi.h"
#include "commandswifi.h"

#define USAGE	"Performs wireless testing and calibration."

extern char cancel_prompt_echo;

wcd_data_t calDataObtained;

static int do_testhw_wireless(int argc, char* argv[])
{
	
	/* For loop control */
	signed char i;
	
	/* Wireless Status */
	static struct menuStatus auxMenuStatus;

	if (argc > 1)
	{
		printf("Incorrect number of arguments");
		return 0;
	}
	

	printf("\nSTARTING TESTHW WIRELESS APPLICATION...\n");
	printf("Resetting Piper...\n");
	piperReset();
		
	printf("Initializing uP and Piper...\n");
	initializeuPToKnowState();
	
	#if CONFIG_CCW9P9215
	//registersPiperToDefault();
	#endif
	
	waitUS(10);
	
	printf("Loading Firmware to Piper...\n");
	InitHW();
	
	printf("Setting Channel 1...\n");
	SetChannel(1);
	
	printf("Setting BSS...\n");
	SetBSS (1, (unsigned char *) "TESTING123", 10, 0xFFFF, 10); /* "TESTING123" */

	//SetBSS (0, "TESTING123", 10, 0xFFFF, 10);
	//MacRadioTXPeriodic (0, 0, 0, 0, 0, 0);

	//waitUS(1000);

	printf("Activating DSP and MAC processor...\n");
	__write32 (HW_GEN_CONTROL, DSP_MACASSIST_ENABLE, OR);
	
	
	printf("Setting output power level to min...\n");
	MacSetDirectTxPower(0);
	

	/* Golden delay. Never delete. */
	printf("Waiting for start-up:");
	for (i = 1; i >= 0; i--) // From 2
	{
		printf (" %ds. ",i);
		waitUS(1000000);	
	}
	printf("\n");

		
	//MacSetDirectTxPower(20);
	
	auxMenuStatus.statusInformation = RADIO_NOSELECTION;
	auxMenuStatus.power_control = FALSE;
	auxMenuStatus.calibration_status = FALSE;
	auxMenuStatus.channel = 1;
	auxMenuStatus.transmit_mode = 0;
	auxMenuStatus.txpower = 0;
	auxMenuStatus.transmit_rate = 0;
	auxMenuStatus.frame_period = 0;
	auxMenuStatus.frame_length = 0;
	auxMenuStatus.lastNumRxFrames = 0;
	//writeDummyPowerLimitValues(&(auxMenuStatus.calData));

	printf("Wireless hardware running.\nStarting up test enviroment ");
	
	if (argc == 1)
	{
		cancel_prompt_echo = 1;
		printf ("without human interface...\n");
		mainRoutineTestHWWirelessNoMenu(&auxMenuStatus);
		cancel_prompt_echo = 0;
	}
	else
	{
		printf ("with human interface...\n");
		tst_wifi(&auxMenuStatus);
	}

	return 1;
}

/* ********** Test command implemented ********** */

TESTHW_CMD(wireless, USAGE);

#endif /* CFG_HAS_WIRELESS */

