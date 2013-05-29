#include <common.h>

#if (CONFIG_COMMANDS & CFG_CMD_BSP && \
     defined(CONFIG_UBOOT_CMD_BSP_TESTHW) && \
     defined(CFG_HAS_WIRELESS))

#ifndef _ADC081C021_H_
#define _ADC081C021_H_

// ADC121C021/ADC121C027 registers
#define ADC_CONV_RESULT		0x0
#define ADC_ALERT_STATUS	0x1
#define ADC_CONF		0x2
#define ADC_LOW_LIMIT		0x3
#define ADC_HIGH_LIMIT		0x4
#define ADC_HYST		0x5
#define ADC_CONV_LOWEST		0x6
#define ADC_CONV_HIGHEST	0x7

/* ADC081C027 && ADDR==GND */
#define ADC_SLAVE_ADDR          0x51

#define CLEARLOWESTVAL		0x0FF0
#define CLEARHIGHESTVAL		0x0000


/* initialize I2C interface for ADC */
int init_adc_pc(void);

/* Reading */
int read_conversion (unsigned char *buffer);
int read_max_conversion (unsigned char *buffer);
int read_max_min_avg_conversion (unsigned char numOfAverages, unsigned char *buffer);

/* Resseting */
int reset_max_min_values (void);

/* Writing */
int write_adc_pc_char (unsigned int address, unsigned char buffer);

#endif

#endif /* CFG_HAS_WIRELESS */
