#include <common.h>

#if (CONFIG_COMMANDS & CFG_CMD_BSP && \
     defined(CONFIG_UBOOT_CMD_BSP_TESTHW) && \
     defined(CFG_HAS_WIRELESS))

#include "calibration.h"
/*#include "fpArithmetic.h"*/
#include "gui_tst_wifi.h"
#include "vBPiper.h"
#include "adc081C021.h"
#include "AIROHA_7230.h"

extern wcd_data_t calDataObtained;

int parse_line (char *line, char *argv[])
{
	int nargs = 0;

	while (nargs < CFG_MAXARGS) {
		/* skip any white space */
		while ((*line == ' ') || (*line == '\t')) {
			++line;
		}

		if (*line == '\0') {	/* end of line, no more args	*/
			argv[nargs] = NULL;
			return (nargs);
		}

		argv[nargs++] = line;	/* begin of argument string	*/

		/* find end of string */
		while (*line && (*line != ' ') && (*line != '\t')) {
			++line;
		}

		if (*line == '\0') {	/* end of line, no more args	*/
			argv[nargs] = NULL;
			return (nargs);
		}

		*line++ = '\0';		/* terminate current arg	 */
	}

	printf ("** Too many args (max. %d) **\n", CFG_MAXARGS);

	return (nargs);
}

void printMenu (const unsigned char message[][80], const unsigned char num_of_lines)
{
	unsigned char aux;
	
	for(aux = 0; aux < num_of_lines; aux++)
	{
		printf ("%s", message[aux]);	
	}
}

int getSLongConsole(long *returned)
{
	int aux;
		
	aux = readline(RADIO_TST_PROMPT);
	
	if (aux > 0)
       	{
        	*returned = simple_strtol (console_buffer, NULL, 10);
       	}
       	return aux;
}

#if 0
int getSFP16_8ConsoleNOREAD (char *auxConBuff, FIXED16_8 *returned, unsigned char decimalSeparator, char **p)
{
	/*int aux;*/
	//char *p;

	//aux = readline(RADIO_TST_PROMPT);
	/*if (aux > 0)*/
       	/*{*/
		returned->part.integer = simple_strtol (auxConBuff, p, 10);
		if (**p == decimalSeparator)// || *p == ',')
			returned->part.fraction = simple_strtol ((*p+1), p, 10);
		else
			returned->part.fraction = 0;
	/*}*/
       	/*return aux;*/
	return 0;
}
#endif
int getSLongConsoleNOREAD(char *auxConBuff, long *returned, char **p)
{
	/*int aux;*/
		
	/*if (aux > 0)*/
       	/*{*/
        	*returned = simple_strtol (auxConBuff, p, 10);
       	/*}*/
       	/*return aux;*/
	return 0;
}

#if 0
int retrieveLineConsole (lineDefinition *auxLine, unsigned char decimalSeparator, \
			    unsigned char valuesSeparator, unsigned char endProcessSeparator, \
			    unsigned char lineNumber)
{
	int aux;
	char *p = NULL;
	
	printf ("Input_line_%i>>", lineNumber);
	aux = readline("");
	if (aux > 0)
       	{
		getSFP16_8ConsoleNOREAD(console_buffer, &(auxLine->slope), decimalSeparator, &p);
		p++;
		getSFP16_8ConsoleNOREAD(p, &(auxLine->intercept), decimalSeparator, &p);
		p++;
		auxLine->maxIndex = 0;
		getSLongConsoleNOREAD(p, &aux, &p);
		auxLine->maxIndex = (unsigned long) (0x0000FFFF & aux); /* Max index */
		p++;
		getSLongConsoleNOREAD(p, &aux, &p);
		auxLine->maxIndex |= (unsigned long) ((0x0000FFFF & aux) << 16); /* Min Index */
		auxLine->crc = 0;
		auxLine->crc = crc32 (0, (unsigned char*) auxLine, sizeof(auxLine));
	}
       	return -1;
}
#endif

unsigned int validateValue(signed int *toValidate, enum validateInputs *inputType)
{
	/* The condition >= 0 is not necessary while toValidate is unsigned */
	switch	(*inputType)
	{
		case TXPOWER:
			if (*toValidate >= 0 && *toValidate <= 63)
				return 0;
			else
				return 1;
		break;
		case RFTXPOWER:
			if (*toValidate >= -10000 && *toValidate <= 16000)
				return 0;
			else
				return 1;
		break;
		case TRANSMIT_MODE:
			if (*toValidate >= 0 && *toValidate <= 4)
				return 0;
			else
				return 1;
		break;
		case TRANSMIT_RATE:
			if (*toValidate >= 0 && *toValidate <= 11)
				return 0;
			else
				return 1;
		break;
		case CHANNEL:
			if ( (*toValidate >= 1) && (*toValidate <= 49) )
				return 0;
			else
				return 1;
		break;
		case FRAME_PERIOD:
			if (*toValidate >= 0 && *toValidate <= 100)
				return 0;
			else
				return 1;
		break;
		case FRAME_LENGTH:
			if (*toValidate >= 0 && *toValidate <= 1)
				return 0;
			else
				return 1;
		break;
		default:
			return 1;
		break;
	}
}

unsigned char setControlledTxPower (struct menuStatus *auxMenuStatus)
{
	if (auxMenuStatus->power_control == TRUE)
		/* Measure ADC value */
		/* Transform ADC value */
		/* Decivde if corrective measures are necessary */
		/* Correct output power within the limits */
		NOP;
	else
		MacSetDirectTxPower(auxMenuStatus->txpower);

	return 0;
}

unsigned char setControlledChannel (struct menuStatus *auxMenuStatus)
{
	/* Assuming if rate 1Mbps nulled, complete channel is nulled as well */
	//if (auxMenuStatus->calData.maxIndexBand_B_G[auxMenuStatus->channel-1][0] == 0)
	if(0)	
	{
		//printf ("Value %d",auxMenuStatus->calData.maxIndexBand_B_G[auxMenuStatus->channel-1][0]);
		printf ("Channel %d is blocked via software.\n", auxMenuStatus->channel);
		return 1; /* Channel not allowed */
	}
	else
	{
		SetChannel(auxMenuStatus->channel);
		return 0;
	}
}

unsigned char applyWirelessSettings (struct menuStatus *auxMenuStatus)
{
	/*if (setControlledChannel(auxMenuStatus))
		return 1;*/
	SetChannel(auxMenuStatus->channel);
	
	if (auxMenuStatus->statusInformation == RADIO_TX_CONTINUOUS_WIRELESS_TEST ||
	    auxMenuStatus->statusInformation == RADIO_START_CONTINUOUS_TRANSMIT)
	{
		MacContinuousTransmit(1, auxMenuStatus->transmit_mode, auxMenuStatus->transmit_rate);	

	}
	else if (auxMenuStatus->statusInformation == RADIO_TX_PERIODIC_FRAME_TRANSMIT ||
		 auxMenuStatus->statusInformation == RADIO_START_TX_PERIODIC_FRAME_TRANSMIT ||
		 auxMenuStatus->statusInformation == RADIO_SWITCH_POWER_CONTROL_LOOP)
	{
		MacRadioTXPeriodic(1, auxMenuStatus->transmit_mode, \
				auxMenuStatus->transmit_rate, \
				1000, \
				auxMenuStatus->frame_period, \
				auxMenuStatus->frame_length);		
	}

	/*if (setControlledTxPower(auxMenuStatus))
		return 1;*/
	
	return 0;
}

int menuRadioTxContinuousWirelessTest(struct menuStatus *auxMenuStatus)
{
	/*struct menuStatus auxMenuStatus; */
	
	long auxEnteredValue;
	/* WARNING */
	/* unsigned int enteredValue; */
	signed int enteredValue;
	enum validateInputs enteredValueType;
	
	enteredValueType = TXPOWER;
	do
	{
		printMenu (menu_Request_TX_Power, menu_Request_TX_Power_size);
		getSLongConsole(&auxEnteredValue);
		enteredValue = (signed int) auxEnteredValue;
	} while (validateValue (&enteredValue, &enteredValueType));

	auxMenuStatus->txpower = enteredValue;
		
	enteredValueType = TRANSMIT_MODE;
	do
	{	
		printMenu (menu_Request_TX_Mode, menu_Request_TX_Mode_size);
		getSLongConsole(&auxEnteredValue);
		enteredValue = (signed int) auxEnteredValue;
	} while (validateValue (&enteredValue, &enteredValueType));
	
	auxMenuStatus->transmit_mode = enteredValue;
	
	enteredValueType = TRANSMIT_RATE;
	do
	{	
		printMenu (menu_Request_TX_Rate, menu_Request_TX_Rate_size);
		getSLongConsole(&auxEnteredValue);
		enteredValue = (signed int) auxEnteredValue;
	} while (validateValue (&enteredValue, &enteredValueType));
	
	
	auxMenuStatus->transmit_rate = enteredValue;
		
	enteredValueType = CHANNEL;
	do
	{
		printMenu (menu_Request_TX_Channel, menu_Request_TX_Channel_size);
		getSLongConsole(&auxEnteredValue);
		enteredValue = (signed int) auxEnteredValue;
	} while (validateValue (&enteredValue, &enteredValueType));
	
	auxMenuStatus->channel = enteredValue;
	
	if (applyWirelessSettings (auxMenuStatus))
	{
		printf ("One or more errors occured. Settings not activated.\n");
		return 1;
	}
		

	/*printf("Channel: %d -- Tx Power: %d -- Tx Mode: %s -- Tx Rate: %s\n", \
		auxMenuStatus->channel, \ 
		auxMenuStatus->txpower, \
		nTs_transmit_mode[auxMenuStatus->transmit_mode], \
		nTs_transmit_rate[auxMenuStatus->transmit_rate]);*/

	/*SetChannel(auxMenuStatus->channel);
	MacContinuousTransmit(1, auxMenuStatus->transmit_mode, auxMenuStatus->transmit_rate);
	MacSetDirectTxPower(auxMenuStatus->txpower);*/
	
	subMenuRadioTxContinuousWirelessTest(auxMenuStatus);
	return 0;
}


void subMenuRadioTxContinuousWirelessTest(struct menuStatus *auxMenuStatus)
{
	signed int value;
	unsigned char instantadc;
	unsigned long avgadc, maxadc;
	unsigned char loopadc;
	enum validateInputs valueType;
	
	char *argv[CFG_MAXARGS + 1];
	// Delete if not error: char finaltoken[CFG_CBSIZE];
	char argc;
	char *p = '\0';

		
	while (1)
	{
		printf("Channel: %d -- Tx Power: %d -- Tx Mode: %s -- Tx Rate: %s\n", \
			freqTableAiroha_7230[auxMenuStatus->channel].channelFrequency, \
			auxMenuStatus->txpower, \
			nTs_transmit_mode[auxMenuStatus->transmit_mode], \
			nTs_transmit_rate[auxMenuStatus->transmit_rate]);
		
		printMenu (submenu_Radio_Tx_Cont_Wir_Test, submenu_Radio_Tx_Cont_Wir_Test_size);
		printf(RADIO_TST_PROMPT);
	
		do
		{
			NOP;
		} while (!tstc());
				
		readline("");
		
		/* Extract arguments */
		if ((argc = parse_line (console_buffer, argv)) == 0) {
			continue;
		}
				
		value = (signed int) simple_strtol (argv[1], &p, 10);
		
		switch (argv[0][0])
		{
			/* Change channel */
			case 'c':
				valueType = CHANNEL;
				if (validateValue (&value, &valueType))
					goto ERROR_SUB_CONT_TX;
				else	
					auxMenuStatus->channel = value;
				// To prevent the carrier appear as a peak with OFDM modulation, we have first to cancel the continuous
				// TX before changing the channel.
				//MacContinuousTransmit(0, auxMenuStatus->transmit_mode, auxMenuStatus->transmit_rate);
				//SetChannel(auxMenuStatus->channel);
				//MacSetDirectTxPower(auxMenuStatus->txpower);
				//MacContinuousTransmit(1, auxMenuStatus->transmit_mode, auxMenuStatus->transmit_rate);
				MacContinuousTransmit(0, auxMenuStatus->transmit_mode, auxMenuStatus->transmit_rate);
				waitUS(100000); // If change channel in same moment, then appeads peaks in CCK/PSK.
				SetChannel(auxMenuStatus->channel);
				//setControlledChannel(auxMenuStatus->channel);
				MacContinuousTransmit(1, auxMenuStatus->transmit_mode, auxMenuStatus->transmit_rate);
				MacSetDirectTxPower(auxMenuStatus->txpower);
			break;
			/* Change rate */
			case 'm':
				valueType = TRANSMIT_RATE;
				if (validateValue (&value, &valueType))
					goto ERROR_SUB_CONT_TX;
				else
				{
					MacContinuousTransmit(0, auxMenuStatus->transmit_mode, auxMenuStatus->transmit_rate);
					auxMenuStatus->transmit_rate = value;
				}
				/* Check why I need a delay ?*/
				/* TBD */
				//waitUS(100000);
				//SetChannel(auxMenuStatus->channel);
				//MacSetDirectTxPower(auxMenuStatus->txpower);
				//MacContinuousTransmit(1, auxMenuStatus->transmit_mode, auxMenuStatus->transmit_rate);
				//MacUpdateFccBufferX(auxMenuStatus->transmit_mode, auxMenuStatus->transmit_rate);
				MacContinuousTransmit(0, auxMenuStatus->transmit_mode, auxMenuStatus->transmit_rate);
				waitUS(100000); // If change channel in same moment, then appears peaks in CCK/PSK.
				SetChannel(auxMenuStatus->channel);
				MacContinuousTransmit(1, auxMenuStatus->transmit_mode, auxMenuStatus->transmit_rate);
				/*#if CONFIG_CCW9P9215
					waitUS(100000); // If change channel in same moment, then appears peaks in CCK/PSK.
				#endif*/
				MacSetDirectTxPower(auxMenuStatus->txpower);
			break;
			/* Increase power */
			case '+':
				if (auxMenuStatus->txpower == 63)
				{
					printf ("Maximum output power index reached.\n");
					goto ERROR_SUB_CONT_TX;
				}
				else	
					auxMenuStatus->txpower++;
				MacSetDirectTxPower(auxMenuStatus->txpower);
			break;
			/* Decrease power */
			case '-':
				if (auxMenuStatus->txpower == 0)
				{
					printf ("Minimum output power index reached.\n");
					goto ERROR_SUB_CONT_TX;
				}
				else
					auxMenuStatus->txpower--;
				MacSetDirectTxPower(auxMenuStatus->txpower);
			break;
			/* Change power */
			case 'p':
				valueType = TXPOWER;
				if (validateValue (&value, &valueType))
					goto ERROR_SUB_CONT_TX;
				else	
					auxMenuStatus->txpower = value;
				MacSetDirectTxPower(auxMenuStatus->txpower);
			break;
			/* Obtain instant value from ADC*/
			case 'i':
				read_conversion (&instantadc);
				printf("%u\n",instantadc);
				
			break;
			/* Obtain averaged value from ADC*/
			case 'a':
				avgadc = 0;
				maxadc = 0;
				reset_max_min_values();
				for(loopadc=0; loopadc<100; loopadc++) 
				{	
					waitUS(1000);
					read_conversion (&instantadc);
					avgadc += instantadc;
				}
				avgadc /= 100;
				read_max_conversion ((unsigned char *) &maxadc);
				printf("%u, %u\n",avgadc, maxadc);
				
			break;
			/* Exit subMenu */
			case 'q':
				//MacContinuousTransmit(0, auxMenuStatus->transmit_mode, auxMenuStatus->transmit_rate);
				goto EXIT_SUB_CONT_TX;
			break;
			/* Show error, repeat subMenu */
			default:
				printf ("\nWrong value, try again:\n");
				ERROR_SUB_CONT_TX:
			break;
		}
	}
	
	EXIT_SUB_CONT_TX:
	{
		NOP;
	}

}

int menuRadioTxPeriodicWirelessTest(struct menuStatus *auxMenuStatus)
{
	/* struct menuStatus auxMenuStatus; */
	
	long auxEnteredValue;
	/* WARNING */
	/* unsigned int enteredValue; */
	signed int enteredValue;
	enum validateInputs enteredValueType;

	enteredValueType = TXPOWER;
	do
	{
		printMenu (menu_Request_TX_Power, menu_Request_TX_Power_size);
		getSLongConsole(&auxEnteredValue);
		enteredValue = (signed int) auxEnteredValue;
	} while (validateValue (&enteredValue, &enteredValueType));
	auxMenuStatus->txpower = enteredValue;

	enteredValueType = FRAME_PERIOD;
	do
	{	
		printMenu (menu_Request_TX_Frame_Period, menu_Request_TX_Frame_Period_size);
		getSLongConsole(&auxEnteredValue);
		enteredValue = (signed int) auxEnteredValue;
	} while (validateValue (&enteredValue, &enteredValueType));
	auxMenuStatus->frame_period = enteredValue;

	enteredValueType = FRAME_LENGTH;
	do
	{	
		printMenu (menu_Request_TX_Frame_Length, menu_Request_TX_Frame_Length_size);
		getSLongConsole(&auxEnteredValue);
		enteredValue = (signed int) auxEnteredValue;
	} while (validateValue (&enteredValue, &enteredValueType));
	auxMenuStatus->frame_length = enteredValue;

	
	enteredValueType = TRANSMIT_RATE;
	do
	{	
		printMenu (menu_Request_TX_Rate, menu_Request_TX_Rate_size);
		getSLongConsole(&auxEnteredValue);
		enteredValue = (signed int) auxEnteredValue;
	} while (validateValue (&enteredValue, &enteredValueType));
	auxMenuStatus->transmit_rate = enteredValue;
		
	enteredValueType = CHANNEL;
	do
	{
		printMenu (menu_Request_TX_Channel, menu_Request_TX_Channel_size);	  
		getSLongConsole(&auxEnteredValue);
		enteredValue = (signed int) auxEnteredValue;
	} while (validateValue (&enteredValue, &enteredValueType));
	auxMenuStatus->channel = enteredValue;
	
	if (applyWirelessSettings (auxMenuStatus))
	{
		printf ("One or more errors occured. Settings not activated.\n");
		return 1;
	}
		
	/*printf("Channel: %d -- Tx Power: %d -- Tx Rate: %s -- Frame Length: %s -- Frame Rate: %d ms. --\n",
		auxMenuStatus->channel, \
		auxMenuStatus->txpower, \
		nTs_transmit_rate[auxMenuStatus->transmit_rate], \
		nTs_frame_length[auxMenuStatus->frame_length], \
		auxMenuStatus->frame_period);*/
	
	//applyWirelessSettings (auxMenuStatus);

	/*SetChannel(auxMenuStatus->channel);
	MacRadioTXPeriodic(1, auxMenuStatus->transmit_mode, \
			      auxMenuStatus->transmit_rate, \
			      1000, \
			      auxMenuStatus->frame_period, \
			      auxMenuStatus->frame_length);
	MacSetDirectTxPower(auxMenuStatus->txpower);*/
	
	subMenuRadioTxPeriodicWirelessTest(auxMenuStatus);
	return 0;
}

void subMenuRadioTxPeriodicWirelessTest(struct menuStatus *auxMenuStatus)
{
	signed int value;
	unsigned char maxadc;
	enum validateInputs valueType;
	
	char *argv[CFG_MAXARGS + 1];
	/*char finaltoken[CFG_CBSIZE];*/
	char argc;
	char *p = '\0';

	int auxPAStatus;
	int auxFTXStatus;


	auxPAStatus = 0x000A3;
	auxFTXStatus = 0x221BB;
		
	while (1)
	{
		printf("Channel: %d -- Tx Power: %d -- Tx Rate: %s -- Frame Length: %s -- Frame Rate: %d ms. --\n",
			freqTableAiroha_7230[auxMenuStatus->channel].channelFrequency, \
			auxMenuStatus->txpower, \
			nTs_transmit_rate[auxMenuStatus->transmit_rate], \
			nTs_frame_length[auxMenuStatus->frame_length], \
			auxMenuStatus->frame_period);

		printf("\nPA Value: %x\n", auxPAStatus);

		printMenu (submenu_Radio_Tx_Cont_Wir_Test, submenu_Radio_Tx_Cont_Wir_Test_size);
		printf(RADIO_TST_PROMPT);

		do
		{
			NOP;
		} while (!tstc());
				
		readline("");
		
		/* Extract arguments */
		if ((argc = parse_line (console_buffer, argv)) == 0) {
			continue;
		}
				
		value = (signed int) simple_strtol (argv[1], &p, 10);
		
		switch (argv[0][0])
		{
			/* Change channel */
			case 'c':
				valueType = CHANNEL;
				if (validateValue (&value, &valueType))
					goto ERROR_SUB_CONT_TX;
				else	
					auxMenuStatus->channel = value;
				//setControlledChannel(auxMenuStatus->channel);
				//SetChannel(auxMenuStatus->channel);
				MacRadioTXPeriodic(0, auxMenuStatus->transmit_mode, 
			      				      auxMenuStatus->transmit_rate, \
			      				      1000, \
			      				      auxMenuStatus->frame_period, \
			      				      auxMenuStatus->frame_length);
				waitUS(100000); // If change channel in same moment, then appeads peaks in CCK/PSK.
				SetChannel(auxMenuStatus->channel);
				//setControlledChannel(auxMenuStatus->channel);
				MacRadioTXPeriodic(1, auxMenuStatus->transmit_mode, 
			      				      auxMenuStatus->transmit_rate, \
			      				      1000, \
			      				      auxMenuStatus->frame_period, \
			      				      auxMenuStatus->frame_length);
				/*#if CONFIG_CCW9P9215
					waitUS(100000); // If change channel in same moment, then appears peaks in CCK/PSK.
				#endif*/
				MacSetDirectTxPower(auxMenuStatus->txpower);
			break;
			/* Change rate */
			case 'm':
				valueType = TRANSMIT_RATE;
				if (validateValue (&value, &valueType))
					goto ERROR_SUB_CONT_TX;
				else
				{
					MacRadioTXPeriodic(0, auxMenuStatus->transmit_mode, 
			      				      auxMenuStatus->transmit_rate, \
			      				      1000, \
			      				      auxMenuStatus->frame_period, \
			      				      auxMenuStatus->frame_length);
					auxMenuStatus->transmit_rate = value;
				}
				/* Check why I need a delay ?*/
				/* TBD */
				waitUS(100000);
				MacRadioTXPeriodic(1, auxMenuStatus->transmit_mode, \
			      			      auxMenuStatus->transmit_rate, \
			      			      1000, \
			      			      auxMenuStatus->frame_period, \
			      			      auxMenuStatus->frame_length);
				//MacUpdateFccBufferX(auxMenuStatus->transmit_mode, auxMenuStatus->transmit_rate);
				break;
			/* Increase power */
			case '+':
				if (auxMenuStatus->txpower == 63)
				{
					printf ("Maximum output power index reached.\n");
					goto ERROR_SUB_CONT_TX;
				}
				else	
					auxMenuStatus->txpower++;
				MacSetDirectTxPower(auxMenuStatus->txpower);
			break;
			/* Decrease power */
			case '-':
				if (auxMenuStatus->txpower == 0)
				{
					printf ("Minimum output power index reached.\n");
					goto ERROR_SUB_CONT_TX;
				}
				else
					auxMenuStatus->txpower--;
				MacSetDirectTxPower(auxMenuStatus->txpower);
			break;
			/* Change power */
			case 'p':
				valueType = TXPOWER;
				if (validateValue (&value, &valueType))
					goto ERROR_SUB_CONT_TX;
				else	
					auxMenuStatus->txpower = value;
				MacSetDirectTxPower(auxMenuStatus->txpower);
			break;
			/* Obtain averaged value from ADC*/
			case 'a':
				reset_max_min_values();
				waitUS(200000);
				read_max_conversion (&maxadc);
				printf("%u\n",maxadc);
				
			break;

			case '1':
				/*valueType = TRANSMIT_RATE;
				MacRadioTXPeriodic(0, auxMenuStatus->transmit_mode, 
			      			      auxMenuStatus->transmit_rate, \
			      			      1000, \
			      			      auxMenuStatus->frame_period, \
			      			      auxMenuStatus->frame_length);*/
				
				auxPAStatus &= ((~0x7) << 6);
				auxPAStatus |= ((value & 0x7) << 6);
				WriteRF(12, auxPAStatus);
				
				//waitUS(100000);
				/*MacRadioTXPeriodic(1, auxMenuStatus->transmit_mode, \
			      			      auxMenuStatus->transmit_rate, \
			      			      1000, \
			      			      auxMenuStatus->frame_period, \
			      			      auxMenuStatus->frame_length);*/
				
			break;

			case '2':
				/*valueType = TRANSMIT_RATE;
				MacRadioTXPeriodic(0, auxMenuStatus->transmit_mode, 
			      			      auxMenuStatus->transmit_rate, \
			      			      1000, \
			      			      auxMenuStatus->frame_period, \
			      			      auxMenuStatus->frame_length);*/
				
				auxPAStatus &= (~0x7 << 3);
				auxPAStatus |= (value << 3);
				WriteRF(12, auxPAStatus);
				
				/*waitUS(100000);
				MacRadioTXPeriodic(1, auxMenuStatus->transmit_mode, \
			      			      auxMenuStatus->transmit_rate, \
			      			      1000, \
			      			      auxMenuStatus->frame_period, \
			      			      auxMenuStatus->frame_length);*/
				
			break;

			case '3':
				/*valueType = TRANSMIT_RATE;
				MacRadioTXPeriodic(0, auxMenuStatus->transmit_mode, 
			      			      auxMenuStatus->transmit_rate, \
			      			      1000, \
			      			      auxMenuStatus->frame_period, \
			      			      auxMenuStatus->frame_length);*/
				
				auxPAStatus &= (~0x7);
				auxPAStatus |= (value);
				WriteRF(12, auxPAStatus);
				
				/*waitUS(100000);
				MacRadioTXPeriodic(1, auxMenuStatus->transmit_mode, \
			      			      auxMenuStatus->transmit_rate, \
			      			      1000, \
			      			      auxMenuStatus->frame_period, \
			      			      auxMenuStatus->frame_length);*/
				
			break;

			case 'b':
				valueType = TRANSMIT_RATE;
				MacRadioTXPeriodic(0, auxMenuStatus->transmit_mode, 
			      			      auxMenuStatus->transmit_rate, \
			      			      1000, \
			      			      auxMenuStatus->frame_period, \
			      			      auxMenuStatus->frame_length);
				
				auxFTXStatus &= (0xFFFFFF & (~0x3 << 18));
				auxFTXStatus |= (value << 18);
				WriteRF(9, auxFTXStatus);
				
				waitUS(100000);
				MacRadioTXPeriodic(1, auxMenuStatus->transmit_mode, \
			      			      auxMenuStatus->transmit_rate, \
			      			      1000, \
			      			      auxMenuStatus->frame_period, \
			      			      auxMenuStatus->frame_length);
				
			break;

			/* Exit subMenu */
			case 'q':
				//MacRadioTXPeriodic(0, auxMenuStatus->transmit_mode, auxMenuStatus->transmit_rate, 100);
				goto EXIT_SUB_CONT_TX;
			break;
			/* Show error, repeat subMenu */
			default:
				printf ("\nWrong value, try again:\n");
				ERROR_SUB_CONT_TX:
			break;
		}
	}
	
	EXIT_SUB_CONT_TX:
	{
	
	}

}

int menuRadioTxPeriodicControlLoop(struct menuStatus *auxMenuStatus)
{
	/* struct menuStatus auxMenuStatus; */
	
	long auxEnteredValue;
	/* WARNING */
	/* unsigned int enteredValue; */
	signed int enteredValue;
	enum validateInputs enteredValueType;

	enteredValueType = RFTXPOWER;
	do
	{
		printMenu (menu_Request_RF_TX_Power, menu_Request_RF_TX_Power_size);
		getSLongConsole(&auxEnteredValue);
		enteredValue = (signed int) auxEnteredValue;
	} while (validateValue ((signed int *) &auxEnteredValue, &enteredValueType));
	auxMenuStatus->rftxpower = enteredValue;

	auxMenuStatus->frame_period = 20;
	auxMenuStatus->frame_length = 0;

	enteredValueType = TRANSMIT_RATE;
	do
	{	
		printMenu (menu_Request_TX_Rate, menu_Request_TX_Rate_size);
		getSLongConsole(&auxEnteredValue);
		enteredValue = (signed int) auxEnteredValue;
	} while (validateValue (&enteredValue, &enteredValueType));
	auxMenuStatus->transmit_rate = enteredValue;
		
	enteredValueType = CHANNEL;
	do
	{
		printMenu (menu_Request_TX_Channel, menu_Request_TX_Channel_size);	  
		getSLongConsole(&auxEnteredValue);
		enteredValue = (signed int) auxEnteredValue;
	} while (validateValue (&enteredValue, &enteredValueType));
	auxMenuStatus->channel = enteredValue;
	
	if (applyWirelessSettings (auxMenuStatus))
	{
		printf ("One or more errors occured. Settings not activated.\n");
		return 1;
	}
		
	return 0;
}

void menuRadioSilentRX(struct menuStatus *auxMenuStatus)
{
	long auxEnteredValue;
	signed int enteredValue;
	unsigned int tst;
	enum validateInputs enteredValueType;

	enteredValueType = CHANNEL;
	do
	{
		printMenu (menu_Request_TX_Channel, menu_Request_TX_Channel_size);	  
		getSLongConsole(&auxEnteredValue);
		enteredValue = (signed int) auxEnteredValue;
	} while (validateValue (&enteredValue, &enteredValueType));
	
	auxMenuStatus->channel = enteredValue;	

	setControlledChannel(auxMenuStatus);
	MacRadioRXTest(1);
	MacReadPacketFromBuffer(&tst);
	/* 13/07/09 */
	MacRadioRXTest(0); /* This shoudl be the correct way. Test and delete comment */
	printf("Received frames: %u\n", tst);
}

/**************************************************************
 * tst_wifi()
 **************************************************************/
int tst_wifi (struct menuStatus *auxMenuStatus)
{
	/* int i; */
	
	/* For user input */
        int error_code;
        long read_value;
	char *s;

	/* Initializes ADC from wireless for power control */
	init_adc_pc();

	//displayPowerLimitValues(&(auxMenuStatus->calData));

	/* Displays the main menu listing all the tests */
	printMenu (menu_principal_header, menu_principal_header_size);
	
	/* Declared in vBPiper.h */
	//#ifdef IBSSOFF
		SetBSS (0, (unsigned char *) "TESTING123", 10, 0xFFFF, 10); /* "TESTING123" */
    	//#endif

    	/* Main loop for program interaction */
    	while (1)
    	{
        	/* Displays the main menu listing all the tests */
		printMenu (menu_principal_body, menu_principal_body_size);
		
              	/* get a line of input from the user */
	        error_code = getSLongConsole(&read_value);
                if (error_code < 0)
              	{
	 		printf ("\n Wrong value, try again:\n");
	                return 0;
        	}
                else
		{
			auxMenuStatus->statusInformation = (unsigned char) read_value;

			if ( (auxMenuStatus->statusInformation >= RADIO_TX_CONTINUOUS_WIRELESS_TEST) \
			      && (auxMenuStatus->statusInformation <= RADIO_EXIT) )
                	{
				switch (auxMenuStatus->statusInformation)
	        		{
					case RADIO_TX_CONTINUOUS_WIRELESS_TEST:
			        		menuRadioTxContinuousWirelessTest(auxMenuStatus);
						MacContinuousTransmit(0, auxMenuStatus->transmit_mode, auxMenuStatus->transmit_rate);
			        	break;

			        	case RADIO_START_CONTINUOUS_TRANSMIT:
						menuRadioTxContinuousWirelessTest(auxMenuStatus);
			        	break;

			        	case RADIO_STOP_CONTINUOUS_TRANSMIT:
			        		MacContinuousTransmit(0, auxMenuStatus->transmit_mode, auxMenuStatus->transmit_rate);
			        	break;

			        	case RADIO_RX_SILENT_WIRELESS_TEST:
			        		menuRadioSilentRX (auxMenuStatus);
						//MacRadioRXTest(0);
			        	break;

			        	case RADIO_TX_PERIODIC_FRAME_TRANSMIT:
						menuRadioTxPeriodicWirelessTest(auxMenuStatus);
						MacRadioTXPeriodic(0, auxMenuStatus->transmit_mode, \
			      					      auxMenuStatus->transmit_rate, \
			      				              1000, \
			      					      auxMenuStatus->frame_period,  \
			      					      auxMenuStatus->frame_length);
			        	break;

			        	case RADIO_START_TX_PERIODIC_FRAME_TRANSMIT:
			        		menuRadioTxPeriodicWirelessTest(auxMenuStatus);
			        	break;
						
					case RADIO_STOP_TX_PERIODIC_FRAME_TRANSMIT:
						MacRadioTXPeriodic(0, auxMenuStatus->transmit_mode, \
								      auxMenuStatus->transmit_rate, \
							              1000, \
								      auxMenuStatus->frame_period,  \
								      auxMenuStatus->frame_length);
					break;
			
					case RADIO_WRITE_RF:
						printf ("Write to RF is not yet available.\n");
						for (error_code = 0; error_code < 255; error_code++)
						{
							printf ("Index: %i, ADC: %i\n", (unsigned int) getAIfromADC(&calDataObtained, error_code, 1, 0), error_code);
						}
					break;

				        case RADIO_READ_MAC_ADDRESS:
				        	if ((s = getenv("wlanaddr")) != NULL)
						{
							printf("%s\n", s);
						}
				        break;

				        case RADIO_SET_CALIBRATION_COEF:
						retrieveCalData(&calDataObtained ,'.',',',';');
						printf ("Returned value SaveInFlash: %i\n", NvPrivWCDSaveInFlash(&calDataObtained));
						//waitUS(1000000);
						//printf ("Returned value GetFromFlashAndSaveInNvram: %i\n", NvPrivWCDGetFromFlashAndSetInNvram());
						/* This will go to another function in order to do the following */
						/* Check if module calibrated and test pass*/
						/* If calibrated and tested, modification is not possible */
						/* if test not passed, then calibration is possible */
						/*for (i=0; i<7; i++)
						{
							retrieveLineConsole (&calData[i], '.',',',';', i);
						}*/
						/* Write calibration to flash */
						auxMenuStatus->calibration_status = TRUE;
				        break;

					case RADIO_READ_CALIBRATION_COEF:
						
						printCalData(&calDataObtained);
						/*if (auxMenuStatus->calibration_status == FALSE)
							printf ("Module not calibrated.\n");
						else
						{
							for (i=0; i<7; i++)
							{
								printf("Line %i = %i.%i, %i.%i, %i, %i, 0x%8x\n", i,  \
									calData[i].slope.part.integer, calData[i].slope.part.fraction, \
									calData[i].intercept.part.integer, calData[i].intercept.part.fraction, \
									(calData[i].maxIndex & 0x0000FFFF), ((calData[i].maxIndex & 0xFFFF0000) >> 16), \
									calData[i].crc);
							}
						}*/
				        break;

				        case RADIO_SWITCH_POWER_CONTROL_LOOP:
					 	menuRadioTxPeriodicControlLoop(auxMenuStatus);
						PIDController (&calDataObtained, auxMenuStatus->rftxpower,auxMenuStatus->channel, auxMenuStatus->transmit_rate);
						MacRadioTXPeriodic(0, auxMenuStatus->transmit_mode, \
			      					      auxMenuStatus->transmit_rate, \
			      				              1000, \
			      					      auxMenuStatus->frame_period,  \
			      					      auxMenuStatus->frame_length);
						
						/*printf ("The new power control status is: ");
						if (auxMenuStatus->power_control == 0)
						{
							auxMenuStatus->power_control = 1;
							printf ("active.\n");
						}
						else
						{
							auxMenuStatus->power_control = 0;
							printf ("inactive.\n");
						}*/
				        break;
					
					case RADIO_EXIT:
						goto EXIT;
					break;
			
					default:
						printf ("\n Unknown test error\n");
					break;

				}

				auxMenuStatus->statusInformation = RADIO_NOSELECTION;
			}
			else
			{
				printf("Values outside the range, please repeat.\n");
				auxMenuStatus->statusInformation = RADIO_NOSELECTION;
			}
		}
	}
	
	EXIT:
	{
		ShutdownHW();
		return 0;
	}
}


void tst_wifi_nomenu(struct menuStatus *auxMenuStatus)
{
	signed int value;
	unsigned char maxadc;
	enum validateInputs valueType;
	
	char *argv[CFG_MAXARGS + 1];
	/*char finaltoken[CFG_CBSIZE];*/
	char argc;
	char *p = '\0';

		
	while (1)
	{
		printMenu (submenu_Radio_Tx_Cont_Wir_Test, submenu_Radio_Tx_Cont_Wir_Test_size);
		printf(RADIO_TST_PROMPT);

		do
		{
			NOP;
		} while (!tstc());
				
		readline("");
		
		/* Extract arguments */
		if ((argc = parse_line (console_buffer, argv)) == 0) {
			continue;
		}
				
		value = (signed int) simple_strtol (argv[1], &p, 10);
		
		switch (argv[0][0])
		{
			/* Change channel */
			case 'c':
				valueType = CHANNEL;
				if (validateValue (&value, &valueType))
					goto ERROR_SUB_CONT_TX;
				else	
					auxMenuStatus->channel = value;
				//setControlledChannel(auxMenuStatus->channel);
				//SetChannel(auxMenuStatus->channel);
				MacRadioTXPeriodic(0, auxMenuStatus->transmit_mode, 
			      				      auxMenuStatus->transmit_rate, \
			      				      1000, \
			      				      auxMenuStatus->frame_period, \
			      				      auxMenuStatus->frame_length);
				waitUS(100000); // If change channel in same moment, then appeads peaks in CCK/PSK.
				SetChannel(auxMenuStatus->channel);
				//setControlledChannel(auxMenuStatus->channel);
				MacRadioTXPeriodic(1, auxMenuStatus->transmit_mode, 
			      				      auxMenuStatus->transmit_rate, \
			      				      1000, \
			      				      auxMenuStatus->frame_period, \
			      				      auxMenuStatus->frame_length);
				/*#if CONFIG_CCW9P9215
					waitUS(100000); // If change channel in same moment, then appears peaks in CCK/PSK.
				#endif*/
				MacSetDirectTxPower(auxMenuStatus->txpower);
			break;
			/* Change rate */
			case 'm':
				valueType = TRANSMIT_RATE;
				if (validateValue (&value, &valueType))
					goto ERROR_SUB_CONT_TX;
				else
				{
					MacRadioTXPeriodic(0, auxMenuStatus->transmit_mode, 
			      				      auxMenuStatus->transmit_rate, \
			      				      1000, \
			      				      auxMenuStatus->frame_period, \
			      				      auxMenuStatus->frame_length);
					auxMenuStatus->transmit_rate = value;
				}
				/* Check why I need a delay ?*/
				/* TBD */
				waitUS(100000);
				MacRadioTXPeriodic(1, auxMenuStatus->transmit_mode, \
			      			      auxMenuStatus->transmit_rate, \
			      			      1000, \
			      			      auxMenuStatus->frame_period, \
			      			      auxMenuStatus->frame_length);
				//MacUpdateFccBufferX(auxMenuStatus->transmit_mode, auxMenuStatus->transmit_rate);
				break;
			/* Increase power */
			case '+':
				if (auxMenuStatus->txpower == 63)
				{
					printf ("Maximum output power index reached.\n");
					goto ERROR_SUB_CONT_TX;
				}
				else	
					auxMenuStatus->txpower++;
				MacSetDirectTxPower(auxMenuStatus->txpower);
			break;
			/* Decrease power */
			case '-':
				if (auxMenuStatus->txpower == 0)
				{
					printf ("Minimum output power index reached.\n");
					goto ERROR_SUB_CONT_TX;
				}
				else
					auxMenuStatus->txpower--;
				MacSetDirectTxPower(auxMenuStatus->txpower);
			break;
			/* Change power */
			case 'p':
				valueType = TXPOWER;
				if (validateValue (&value, &valueType))
					goto ERROR_SUB_CONT_TX;
				else	
					auxMenuStatus->txpower = value;
				MacSetDirectTxPower(auxMenuStatus->txpower);
			break;
			/* Obtain averaged value from ADC*/
			case 'a':
				reset_max_min_values();
				waitUS(200000);
				read_max_conversion (&maxadc);
				printf("%u\n",maxadc);
				
			break;
			/* Exit subMenu */
			case 'q':
				//MacRadioTXPeriodic(0, auxMenuStatus->transmit_mode, auxMenuStatus->transmit_rate, 100);
				goto EXIT_SUB_CONT_TX;
			break;
			/* Show error, repeat subMenu */
			default:
				printf ("\nWrong value, try again:\n");
				ERROR_SUB_CONT_TX:
			break;
		}
	}
	
	EXIT_SUB_CONT_TX:
	{
	
	}

}



#endif /* CFG_HAS_WIRELESS */
