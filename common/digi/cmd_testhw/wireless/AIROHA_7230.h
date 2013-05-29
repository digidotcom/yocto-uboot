#include <common.h>

#if (CONFIG_COMMANDS & CFG_CMD_BSP && \
     defined(CONFIG_UBOOT_CMD_BSP_TESTHW) && \
     defined(CFG_HAS_WIRELESS))

/*********************/
/* OTHER DEFINITIONS */
/*********************/
//#define AIROHA_PWR_CALIBRATION

#ifndef _AIROHA_H
#define _AIROHA_H

// AIROHA specific RF transceiver frequency divider for each channel
static const struct 
{
	uint32 integer;
	uint32 fraction;
	uint32 addres4Airoha;
	uint32 channelNumber;
	uint32 channelFrequency;
	
} freqTableAiroha_7230[] = {
	{ 0, 0, 0, 0, 0},
	// 2.4 GHz band (802.11b/g)
	{ 0x00379, 0x13333, 0x7FD78,   1, 2412},
	{ 0x00379, 0x1B333, 0x7FD78,   2, 2417},
	{ 0x00379, 0x03333, 0x7FD78,   3, 2422},
	{ 0x00379, 0x0B333, 0x7FD78,   4, 2427},
	{ 0x0037A, 0x13333, 0x7FD78,   5, 2432},
	{ 0x0037A, 0x1B333, 0x7FD78,   6, 2437},
	{ 0x0037A, 0x03333, 0x7FD78,   7, 2442},
	{ 0x0037A, 0x0B333, 0x7FD78,   8, 2447},
	{ 0x0037B, 0x13333, 0x7FD78,   9, 2452},
	{ 0x0037B, 0x1B333, 0x7FD78,  10, 2457},
	{ 0x0037B, 0x03333, 0x7FD78,  11, 2462},
	{ 0x0037B, 0x0B333, 0x7FD78,  12, 2467},
	{ 0x0037C, 0x13333, 0x7FD78,  13, 2472},
	{ 0x0037C, 0x06666, 0x7FD78,  14, 2484},
	// 5 GHz band (802.11a)
	{ 0x0FF52, 0x00000, 0x67F78, 184, 4920}, //15
	{ 0x0FF52, 0x0AAAA, 0x77F78, 188, 4940}, //16
	{ 0x0FF53, 0x15555, 0x77F78, 192, 4960}, //17
	{ 0x0FF53, 0x00000, 0x67F78, 196, 4980}, //18
	{ 0x0FF54, 0x00000, 0x67F78,   8, 5040}, //19
	{ 0x0FF54, 0x0AAAA, 0x77F78,  12, 5060}, //20
	{ 0x0FF55, 0x15555, 0x77F78,  16, 5080}, //21
	{ 0x0FF56, 0x05555, 0x77F78,  34, 5170}, //22
	{ 0x0FF56, 0x0AAAA, 0x77F78,  36, 5180}, //23
	{ 0x0FF57, 0x10000, 0x77F78,  38, 5190}, //24
	{ 0x0FF57, 0x15555, 0x77F78,  40, 5200}, //25
	{ 0x0FF57, 0x1AAAA, 0x77F78,  42, 5210}, //26
	{ 0x0FF57, 0x00000, 0x67F78,  44, 5220}, //27
	{ 0x0FF57, 0x05555, 0x77F78,  46, 5230}, //28
	{ 0x0FF57, 0x0AAAA, 0x77F78,  48, 5240}, //29
	{ 0x0FF58, 0x15555, 0x77F78,  52, 5260}, //30
	{ 0x0FF58, 0x00000, 0x67F78,  56, 5280}, //31
	{ 0x0FF58, 0x0AAAA, 0x77F78,  60, 5300}, //32
	{ 0x0FF59, 0x15555, 0x77F78,  64, 5320}, //33
	{ 0x0FF5C, 0x15555, 0x77F78, 100, 5500}, //34
	{ 0x0FF5C, 0x00001, 0x67F78, 104, 5520}, //35
	{ 0x0FF5C, 0x0AAAA, 0x77F78, 108, 5540}, //36
	{ 0x0FF5D, 0x15555, 0x77F78, 112, 5560}, //37
	{ 0x0FF5D, 0x00000, 0x67F78, 116, 5580}, //38
	{ 0x0FF5D, 0x0AAAA, 0x77F78, 120, 5600}, //39
	{ 0x0FF5E, 0x15555, 0x77F78, 124, 5620}, //40
	{ 0x0FF5E, 0x00000, 0x67F78, 128, 5640}, //41
	{ 0x0FF5E, 0x0AAAA, 0x77F78, 132, 5660}, //42
	{ 0x0FF5F, 0x15555, 0x77F78, 136, 5680}, //43
	{ 0x0FF5F, 0x00000, 0x67F78, 140, 5700}, //44
	{ 0x0FF60, 0x18000, 0x77F78, 149, 5745}, //45
	{ 0x0FF60, 0x02AAA, 0x77F78, 153, 5765}, //46
	{ 0x0FF60, 0x0D555, 0x77F78, 157, 5785}, //47
	{ 0x0FF61, 0x18000, 0x77F78, 161, 5805}, //48
	{ 0x0FF61, 0x02AAA, 0x77F78, 165, 5825}  //49
};

/*********************/
/* OTHER DEFINITIONS */
/*********************/
#define RF_AIROHA_7230 	0

#define RF_24GHZ 	0
#define RF_50GHZ 	1

// Useful RSSI range in dbm
#define RSSI_LOW    (-90)
#define RSSI_HIGH   (-30)

#define	GEN_RESET		0x527f0000	// Reset state
#define	GEN_INIT		0x37780005	// Initial state
#define	GEN_INIT_MAXIM		0x37780005	// Initial state; PA_ON= active low; bit 25
#define	GEN_INIT_AIROHA_24GHZ	0x31780005	// Initial state; 2.4GHZ_PA_ON= active low; bit 25
#define	GEN_INIT_AIROHA_50GHZ	0x33780008	// Initial state; 5.0GHZ_PA_ON= active high; bit 25
#define	SPI_INIT_AIROHA		0x00000018      // AIROHA-specific SPI length

// TRUE if channel number in 5 GHz band
#define	CHAN_5G(chan)		( (chan) > 14)
#define CHAN_4920_4980(chan)	(((chan) > 14) && ((chan) < 19))
#define CHAN_5150_5350(chan)	(((chan) > 21) && ((chan) < 34))
#define CHAN_5470_5725(chan)	(((chan) > 33) && ((chan) < 44))
#define CHAN_5725_5825(chan)	(((chan) > 44) && ((chan) < 50))


#endif

#endif /* CFG_HAS_WIRELESS */

