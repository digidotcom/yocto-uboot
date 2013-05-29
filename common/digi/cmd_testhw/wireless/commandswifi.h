#include <common.h>

#if (CONFIG_COMMANDS & CFG_CMD_BSP && \
     defined(CONFIG_UBOOT_CMD_BSP_TESTHW) && \
     defined(CFG_HAS_WIRELESS))

#ifndef _COMMANDS_WIFI_H
#define _COMMANDS_WIFI_H

#include "gui_tst_wifi.h"
#include "calibration.h"
#include "vBPiper.h"
#include "adc081C021.h"
#include "AIROHA_7230.h"

/* Indicated that is an option to be performed only if there is a mode selected previously*/
#define REQUIRESMODESELEC(x)	((x >> 30) & 0x01)

/* Answers defintion */
#define COMMANDEXECUTED		"OK\0"


/* Error definition */
#define NOMODESELECTED		"E:0x01\0"
#define UNKNOWNERROR		"E:0xFF\0"

/* PROMPTS */
#define STANDBYNOMODE		"?:\0"

/*****************************************************************************/
#define CMDSCTXGETCHANNEL(x)	((x >> 21) & 0x0000003F)
#define CMDSCTXGETPC(x)		((x >> 13) & 0x00000001)
#define CMDSCTXGETTYPE(x)	((x >> 11) & 0x00000003)
#define CMDSCTXGETRATE(x)	((x >> 7)  & 0x0000000F)
#define CMDSCTXGETPOWER(x)	( x        & 0x0000007F)

#define PRMPTTXCONTPC0		"TXC0:\0"
#define PRMPTTXCONTPC1		"TXC1:\0"

int cmdSetContinousTx (long command, struct menuStatus *auxMenuStatus);

/*****************************************************************************/
#define CMDSCTXGETCHANNEL(x)	((x >> 21) & 0x0000003F)
#define CMDSCTXGETPERIOD(x)	((x >> 14) & 0x0000007F)
#define CMDSCTXGETPC(x)		((x >> 13) & 0x00000001)
#define CMDSCTXGETLENGTH(x)	((x >> 11) & 0x00000003)
#define CMDSCTXGETRATE(x)	((x >> 7)  & 0x0000000F)
#define CMDSCTXGETPOWER(x)	( x        & 0x0000007F)

#define PRMPTTXPERIODPC0		"TXP0:\0"
#define PRMPTTXPERIODPC1		"TXP1:\0"

int cmdSetPeriodicTx (long command, struct menuStatus *auxMenuStatus);
/*****************************************************************************/
#define PRMPTRX			"RX:\0"
#define CMDSRXGETCHANNEL(x)	((x >> 21) & 0x0000003F)

int cmdSetRx (long command, struct menuStatus *auxMenuStatus);
/*****************************************************************************/
#define CMDCPGETPOWER(x)	( x       & 0x0000003F)
#define CMDCPGETINCRTYPE(x)	((x >> 7) & 0x00000001)
#define CMDCPGETINCRDIR(x)	((x >> 8) & 0x00000001)

int cmdChangePower (long command, struct menuStatus *auxMenuStatus);
/*****************************************************************************/
#define CMDCCCHANNEL(x)		((x >> 21) & 0x0000003F)

int cmdChangeChannel (long command, struct menuStatus *auxMenuStatus);

/*****************************************************************************/
#define CMDCRRATE(x)		((x >> 7) & 0x0000000B)

int cmdChangeRate (long command, struct menuStatus *auxMenuStatus);
/*****************************************************************************/
#define CMDCFLLENGTH(x)		((x >> 11) & 0x00000003)

int cmdChangeFrameLength (long command, struct menuStatus *auxMenuStatus);
/*****************************************************************************/
#define CMDCFPPERIOD(x)		((x >> 14) & 0x0000007F)

int cmdChangeFramePeriod (long command, struct menuStatus *auxMenuStatus);
/*****************************************************************************/
#define CMDRAVGETMEAS(x)	((x >> 7) & 0x03)
#define CMDRAVGETFORMAT(x)	((x >> 2 & 0x1))
#define ADCOUTPUT(string)	"ADC: string\r\n"

#define MAXADC		0x00
#define MINADC		0x01
#define AVGADC		0x02

int cmdReadADCValues (long command, struct menuStatus *auxMenuStatus);
/*****************************************************************************/
#define CMDRRFGETFORMAT(x)	((x >> 2 & 0x1))
#define RESETCOUNTER(x)		(x & 0x01)

#define RXOUTPUT(string)	"RXf: string\r\n"

int cmdReadResetRxFrames (long command, struct menuStatus *auxMenuStatus);
/*****************************************************************************/
#define PRMPTEXITING	"CLOSING...\r\n\0"
int cmdExitTestHWWireless (long command, struct menuStatus *auxMenuStatus);
/*****************************************************************************/
int cmdExitMode (long command, struct menuStatus *auxMenuStatus);
/*****************************************************************************/
int cmdReadCalibration (long command, struct menuStatus *auxMenuStatus);
/*****************************************************************************/
int cmdSetCalibration (long command, struct menuStatus *auxMenuStatus);

/*****************************************************************************/
int cmdGetStatus (long command, struct menuStatus *auxMenuStatus);
/*****************************************************************************/
#define GETCOMMAND(x)		((x >> 27) & 0x0000001F)

int mainRoutineTestHWWirelessNoMenu(struct menuStatus *auxMenuStatus);

/*****************************************************************************/
int applyWirelessSettingsNoMenu (struct menuStatus *auxMenuStatus);

/*****************************************************************************/
int changePrompt(char *newPrompt, struct menuStatus *auxMenuStatus);
/*****************************************************************************/
void refreshPrompt(struct menuStatus *auxMenuStatus);
/*****************************************************************************/
void printMessage (char *message);
/*****************************************************************************/
void printError(char *error);

//#define DEBUGGING 1
#ifdef DEBUGGING
	#define PRINTDEBUG(x)	printf(x);
#else
	#define PRINTDEBUG(x)
#endif

#endif /*COMMAND WIFI*/
#endif
