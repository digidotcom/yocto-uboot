#include <common.h>
 
#if (CONFIG_COMMANDS & CFG_CMD_BSP && \
     defined(CONFIG_UBOOT_CMD_BSP_TESTHW) && \
     defined(CFG_HAS_WIRELESS))

#include "commandswifi.h"
#include "gui_tst_wifi.h"
#include "calibration.h"
#include "vBPiper.h"
#include "adc081C021.h"
#include "AIROHA_7230.h"



int cmdSetContinousTx (long command, struct menuStatus *auxMenuStatus)
{
	cmdExitMode(command, auxMenuStatus);
	
	if (CMDSCTXGETPC(command))
	{
		auxMenuStatus->statusInformation = RADIO_TX_CONTINUOUS_WIRELESS_TEST;
		auxMenuStatus->power_control = 1;
		auxMenuStatus->channel = CMDSCTXGETCHANNEL(command);
		auxMenuStatus->transmit_mode = CMDSCTXGETTYPE(command);
		auxMenuStatus->txpower = 0;
		auxMenuStatus->rftxpower = CMDSCTXGETPOWER(command);
		auxMenuStatus->transmit_rate = CMDSCTXGETRATE(command);
		
		applyWirelessSettingsNoMenu (auxMenuStatus);
		changePrompt(PRMPTTXCONTPC1, auxMenuStatus);
	}
	else
	{
		auxMenuStatus->statusInformation = RADIO_TX_CONTINUOUS_WIRELESS_TEST;
		auxMenuStatus->power_control = 0;
		auxMenuStatus->channel = CMDSCTXGETCHANNEL(command);
		auxMenuStatus->transmit_mode = CMDSCTXGETTYPE(command);
		auxMenuStatus->txpower = CMDSCTXGETPOWER(command);
		auxMenuStatus->rftxpower = 0;
		auxMenuStatus->transmit_rate = CMDSCTXGETRATE(command);

		applyWirelessSettingsNoMenu (auxMenuStatus);
		changePrompt(PRMPTTXCONTPC0, auxMenuStatus);
	}
	return 0;
}

/*****************************************************************************/
int cmdSetPeriodicTx (long command, struct menuStatus *auxMenuStatus)
{
	cmdExitMode(command, auxMenuStatus);
	
	if (CMDSCTXGETPC(command))
	{
		auxMenuStatus->statusInformation = RADIO_TX_PERIODIC_FRAME_TRANSMIT;
		auxMenuStatus->power_control = 1;
		auxMenuStatus->channel = CMDSCTXGETCHANNEL(command);
		auxMenuStatus->transmit_mode = 0;
		auxMenuStatus->txpower = 0;
		auxMenuStatus->rftxpower = CMDSCTXGETPOWER(command);
		auxMenuStatus->frame_length = CMDSCTXGETLENGTH(command);
		auxMenuStatus->frame_period = CMDSCTXGETPERIOD(command);
		auxMenuStatus->transmit_rate = CMDSCTXGETRATE(command);

		applyWirelessSettingsNoMenu (auxMenuStatus);
		changePrompt(PRMPTTXPERIODPC1, auxMenuStatus);
		
	}
	else
	{
		auxMenuStatus->statusInformation = RADIO_TX_PERIODIC_FRAME_TRANSMIT;
		auxMenuStatus->power_control = 0;
		auxMenuStatus->channel = CMDSCTXGETCHANNEL(command);
		auxMenuStatus->transmit_mode = 0;
		auxMenuStatus->txpower = CMDSCTXGETPOWER(command);
		auxMenuStatus->rftxpower = 0;
		auxMenuStatus->frame_length = CMDSCTXGETLENGTH(command);
		auxMenuStatus->frame_period = CMDSCTXGETPERIOD(command);
		auxMenuStatus->transmit_rate = CMDSCTXGETRATE(command);
	
		applyWirelessSettingsNoMenu (auxMenuStatus);
		changePrompt(PRMPTTXPERIODPC0, auxMenuStatus);
		
	}
	return (0);
}

/*****************************************************************************/
int cmdSetRx (long command, struct menuStatus *auxMenuStatus)
{
	cmdExitMode(command, auxMenuStatus);
	auxMenuStatus->statusInformation = RADIO_RX_SILENT_WIRELESS_TEST;
	auxMenuStatus->channel = CMDSRXGETCHANNEL(command);
	applyWirelessSettingsNoMenu (auxMenuStatus);
	changePrompt(PRMPTRX, auxMenuStatus);
	return (0);
}

/*****************************************************************************/
/*#define CMDCPGETPOWER(x)	(x & 0x0000000F)
#define CMDCPGETINCRTYPE(x)	((x >> 8) & 0x00000001)
#define CMDCPGETINCRDIR(x)	((x >> 9) & 0x00000001)*/

int cmdChangePower (long command, struct menuStatus *auxMenuStatus)
{
	if (auxMenuStatus->statusInformation == RADIO_TX_CONTINUOUS_WIRELESS_TEST || \
	    auxMenuStatus->statusInformation == RADIO_TX_PERIODIC_FRAME_TRANSMIT)
	{
		if (!CMDCPGETINCRTYPE(command))
		{
			auxMenuStatus->txpower = CMDCPGETPOWER(command);
			MacSetDirectTxPower(auxMenuStatus->txpower);
			////printMessage(COMMANDEXECUTED);
			return (0);
		}
		else
		{
			if (CMDCPGETINCRDIR(command))
			{
				auxMenuStatus->txpower += CMDCPGETPOWER(command);
				MacSetDirectTxPower(auxMenuStatus->txpower);
				//printMessage(COMMANDEXECUTED);
				return (0);
			}
			else
			{
				auxMenuStatus->txpower -= CMDCPGETPOWER(command);
				MacSetDirectTxPower(auxMenuStatus->txpower);
				//printMessage(COMMANDEXECUTED);
				return (0);
			}
		}
	}
	printError(NOMODESELECTED);
	return (1);
}

/*****************************************************************************/
int cmdChangeChannel (long command, struct menuStatus *auxMenuStatus)
{
	if (auxMenuStatus->statusInformation == RADIO_TX_CONTINUOUS_WIRELESS_TEST || \
	    auxMenuStatus->statusInformation == RADIO_TX_PERIODIC_FRAME_TRANSMIT)
	{
		auxMenuStatus->channel = CMDCCCHANNEL(command);
		applyWirelessSettingsNoMenu (auxMenuStatus);
		//printMessage(COMMANDEXECUTED);
		return (0);
	}
	else if (auxMenuStatus->statusInformation == RADIO_RX_SILENT_WIRELESS_TEST)
	{
		auxMenuStatus->channel = CMDCCCHANNEL(command);
		cmdExitMode(command, auxMenuStatus);
		SetChannel(auxMenuStatus->channel);
		auxMenuStatus->statusInformation = RADIO_RX_SILENT_WIRELESS_TEST;
		MacRadioRXTest(1);
		rxBeforeLoop(&(auxMenuStatus->lastNumRxFrames));
		changePrompt(PRMPTRX, auxMenuStatus);
		return (0);
	}
	printError(NOMODESELECTED);
	return (1);
}

/*****************************************************************************/
int cmdChangeRate (long command, struct menuStatus *auxMenuStatus)
{
	if (auxMenuStatus->statusInformation == RADIO_TX_CONTINUOUS_WIRELESS_TEST || \
	    auxMenuStatus->statusInformation == RADIO_TX_PERIODIC_FRAME_TRANSMIT)
	{
		auxMenuStatus->transmit_rate = CMDCRRATE(command);
		applyWirelessSettingsNoMenu(auxMenuStatus);
		//printMessage(COMMANDEXECUTED);
		return (0);
	}
	printError(NOMODESELECTED);
	return (1);
}

/*****************************************************************************/
int cmdChangeFrameLength (long command, struct menuStatus *auxMenuStatus)
{
	if (auxMenuStatus->statusInformation == RADIO_TX_PERIODIC_FRAME_TRANSMIT)
	{
		auxMenuStatus->frame_length = CMDCFLLENGTH(command);
		applyWirelessSettingsNoMenu (auxMenuStatus);
		//printMessage(COMMANDEXECUTED);
		return (0);
	}
	printError(NOMODESELECTED);
	return (1);
}

/*****************************************************************************/
int cmdChangeFramePeriod (long command, struct menuStatus *auxMenuStatus)
{
	if (auxMenuStatus->statusInformation == RADIO_TX_PERIODIC_FRAME_TRANSMIT)
	{
		auxMenuStatus->frame_period = CMDCFPPERIOD(command);
		applyWirelessSettingsNoMenu (auxMenuStatus);
		//printMessage(COMMANDEXECUTED);
		return (0);
	}
	printError(NOMODESELECTED);
	return (1);
}

/*****************************************************************************/
int cmdReadADCValues (long command, struct menuStatus *auxMenuStatus)
{
	/* For measuring ADC value */
	unsigned char adcValue;

	if (auxMenuStatus->statusInformation == RADIO_TX_CONTINUOUS_WIRELESS_TEST || \
	    auxMenuStatus->statusInformation == RADIO_TX_PERIODIC_FRAME_TRANSMIT)
	{

		reset_max_min_values();
		waitUS(auxMenuStatus->frame_period * 2 * 1000);
		
		switch (CMDRAVGETMEAS(command))
		{
			case MAXADC:
				read_max_conversion (&adcValue);
			break;
		
			case MINADC:
				//read_max_min_avg_conversion (0, &adcValue);
			break;

			case AVGADC:
				//read_max_min_avg_conversion ((command & 0xFF), &adcValue);
			break;
		}
				
			/* Check desired numerical format */
		if (!CMDRAVGETFORMAT(command))
			printf("ADC: %u\r\n",adcValue);
		else
			printf("ADC: 0x%02x\r\n",adcValue);
		
		return (0);
	}
	printError(NOMODESELECTED);
	return (1);
}

/*****************************************************************************/

int cmdReadResetRxFrames (long command, struct menuStatus *auxMenuStatus)
{
	if (RESETCOUNTER(command))
		auxMenuStatus->lastNumRxFrames = 0;
	
	if (CMDRRFGETFORMAT(command))
		printf("RXd: %u\r\n",auxMenuStatus->lastNumRxFrames);
	else
		printf("RXf: 0x%08x\r\n",auxMenuStatus->lastNumRxFrames);

	
	return (0);
}	


/*****************************************************************************/
int cmdExitTestHWWireless (long command, struct menuStatus *auxMenuStatus)
{
	cmdExitMode(command, auxMenuStatus);
	printMessage(PRMPTEXITING);
	return (-1);
}

/*****************************************************************************/
int cmdExitMode (long command, struct menuStatus *auxMenuStatus)
{
	if (auxMenuStatus->statusInformation == RADIO_TX_CONTINUOUS_WIRELESS_TEST)
	{
		PRINTDEBUG("DEBUG: TXCONTOFF\r\n");
		MacContinuousTransmit(0, \
			auxMenuStatus->transmit_mode, \
			auxMenuStatus->transmit_rate);
		waitUS(100000);
	}
	else if (auxMenuStatus->statusInformation == RADIO_TX_PERIODIC_FRAME_TRANSMIT || \
		 auxMenuStatus->statusInformation == RADIO_START_TX_PERIODIC_FRAME_TRANSMIT || \
		 auxMenuStatus->statusInformation == RADIO_SWITCH_POWER_CONTROL_LOOP)
	{
		MacRadioTXPeriodic(0, \
			auxMenuStatus->transmit_mode, 
			auxMenuStatus->transmit_rate, \
			1000, \
			auxMenuStatus->frame_period, \
			auxMenuStatus->frame_length);
		waitUS(100000);
	}
	else if (auxMenuStatus->statusInformation == RADIO_RX_SILENT_WIRELESS_TEST)
	{
		rxAfterLoop();
		MacRadioRXTest(0);
	}

	auxMenuStatus->statusInformation = RADIO_NOSELECTION;
	auxMenuStatus->power_control = 0;
	auxMenuStatus->channel = 1;
	auxMenuStatus->transmit_mode = 0;
	auxMenuStatus->txpower = 0;
	auxMenuStatus->rftxpower = 0;
	auxMenuStatus->transmit_rate = 0;
	auxMenuStatus->frame_period = 0;
	auxMenuStatus->frame_length = 0;
	auxMenuStatus->lastNumRxFrames = 0;
	
	changePrompt(STANDBYNOMODE, auxMenuStatus);
	return 0;
}

/*****************************************************************************/
int cmdReadCalibration (long command, struct menuStatus *auxMenuStatus)
{
	return 0;
}

/*****************************************************************************/
int cmdSetCalibration (long command, struct menuStatus *auxMenuStatus)
{
	printf("Use the following command to load calibration data\r\n");
	printf("via tftp:\r\n\r\n");
	printf("update wifical tftp <filename>\r\n");
	return 0;
}

/*****************************************************************************/
int cmdGetStatus (long command, struct menuStatus *auxMenuStatus)
{
	return 0;
}

/*****************************************************************************/
int mainRoutineTestHWWirelessNoMenu(struct menuStatus *auxMenuStatus)
{
	int (*menuFunctions[]) (long , struct menuStatus*) = 
	{
		&cmdSetContinousTx,
		&cmdSetPeriodicTx,
		NULL, NULL,
		&cmdSetRx,
		NULL, NULL, NULL,
		&cmdChangePower,
		&cmdChangeChannel,
		&cmdChangeRate,
		&cmdChangeFrameLength,
		&cmdChangeFramePeriod,
		&cmdReadADCValues,
		&cmdReadResetRxFrames,
		&cmdExitMode,
		&cmdReadCalibration,
		&cmdSetCalibration,
		NULL, NULL, NULL, NULL,
		NULL, NULL, NULL, NULL,
		NULL, NULL, NULL, NULL,
		&cmdGetStatus,
		&cmdExitTestHWWireless
	};

	unsigned long command;
	int returned;
	char *argv[CFG_MAXARGS + 1];
	char argc;
	
	init_adc_pc();
	
	SetBSS (0, (unsigned char *) "TESTING123", 10, 0xFFFF, 10);
	changePrompt(STANDBYNOMODE, auxMenuStatus);
	refreshPrompt(auxMenuStatus);

	while (1)
	{
		do
		{

			if (auxMenuStatus->statusInformation == RADIO_RX_SILENT_WIRELESS_TEST)
			{
				rxLoopExecution(&(auxMenuStatus->lastNumRxFrames));
				//printf("Waiting: %u", &(auxMenuStatus->lastNumRxFrames));
			}
			else if (auxMenuStatus->power_control)
			{
				//control loop here
			}

		} while (!tstc());
				
		readline("");
		
		/* Extract arguments */
		if ((argc = parse_line (console_buffer, argv)) == 0)
		{
			continue;
		}
		else
		{		
			command = (unsigned long) simple_strtol (argv[0], NULL, 10);
			returned = menuFunctions[GETCOMMAND(command)] (command, auxMenuStatus);
			
			if (returned == -1)
				goto EXIT;
			else
				refreshPrompt(auxMenuStatus);
		}
	}
	
	EXIT:
	{
		ShutdownHW();
	}
	
	return (0);
}


/*****************************************************************************/
int applyWirelessSettingsNoMenu (struct menuStatus *auxMenuStatus)
{
	

	if (auxMenuStatus->statusInformation == RADIO_TX_CONTINUOUS_WIRELESS_TEST ||
	    auxMenuStatus->statusInformation == RADIO_START_CONTINUOUS_TRANSMIT ||
	    auxMenuStatus->statusInformation == RADIO_SWITCH_POWER_CONTROL_LOOP )
	{
		MacContinuousTransmit(0, auxMenuStatus->transmit_mode, auxMenuStatus->transmit_rate);
		MacSetDirectTxPower(auxMenuStatus->txpower);
		waitUS(100000);
		SetChannel(auxMenuStatus->channel);
		MacContinuousTransmit(1, auxMenuStatus->transmit_mode, auxMenuStatus->transmit_rate);
		MacSetDirectTxPower(auxMenuStatus->txpower);	
		return 0;

	}
	else if (auxMenuStatus->statusInformation == RADIO_TX_PERIODIC_FRAME_TRANSMIT ||
		 auxMenuStatus->statusInformation == RADIO_START_TX_PERIODIC_FRAME_TRANSMIT ||
		 auxMenuStatus->statusInformation == RADIO_SWITCH_POWER_CONTROL_LOOP)
	{
		MacRadioTXPeriodic(0, \
			auxMenuStatus->transmit_mode, 
			auxMenuStatus->transmit_rate, \
			1000, \
			auxMenuStatus->frame_period, \
			auxMenuStatus->frame_length);
		MacSetDirectTxPower(auxMenuStatus->txpower);
		waitUS(100000);
		SetChannel(auxMenuStatus->channel);
		MacRadioTXPeriodic(1, \
				auxMenuStatus->transmit_mode, \
				auxMenuStatus->transmit_rate, \
				1000, \
				auxMenuStatus->frame_period, \
				auxMenuStatus->frame_length);
		MacSetDirectTxPower(auxMenuStatus->txpower);
		return 0;
	}

	else if (auxMenuStatus->statusInformation == RADIO_RX_SILENT_WIRELESS_TEST)
	{
		SetChannel(auxMenuStatus->channel);
		MacRadioRXTest(1);
		rxBeforeLoop(&(auxMenuStatus->lastNumRxFrames));
	}

	return 1;
}

/*****************************************************************************/
int changePrompt(char *newPrompt, struct menuStatus *auxMenuStatus)
{
	int aux;

	aux = (int) strcpy(auxMenuStatus->prompt, newPrompt);
	/* printf("%s ", auxMenuStatus->prompt); */
	
	return aux;
}

/*****************************************************************************/
void refreshPrompt(struct menuStatus *auxMenuStatus)
{
	printf("%s ", auxMenuStatus->prompt);
}

/*****************************************************************************/
void printMessage (char *message)
{
	printf("%s", message);
}

/*****************************************************************************/
void printError(char *errorToPrint)
{
	printf("%s", errorToPrint);
}

#endif
