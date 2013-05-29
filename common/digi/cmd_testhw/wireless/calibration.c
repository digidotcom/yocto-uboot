#include <common.h>
#include "calibration.h"
#include "adc081C021.h"
#include "vBPiper.h"
#include <nvram.h>

#include "gui_tst_wifi.h" /* MACROS FOR RATES */

#if (CONFIG_COMMANDS & CFG_CMD_BSP && \
     defined(CONFIG_UBOOT_CMD_BSP_TESTHW) && \
     defined(CFG_HAS_WIRELESS))

//#include <stdarg.h>
//#include <linux/types.h>
//#include <linux/string.h>
//#include <linux/ctype.h>
//#include <nvram_types.h>
#include <mtd.h>

static unsigned char numPoints;

/*static int getSLongConsoleNOREAD(char *auxConBuff, long *returned, char **p)
{
	*returned = simple_strtol (auxConBuff, p, 10);

	return 0;
}*/

int getConsoleNOREAD(char *auxConBuff, void *toReturn, char **p, unsigned char size, unsigned char isSigned)
{
	if (size == sizeof(char_t))
	{
		if (isSigned)
		{
			char_t *returned;
			returned = (char_t *) toReturn;
			*returned = (char_t) simple_strtol (auxConBuff, p, 10);
			return 0;
		}
		else
		{
			uchar_t *returned;
			returned = (uchar_t *) toReturn;
			*returned = (uchar_t) simple_strtoul (auxConBuff, p, 10);
			return 0;
		}
	}
	else if (size == sizeof(int16_t))
	{
		if (isSigned)
		{
			int16_t *returned;
			returned = (int16_t *) toReturn;
			*returned = (int16_t) simple_strtol (auxConBuff, p, 10);
			return 0;
		}
		else
		{
			uint16_t *returned;
			returned = (uint16_t *) toReturn;
			*returned = (uint16_t) simple_strtoul (auxConBuff, p, 10);
			return 0;
		}
	}
	else if (size == sizeof(int32_t))
	{
		if (isSigned)
		{
			int32_t *returned;
			returned  = (int32_t *) toReturn;
			*returned = (int32_t) simple_strtol (auxConBuff, p, 10);
			return 0;
		}
		else
		{
			uint32_t *returned;
			returned = (uint32_t *) toReturn;
			*returned = (uint32_t) simple_strtoul (auxConBuff, p, 10);
			return 0;
		}
	}
	return 1;
}


int retrieveCalHeaderConsole (nv_wcd_header_t *auxHeader, unsigned char decimalSeparator, \
			      unsigned char valuesSeparator, unsigned char endProcessSeparator)
{
	int aux;
	char *p = NULL;
		
	printf ("Input_Header_Line >>");	
	aux = readline("");
	if (aux > 0)
       	{
		strcpy(auxHeader->magic_string, WCD_MAGIC);
		
		/* ver_major */
		getConsoleNOREAD(console_buffer, &(auxHeader->ver_major), &p, sizeof(auxHeader->ver_major), 0);
		p++;
		
		/* ver_minor */	
		getConsoleNOREAD(p, &(auxHeader->ver_minor), &p, sizeof(auxHeader->ver_minor), 0);
		p++;
				
		/* hw_platform */	
		getConsoleNOREAD(p, &(auxHeader->hw_platform), &p, sizeof(auxHeader->hw_platform), 0);
		p++;
		
		/* bNumCalPoint */	
		getConsoleNOREAD(p, &(auxHeader->numcalpoints), &p, sizeof(auxHeader->numcalpoints), 0);
		p++;
		
		numPoints = auxHeader->numcalpoints;

		/* uCalDataLen */	
		getConsoleNOREAD(p, &(auxHeader->wcd_len), &p, sizeof(auxHeader->wcd_len), 0);
		p++;
		
		/* uCalDataCrc */	
		getConsoleNOREAD(p, &(auxHeader->wcd_crc), &p, sizeof(auxHeader->wcd_crc), 0);

		/*auxCrc32 = crc32 (0, (unsigned char*) auxHeader, sizeof(nv_wcd_header_t));*/

		if (1)/*auxCrc32 == auxHeader->uCalDataCrc)*/
			return 0; /* Header Correct */
		else
			return -1; /* Error in header */

	}
	return aux;
}

int retrieveCalPointConsole (wcd_point_t *auxPoint, unsigned char decimalSeparator, \
			     unsigned char valuesSeparator, unsigned char endProcessSeparator, \
			     unsigned char channelNumber, unsigned char modulationType, unsigned char point)
{
	int aux;
	char *p = NULL;
		
	printf ("Point_%i_CH_%i_MOD_%s >>", (int) point, (int) channelNumber, nTs_modulation_type[modulationType]);
	aux = readline("");
	if (aux > 0)
       	{
		/* out_power */	
		getConsoleNOREAD(console_buffer, &(auxPoint->out_power), &p, sizeof(auxPoint->out_power), 1);
		p++;

		/* adc_val */	
		getConsoleNOREAD(p, &(auxPoint->adc_val), &p, sizeof(auxPoint->adc_val), 0);
		p++;

		/* power_index */	
		getConsoleNOREAD(p, &(auxPoint->power_index), &p, sizeof(auxPoint->power_index), 0);

		auxPoint->reserved[0] = 0;
		auxPoint->reserved[1] = 0;
		auxPoint->reserved[2] = 0;

		return 0;
	}
	return aux;
}

int retrieveCalCurveConsole (wcd_curve_t *auxCurve, unsigned char decimalSeparator, \
			     unsigned char valuesSeparator, unsigned char endProcessSeparator, \
			     unsigned char channelNumber, unsigned char modulationType)
{
	int aux;
	char *p = NULL;
	
	printf ("Curve_CH_%i_MOD_%s >>", (int) channelNumber, nTs_modulation_type[modulationType]);
	aux = readline("");
	if (aux > 0)
       	{
		auxCurve->reserved[0] = 0;
		auxCurve->reserved[1] = 0;
		auxCurve->reserved[2] = 0;

		/* max_power_index */	
		getConsoleNOREAD(console_buffer, &(auxCurve->max_power_index), &p, sizeof(auxCurve->max_power_index), 0);

		/* points */
		for(aux = 0; aux < numPoints; aux++)
		{
			retrieveCalPointConsole(&(auxCurve->points[aux]), decimalSeparator, \
				valuesSeparator, endProcessSeparator, channelNumber, modulationType, aux + 1);
		}
		return 0;
	}
	return aux;
}

int retrieveCalData (wcd_data_t *auxData, unsigned char decimalSeparator, \
		     unsigned char valuesSeparator, unsigned char endProcessSeparator)
{
	int i,j;
	//char *p = NULL;

	printf("1");
	retrieveCalHeaderConsole(&(auxData->header), '.',',',';');

	printf ("\n-------------------------------------\n");
	printf ("--- CALIBRATION DATA FOR BAND B/G ---\n");
	printf ("-------------------------------------\n\n");

	/* Retrieve curves B/G */
	for(j=0; j<WCD_NUM_MOD_BG; j++)
	{
		for(i=0; i<WCD_CHANNELS_BG; i++)
		{
		
			retrieveCalCurveConsole(&(auxData->cal_curves_bg[i][j]), decimalSeparator, \
				valuesSeparator, endProcessSeparator, i+1, j);
		}
	}

	printf ("\n-----------------------------------\n");
	printf ("--- CALIBRATION DATA FOR BAND A ---\n");
	printf ("-----------------------------------\n\n");

	/* Retrieve curves A */
	for(i=0; i<WCD_CHANNELS_A; i++)
	{
		retrieveCalCurveConsole(&(auxData->cal_curves_a[i]), decimalSeparator, \
			valuesSeparator, endProcessSeparator, i+15, 1);
	}

	//auxData->header.wcd_len = sizeof(wcd_curve_t)*WCD_CHANNELS_BG*2 + sizeof(wcd_curve_t)*WCD_CHANNELS_A;
	//auxData->header.wcd_crc = crc32( 0, (const unsigned char*) auxData->cal_curves_bg, auxData->header.wcd_len);
	
	return 0;
}

int printCalHeaderConsole (nv_wcd_header_t *auxHeader)
{
	printf("%s", auxHeader->magic_string);
		
	/* ver_major */	
	printf("%i, ", auxHeader->ver_major);
	
	/* ver_minor */	
	printf("%i, ", auxHeader->ver_minor);
	
	/* hw_platform */	
	printf("%i, ", auxHeader->hw_platform);
	
	/* bNumCalPoint */	
	printf("%i, ", auxHeader->numcalpoints);
	
	/* uCalDataLen */	
	printf("%x, ", auxHeader->wcd_len);
	
	/* uCalDataCrc */	
	printf("%x\n", auxHeader->wcd_crc);

	return 0;
}

int printCalPointConsole (wcd_point_t *auxPoint, \
			     unsigned char channelNumber, unsigned char modulationType, unsigned char point)
{
	printf ("Point_%i_CH_%i_MOD_%s -- ", point, channelNumber, nTs_modulation_type[modulationType]);
	
	/* out_power */	
	printf("%i, ", auxPoint->out_power);
	
	/* adc_val */	
	printf("%i, ", auxPoint->adc_val);

	/* power_index */	
	printf("%i\n", auxPoint->power_index);

	return 0;
}

int printCalCurveConsole (wcd_curve_t *auxCurve, \
			     unsigned char channelNumber, unsigned char modulationType)
{
	unsigned char aux;

	printf ("Curve_CH_%i_MOD_%s -- ", channelNumber, nTs_modulation_type[modulationType]);
	
	/* max_power_index */	
	printf("%i --> \n", auxCurve->max_power_index);

	/* points */
	for(aux = 0; aux < numPoints; aux++)
	{
		printCalPointConsole(&(auxCurve->points[aux]),channelNumber, modulationType, aux + 1);
	}

	return 0;
}

int printCalData (wcd_data_t *auxData)
{
	int i,j;

	//char *p = NULL;

	printCalHeaderConsole(&(auxData->header));

	printf ("\n-------------------------------------\n");
	printf ("--- CALIBRATION DATA FOR BAND B/G ---\n");
	printf ("-------------------------------------\n\n");

	/* Retrieve curves B/G */
	for(j=0; j<WCD_NUM_MOD_BG; j++)
	{
		for(i=0; i<WCD_CHANNELS_BG; i++)
		{
			printCalCurveConsole(&(auxData->cal_curves_bg[i][j]), i+1, j);
		}
	}

	printf ("\n-----------------------------------\n");
	printf ("--- CALIBRATION DATA FOR BAND A ---\n");
	printf ("-----------------------------------\n\n");

	/* Retrieve curves A */
	for(i=0; i<WCD_CHANNELS_A; i++)
	{
		printCalCurveConsole(&(auxData->cal_curves_a[i]), i+15, 0);
	}


	printf("CRC's %X %x\n", crc32( 0, (const unsigned char*) auxData->cal_curves_bg, sizeof(wcd_curve_t)*WCD_CHANNELS_BG*2 + sizeof(wcd_curve_t)*WCD_CHANNELS_A), auxData->header.wcd_crc);

	return 0;
}

/* returns Airoha Index based in RF output Power and calibration data */
/* TO BE MODIFIED */
#if 0
unsigned char getAIfromPW(wcd_data_t *auxCalData, int16_t powerOutputRF, unsigned int channel, unsigned int rate)
{
	unsigned char lowCalAI, highCalAI, desiredAI;
	unsigned char i, auxModulation;

	
	if (!CHAN_5G(channel))
	{
		/* Band B/G */

		if (IS_CCK_DSSS(rate))
			auxModulation = 0;
		else
			auxModulation = 1;
		
		if (auxCalData->cal_curves_bg[channel][auxModulation].points[0].out_power >= powerOutputRF)
			desiredAI = 0;
		else if (auxCalData->cal_curves_bg[channel][auxModulation].points[WCD_MAX_CAL_POINTS - 1].out_power < powerOutputRF)
			desiredAI = auxCalData->cal_curves_bg[channel][auxModulation].max_power_index;
		else
		{
			/* Search bottom index */
			i = 0;
			while (auxCalData->cal_curves_bg[channel][auxModulation].points[i].out_power <= powerOutputRF)
			{
				i++;
				if (i > WCD_MAX_CAL_POINTS - 1)
					break;
			}
			
			desiredAI = i-1;

			/* Calculate ADC for desired power */
			desiredAI = (unsigned char) \
			((powerOutputRF * auxCalData->cal_curves_bg[channel][auxModulation].points[lowCalAI].power_index) / \
			(auxCalData->cal_curves_bg[channel][auxModulation].points[lowCalAI].out_power));
		}
	}
	else
	{
		/* Band A */
		/* Search bottom index */
		i = 0;
		while (auxCalData->cal_curves_a[channel].points[i].out_power < powerOutputRF)
		{
			i++;
		} 
			
		lowCalAI = i;
		highCalAI = i+1;

		/* Calculate Airoha Index for desired power */
		desiredAI = (unsigned char) \
		((powerOutputRF * auxCalData->cal_curves_a[channel].points[lowCalAI].power_index) / \
		(auxCalData->cal_curves_a[channel].points[lowCalAI].out_power));
	}
	
	return desiredAI;
}
#endif

/* returns Airoha Index based in RF output Power and calibration data */
unsigned char getADCfromPW(wcd_data_t *auxCalData, int16_t powerOutputRF, unsigned int channel, unsigned int rate)
{
	unsigned char lowCalADC, highCalADC, desiredADC;
	unsigned char i, auxModulation;

	signed int deltaRF = 0;
	signed int deltaADC = 0;//signed int
	float minRF = 0;//int
	
	if (!CHAN_5G(channel))
	{
		/* Band B/G */

		if (IS_CCK_DSSS(rate))
			auxModulation = 0; /* CCK/DSSS */
		else
			auxModulation = 1; /* OFDM */

		/* Calculate ADC for desired power */
		if (auxCalData->cal_curves_bg[channel][auxModulation].points[0].out_power >= powerOutputRF)
		{
			desiredADC = auxCalData->cal_curves_bg[channel][auxModulation].points[0].adc_val;
		}
		else if (auxCalData->cal_curves_bg[channel][auxModulation].points[auxCalData->header.numcalpoints - 1].out_power < powerOutputRF)
		{
			desiredADC = auxCalData->cal_curves_bg[channel][auxModulation].points[auxCalData->header.numcalpoints - 1].adc_val;
		}
		else
		{
			/* Search bottom index */
			i = 0;
			while (auxCalData->cal_curves_bg[channel][auxModulation].points[i].out_power <= powerOutputRF)
			{
				i++;
				if (i > auxCalData->header.numcalpoints - 1)
					break;
			}
			
			lowCalADC = i-1;
			highCalADC = i;
			
			/* Calculate ADC for desired power */
			/*desiredADC = (unsigned char) \
			((powerOutputRF * auxCalData->cal_curves_bg[channel][auxModulation].points[lowCalADC].adc_val) / \
			(auxCalData->cal_curves_bg[channel][auxModulation].points[lowCalADC].out_power));*/

			minRF = auxCalData->cal_curves_bg[channel][auxModulation].points[lowCalADC].out_power;
			deltaRF = auxCalData->cal_curves_bg[channel][auxModulation].points[highCalADC].out_power - minRF;
			deltaADC = auxCalData->cal_curves_bg[channel][auxModulation].points[highCalADC].adc_val \
				- auxCalData->cal_curves_bg[channel][auxModulation].points[lowCalADC].adc_val; 

			desiredADC = (unsigned char) ( (float) (powerOutputRF - minRF) * ( (float) deltaADC / (float) deltaRF) \
				+ auxCalData->cal_curves_bg[channel][auxModulation].points[lowCalADC].adc_val );
			
			/*desiredADC = (unsigned char) ( ((powerOutputRF - minRF) * ( deltaADC*1000000 / deltaRF*1000000))/1000000 \
				+ auxCalData->cal_curves_bg[channel][auxModulation].points[lowCalADC].adc_val );*/

		}
	}
	else
	{
		/* Band A */
		/* Calculate ADC for desired power */
		
		/* Calculate ADC for desired power */
		channel = channel - 15;

		if (auxCalData->cal_curves_a[channel].points[0].out_power >= powerOutputRF)
		{
			desiredADC = auxCalData->cal_curves_a[channel].points[0].adc_val;
		}
		else if (auxCalData->cal_curves_a[channel].points[auxCalData->header.numcalpoints - 1].out_power < powerOutputRF)
		{
			desiredADC = auxCalData->cal_curves_a[channel].points[auxCalData->header.numcalpoints - 1].adc_val;
		}
		else
		{
			/* Search bottom index */
			i = 0;
			while (auxCalData->cal_curves_a[channel].points[i].out_power <= powerOutputRF)
			{
				i++;
				if (i > auxCalData->header.numcalpoints - 1)
					break;
			}
			
			lowCalADC = i-1;
			highCalADC = i;
			
			/* Calculate ADC for desired power */
			/*desiredADC = (unsigned char) \
			((powerOutputRF * auxCalData->cal_curves_bg[channel][auxModulation].points[lowCalADC].adc_val) / \
			(auxCalData->cal_curves_bg[channel][auxModulation].points[lowCalADC].out_power));*/

			minRF = auxCalData->cal_curves_a[channel].points[lowCalADC].out_power;
			deltaRF = auxCalData->cal_curves_a[channel].points[highCalADC].out_power - minRF;
			deltaADC = auxCalData->cal_curves_a[channel].points[highCalADC].adc_val \
				- auxCalData->cal_curves_a[channel].points[lowCalADC].adc_val; 

			desiredADC = (unsigned char) ( (float) (powerOutputRF - minRF) * ( (float) deltaADC / (float) deltaRF) \
				+ auxCalData->cal_curves_a[channel].points[lowCalADC].adc_val );
			
			/*desiredADC = (unsigned char) ( ((powerOutputRF - minRF) * ( deltaADC*1000000 / deltaRF*1000000))/1000000 \
				+ auxCalData->cal_curves_bg[channel][auxModulation].points[lowCalADC].adc_val );*/
		}
	}
	
	return desiredADC;
}

/* returns Airoha Index based in RF output Power and calibration data */
unsigned char getAIfromADC(wcd_data_t *auxCalData, int16_t ADC, unsigned int channel, unsigned int rate)
{
	unsigned char lowCalAI, highCalAI, desiredAI, minADC = 0;
	unsigned char i, auxModulation;

	signed int deltaADC = 0;
	signed int deltaAI = 0;
	
	highCalAI = 0;
	lowCalAI = 0;

	if (!CHAN_5G(channel))
	{
		/* Band B/G */

		if (IS_CCK_DSSS(rate))
			auxModulation = 0;
		else
			auxModulation = 1;
		
		
		if (auxCalData->cal_curves_bg[channel][auxModulation].points[0].adc_val >= ADC)
			desiredAI = 0;
		else if (auxCalData->cal_curves_bg[channel][auxModulation].points[auxCalData->header.numcalpoints - 1].adc_val < ADC)
			desiredAI = auxCalData->cal_curves_bg[channel][auxModulation].max_power_index;
		else
		{
			/* Search bottom index */
			i = 0;
			while (auxCalData->cal_curves_bg[channel][auxModulation].points[i].adc_val <= ADC)
			{
				i++;
				if (i > auxCalData->header.numcalpoints - 1)
					break;
			}
			
			lowCalAI = i-1;
			highCalAI = i;

			/* Calculate Airoha Index for desired power */
			/*desiredAI = (unsigned char) \
			((ADC * auxCalData->cal_curves_bg[channel][auxModulation].points[lowCalAI].power_index) / \
			(auxCalData->cal_curves_bg[channel][auxModulation].points[lowCalAI].adc_val));*/
			
			minADC = auxCalData->cal_curves_bg[channel][auxModulation].points[lowCalAI].adc_val;
			deltaADC = auxCalData->cal_curves_bg[channel][auxModulation].points[highCalAI].adc_val - minADC;
			deltaAI = auxCalData->cal_curves_bg[channel][auxModulation].points[highCalAI].power_index \
				- auxCalData->cal_curves_bg[channel][auxModulation].points[lowCalAI].power_index;

			desiredAI = (unsigned char) ( (float) (ADC - minADC) * ((float) deltaAI / (float) deltaADC) \
				+ auxCalData->cal_curves_bg[channel][auxModulation].points[lowCalAI].power_index );
		}
		#if CALIBRATIONTEST
		printf("Input to function: adc-> %i, channel-> %i\n", ADC, channel);
		printf("Points measured: lowindex-> %i, highindex-> %i, powerindex-> %i, adc-> %i\n", lowCalAI, highCalAI, auxCalData->cal_curves_bg[channel][auxModulation].points[lowCalAI].power_index, auxCalData->cal_curves_bg[channel][auxModulation].points[lowCalAI].adc_val);
		#endif
	}
	else
	{
		/*Band A*/
		channel = channel - 15;

		if (auxCalData->cal_curves_a[channel].points[0].adc_val >= ADC)
			desiredAI = 0;
		else if (auxCalData->cal_curves_a[channel].points[auxCalData->header.numcalpoints - 1].adc_val < ADC)
			desiredAI = auxCalData->cal_curves_a[channel].max_power_index;
		else
		{
			/* Search bottom index */
			i = 0;
			while (auxCalData->cal_curves_a[channel].points[i].adc_val <= ADC)
			{
				i++;
				if (i > auxCalData->header.numcalpoints - 1)
					break;
			}
			
			lowCalAI = i-1;
			highCalAI = i;

			/* Calculate Airoha Index for desired power */
			/*desiredAI = (unsigned char) \
			((ADC * auxCalData->cal_curves_bg[channel][auxModulation].points[lowCalAI].power_index) / \
			(auxCalData->cal_curves_bg[channel][auxModulation].points[lowCalAI].adc_val));*/
			
			minADC = auxCalData->cal_curves_a[channel].points[lowCalAI].adc_val;
			deltaADC = auxCalData->cal_curves_a[channel].points[highCalAI].adc_val - minADC;
			deltaAI = auxCalData->cal_curves_a[channel].points[highCalAI].power_index \
				- auxCalData->cal_curves_a[channel].points[lowCalAI].power_index;

			desiredAI = (unsigned char) ( (float) (ADC - minADC) * ((float) deltaAI / (float) deltaADC) \
				+ auxCalData->cal_curves_a[channel].points[lowCalAI].power_index );
		}
		#if CALIBRATIONTEST
		printf("Input to function: adc-> %i, channel-> %i\n", ADC, channel);
		printf("Points measured: lowindex-> %i, highindex-> %i, powerindex-> %i, adc-> %i\n", lowCalAI, highCalAI, auxCalData->cal_curves_a[channel].points[lowCalAI].power_index, auxCalData->cal_curves_a[channel].points[lowCalAI].adc_val);
		#endif
	}
	
	return desiredAI;
}

unsigned char PIDController (wcd_data_t *auxCalData, unsigned int desiredPower, unsigned int channel, unsigned int rate)
{
	float previous_error = 0;
	float error = 0, dt, output = 0, derivative = 0, integral = 0;
	float kp, ki, kd;
	
	unsigned char desiredADC;
	unsigned char measuredADC;
	signed int avgADC;
	signed int acceptableError;
	unsigned char newAI;
	unsigned char auxModulation;

	/* Only for loop interaction with GUI */
	signed int value;
	char *argv[CFG_MAXARGS + 1], argc, *p = '\0';
	signed int errorMeasuredADC = 0;
	unsigned char flagsMessagues = 1;

	/* Stable procedure */
	unsigned char auxStable = 0;

	auxModulation = 1;
	if (!CHAN_5G(channel) && IS_CCK_DSSS(rate))
		auxModulation = 0;

	if (auxModulation)
	{
		/* Initial conditions loop */
		dt = (float) 40; // from 20
		kp = (float) 15.76; //from 15,76 to 15
		kd = (float) 2.0 * 15.76; // from 2.0 to 2.5 
		ki = (float) 15.76 / 2.0;
		acceptableError = 5;
	}
	else
	{
		/* Initial conditions loop */
		dt = (float) 20; // from 20
		kp = (float) 15.76; //from 15,76 to 15
		kd = (float) 2.0 * 15.76; // from 2.0 to 2.5 
		ki = (float) 15.76 / 2.0;
		acceptableError = 3;
	}

	/* Estimate ADC reading based in desired Output Power */
	desiredADC = getADCfromPW(auxCalData, desiredPower, channel-1, rate);
	#if CALIBRATIONTEST
	printf("First desired ADC value: %i\n", desiredADC);
	#endif
	
	/* Sets Airoha Index to estimated */
	newAI = getAIfromADC(auxCalData, (unsigned int) desiredADC, channel-1, rate);
	#if CALIBRATIONTEST
	printf("First desired AI value: %i\n", newAI);
	#endif
	MacSetDirectTxPower(newAI);
	
	/* Performs first reading after estimation */
	reset_max_min_values();
	waitUS(30000);
	read_max_conversion ((unsigned char *) &avgADC); // int before

	while (1)
	{
		while (!tstc())
		{

			error = (float) (desiredADC - (avgADC + errorMeasuredADC));
			#if CALIBRATIONTEST
			printf ("Error: %i, desiredADC: %i, measuredADC: %i, AI: %i\n", (int) error, desiredADC, (avgADC + errorMeasuredADC), newAI);
			#endif
			
			if ( error > acceptableError || error < (-1 * acceptableError) ) // 5 is ok, 4 is ok
			{
				/* PID calculations */
				integral += error * dt;
				derivative = (error - previous_error) / dt;
				output = desiredADC + (kp * error + ki * integral + kd * derivative)/1000;//1000 before
				
				/* Limiting output values */
				if ( output > 255)
					output = 255.0;
					//newAI = 63;
				else if (output < 0)
					output = 0;
				
				/* Airoha Index Estimation */
				newAI = getAIfromADC(auxCalData, (unsigned int) output, channel-1, rate);
				
				/* Limiting index values */
				/* DElete when confirmed that output limit is enough. */
				//if (CHAN_5G(channel))
				//	if (newAI > auxCalData->cal_curves_a[channel].max_power_index)
				//		newAI = auxCalData->cal_curves_a[channel].max_power_index;
				//else
				//	if (newAI > auxCalData->cal_curves_bg[channel][auxModulation].max_power_index)
				//		newAI = auxCalData->cal_curves_bg[channel][auxModulation].max_power_index;

				MacSetDirectTxPower(newAI);

				#if CALIBRATIONTEST
				printf ("----------------------------\n");
				printf ("Integral: %i\n", (int) integral);
				printf ("Derivative: %i\n", (int) derivative);
				printf ("kp * error: %i\n", (int) (kp * error));
				printf ("ki * integral: %i\n", (int) (ki * integral));
				printf ("kd * derivative: %i\n", (int) (kd * derivative));
				printf ("Output: %i\n", (int) output);
				printf ("New Airoha Index: %i\n", newAI);
				printf ("----------------------------\n");
				#endif
				
				/* If corrections are performed, we delete stability flag */
				auxStable = 0;
				if (!flagsMessagues)
				{
					printf ("System compensating for desired RF: %i\n", desiredPower);
					flagsMessagues = 1;
				}
			}
			else if (auxStable > 15) // From 5.
			{
				
				integral = 0.0;
				if (flagsMessagues)
				{
					printf ("SYSTEM STABLE\nDesired ADC: %i, Measured ADC: %i, AI Index: %i\n", \
						desiredADC, avgADC, newAI);
					flagsMessagues = 0;
				}
				
			}
			else
			{
				auxStable++;
			}

			previous_error = error;
				
			reset_max_min_values();
			waitUS(30000);
			read_max_conversion ((unsigned char *) &measuredADC);
			
			if (auxModulation)
			{
				avgADC += measuredADC;
				avgADC /= 2;
			}
			else
				avgADC = measuredADC;
		}
				
		readline("");
		
		/* Extract arguments */
		if ((argc = parse_line (console_buffer, argv)) == 0)
		{
			continue;
		}
				
		value = simple_strtol (argv[1], &p, 10);
		
		switch (argv[0][0])
		{
			case 'q':
				goto EXIT_SUB_CONT_TX;
			break;
		
			case 'p':
				MacSetDirectTxPower((unsigned char) value);
			break;
			
			case 'r':
				errorMeasuredADC = value;
			break;

			case 'd':
				desiredPower = value;
				desiredADC = getADCfromPW(auxCalData, value, channel-1, rate);
				integral = 0.0;
				derivative = 0.0;
			break;

			default:
			break;
		}
	}

	EXIT_SUB_CONT_TX:
	{
		NOP;
	}

	return 0;
}

#endif /* CFG_HAS_WIRELESS */
