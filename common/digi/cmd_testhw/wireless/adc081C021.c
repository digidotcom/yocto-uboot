#include <common.h>

#if (CONFIG_COMMANDS & CFG_CMD_BSP && \
     defined(CONFIG_UBOOT_CMD_BSP_TESTHW) && \
     defined(CFG_HAS_WIRELESS))

#include <common.h>
#include <i2c.h>
#include "adc081C021.h"

/* initialize I2C interface for ADC */
int init_adc_pc(void)
{
	
	//i2c_init(400, ADC_SLAVE_ADDR << 1);
	write_adc_pc_char (ADC_CONF, 0x20);
	//write_adc_pc_char (ADC_CONV_RESULT, 0x00);
	return 0;
}


int read_conversion (unsigned char *buffer)
{	
	unsigned char aux[2];
	unsigned char error;
	
	error = i2c_read (ADC_SLAVE_ADDR, ADC_CONV_RESULT, 0x1, aux, sizeof(aux));
	*buffer = 0;
	*buffer = (aux[0] << 4) | (aux[1] >> 4);  
	
	return error;
}

int read_max_conversion (unsigned char *buffer)
{	
	unsigned char aux[2];
	unsigned char error;
	
	error = i2c_read (ADC_SLAVE_ADDR, ADC_CONV_HIGHEST, 0x1, aux, sizeof(aux));
	*buffer = 0;
	*buffer = (aux[0] << 4) | (aux[1] >> 4);  
	
	return error;
}

int read_max_min_avg_conversion (unsigned char numOfAverages, unsigned char *buffer)
{
	unsigned char value, instantadc, minadc, maxadc;
	unsigned long avgadc;
	unsigned char loopadc;

	avgadc = 0;
	minadc = 255;
	maxadc = 0;
	
	for(loopadc=0; loopadc<numOfAverages; loopadc++) 
	{	
		udelay(1000);
		value = read_conversion (&instantadc);
		if (value)
			return value;
	   	avgadc += instantadc;
	   	if (instantadc > maxadc) maxadc = instantadc;
	   	if (instantadc < minadc) minadc = instantadc;
	}
	avgadc /= numOfAverages;

	buffer[0] = avgadc;
	buffer[1] = maxadc;
	buffer[2] = minadc;

	return 0;
}

int reset_max_min_values (void)
{
	unsigned int aux;

	aux = CLEARLOWESTVAL;
	if(i2c_write (ADC_SLAVE_ADDR, ADC_CONV_LOWEST, 0x1, (unsigned char*) &aux, 2 * sizeof(unsigned char)) != 0)
		return 1;
	aux = CLEARHIGHESTVAL;
	if (i2c_write (ADC_SLAVE_ADDR, ADC_CONV_HIGHEST, 0x1, (unsigned char*) &aux, 2 * sizeof(unsigned char)) != 0)
		return 1;

	return 0;
}


int write_adc_pc_char (unsigned int address, unsigned char buffer)
{
	return i2c_write (ADC_SLAVE_ADDR, address, 0x1, &buffer, sizeof(unsigned char));
}

#endif /* CFG_HAS_WIRELESS */
