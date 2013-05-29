#define CALIBRATIONTEST 1

#include <common.h>
#include <nvram.h>

#if (CONFIG_COMMANDS & CFG_CMD_BSP && \
     defined(CONFIG_UBOOT_CMD_BSP_TESTHW) && \
     defined(CFG_HAS_WIRELESS))

#ifndef __CALIBRATION_H__
#define __CALIBRATION_H__

#define WCD_NUM_MOD_BG		2

/*#define char_t		char
#define uint8_t		unsigned char
#define uint16_t	unsigned int
#define uint32_t	unsigned long*/


/* imports from common/main.c */
extern char console_buffer[CFG_CBSIZE];

extern int getSLongConsoleNOREAD(char *auxConBuff, long *returned, char **p);

int getConsoleNOREAD(char *auxConBuff, void *toReturn, char **p, unsigned char size, unsigned char isSigned);

int retrieveCalHeaderConsole (nv_wcd_header_t *auxHeader, unsigned char decimalSeparator, \
			      unsigned char valuesSeparator, unsigned char endProcessSeparator);

int retrieveCalPointConsole (wcd_point_t *auxPoint, unsigned char decimalSeparator, \
			     unsigned char valuesSeparator, unsigned char endProcessSeparator, \
			     unsigned char channelNumber, unsigned char modulationType, unsigned char point);

int retrieveCalCurveConsole (wcd_curve_t *auxCurve, unsigned char decimalSeparator, \
			     unsigned char valuesSeparator, unsigned char endProcessSeparator, \
			     unsigned char channelNumber, unsigned char modulationType);

int retrieveCalData (wcd_data_t *auxData, unsigned char decimalSeparator, \
		     unsigned char valuesSeparator, unsigned char endProcessSeparator);

int printCalHeaderConsole (nv_wcd_header_t *auxHeader);

int printCalPointConsole (wcd_point_t *auxPoint, \
			     unsigned char channelNumber, unsigned char modulationType, unsigned char point);

int printCalCurveConsole (wcd_curve_t *auxCurve, \
			     unsigned char channelNumber, unsigned char modulationType);

int printCalData (wcd_data_t *auxData);

unsigned char getAIfromPW(wcd_data_t *auxCalData, int16_t powerOutputRF, unsigned int channel, unsigned int rate);
unsigned char getADCfromPW(wcd_data_t *auxCalData, int16_t powerOutputRF, unsigned int channel, unsigned int rate);
unsigned char getAIfromADC(wcd_data_t *auxCalData, int16_t ADC, unsigned int channel, unsigned int rate);
unsigned char PIDController (wcd_data_t *auxCalData, unsigned int desiredPower, unsigned int channel, unsigned int rate);


#define CCK	0
#define OFDM	1
static const unsigned char nTs_modulation_type[][6] =
{
	"CCK\0",
	"OFDM\0"
};

// TRUE if channel number in 5 GHz band
#define	CHAN_5G(chan)		( (chan) > 14)
#define CHAN_4920_4980(chan)	(((chan) > 14) && ((chan) < 19))
#define CHAN_5150_5350(chan)	(((chan) > 21) && ((chan) < 34))
#define CHAN_5470_5725(chan)	(((chan) > 33) && ((chan) < 44))
#define CHAN_5725_5825(chan)	(((chan) > 44) && ((chan) < 50))


#endif /* CFG_HAS_WIRELESS */
#endif /* __CALIBRATION_H__ */
