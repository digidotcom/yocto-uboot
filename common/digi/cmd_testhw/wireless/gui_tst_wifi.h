#include <common.h>

#if (CONFIG_COMMANDS & CFG_CMD_BSP && \
     defined(CONFIG_UBOOT_CMD_BSP_TESTHW) && \
     defined(CFG_HAS_WIRELESS))

#ifndef _GUI_TST_WIFI_H
#define _GUI_TST_WIFI_H

/* imports from common/main.c */
extern char console_buffer[CFG_CBSIZE];

/* ARM specific */
#define NOP asm("MOV r0, r0")

/* enum used to list all tests that can be conducted using the application. */
enum radio_test_t
{
	RADIO_NOSELECTION = 0,
	RADIO_TX_CONTINUOUS_WIRELESS_TEST = 1,
	RADIO_START_CONTINUOUS_TRANSMIT,
	RADIO_STOP_CONTINUOUS_TRANSMIT,
	RADIO_RX_SILENT_WIRELESS_TEST,
	RADIO_TX_PERIODIC_FRAME_TRANSMIT,
	RADIO_WRITE_RF,
	RADIO_START_TX_PERIODIC_FRAME_TRANSMIT,
	RADIO_STOP_TX_PERIODIC_FRAME_TRANSMIT,
	RADIO_READ_MAC_ADDRESS,
	RADIO_READ_VERSION_REVISION,
	RADIO_SET_CALIBRATION_COEF,
	RADIO_READ_CALIBRATION_COEF,
	RADIO_SWITCH_POWER_CONTROL_LOOP,
	RADIO_EXIT
};

/* enum used to list all calibration related actions */
enum radio_calibrate_t
{
    RADIO_CALIBRATE_TRANSMITTER,
    RADIO_CHARACTERIZE_TRANSMITTER,
    RADIO_CORRELATE_ADC_READINGS_WITH_VOLTMETER_READINGS,
    RADIO_CALIBRATE_RECEIVER
};

/* Used for user values input validation */
enum validateInputs {TXPOWER, TRANSMIT_MODE, TRANSMIT_RATE, CHANNEL, FRAME_LENGTH, FRAME_PERIOD, RFTXPOWER};


#define RANDOM		0
#define ZEROS		1
#define ONES		2
#define UNMODULATED 	3
#define FCC		4
static const unsigned char nTs_transmit_mode[][12] =
{
	"RANDOM\0",
	"ZEROS\0", 
	"ONES\0", 
	"UNMODULATED\0", 
	"FCC\0"
};


#define IS_CCK_DSSS(r)	(r >= 0 && r < 4)
#define IS_OFDM(r)	(r > 4 && r < 12)

static const unsigned char nTs_transmit_rate[][8] =
{
	"1Mbps\0", "2Mbps\0", "5.5Mbps\0", "11Mbps\0", "6Mbps\0", "9Mbps\0",
	"12Mbps\0", "18Mbps\0", "24Mbps\0", "36Mbps\0", "48Mbps\0", "54Mbps\0"
};

#define SIZE1024BYTES	0
#define SIZE128BYTES	1
static const unsigned char nTs_frame_length[][11] =
{
	"1024 bytes\0",
	"128 bytes\0"
};


// DELETE IF NOT ERROR:  #define CTRL_C_CHAR 0x03
// DELETE IF NOT ERROR:  #define RADIO_TST_PROMPT ">> "

/* Used where we get input from user. */
// DELETE IF NOT ERROR: #define FCC_INPUT_BUFFER_SIZE 64


struct menuStatus 
{
	enum radio_test_t statusInformation;
	unsigned char power_control;
	unsigned char calibration_status;
	unsigned char channel;
	unsigned char transmit_mode;
	unsigned char txpower;
	unsigned int  rftxpower;
	unsigned char transmit_rate;
	unsigned char frame_period;
	unsigned char frame_length;
	unsigned int  lastNumRxFrames;
	char 	      prompt[12];
//	struct wlanCalibrationData calData;
};


/*typedef struct lineDefinition
{
	FIXED16_8 	slope;
   	FIXED16_8 	intercept;
   	unsigned long 	maxIndex;
   	unsigned long	crc;
} lineDefinition;
static lineDefinition calData[7];*/


#define RADIO_TST_PROMPT 	"TST_WIFI>>"


static const unsigned char menu_principal_header [][80]
					= { 
						"\n\0",
						"**********************************\n\0",
						"* Test 802.11 radio transceiver: *\n\0",
						"**********************************\n\0",
						"\n\0"
					  };
					  
static const unsigned char menu_principal_header_size = 4;

static const unsigned char menu_principal_body [][80] 
					= {
	            				" 1) Wireless Continuous Transmit Test\n\0",
            					" 2) Start Wireless Continuous Transmit Test\n\0",
            					" 3) Stop Wireless Continuous Transmit Test\n\0",
            					" 4) Wireless Silent Test\n\0",
            					" 5) Wireless Transmit Periodic frame Test\n\0",
            					" 6) Write to RF Transceiver\n\0",
            					" 7) Start Wireless Periodic Transmit Test\n\0",
            					" 8) Stop Wireless Periodic Transmit Test\n\0",
            					" 9) Read Radio Mac Address\n\0",
            					"10) Read Radio Version-Revision\n\0",
            					"11) Set Calibration Coefficients\n\0",
						"12) Read Calibration Coefficients\n\0",
            					"13) Switch Control Loop Status\n\0",
						"14) Quit\n\0"
					   }; 
					   
static const unsigned char menu_principal_body_size = 14;

static const unsigned char menu_Request_TX_Power [][80]
					= {
						"Enter wireless TX Power (from [Min = 0] up to [Max = 63])\n\0",
					  };
					
static const unsigned char menu_Request_TX_Power_size = 1;

static const unsigned char menu_Request_RF_TX_Power [][80]
					= {
						"Enter wireless TX Power (from Min = 0, (0 dBm) up to Max = 1600 (for 16 dBm))\n\0",
					  };
					
static const unsigned char menu_Request_RF_TX_Power_size = 1;
					
static const unsigned char menu_Request_TX_Mode [][80]
					= {
						"Enter wireless transmit mode --> \n\0",
						"     0=random, 1=zeros, 2=ones, 3=unmodulated, 4=fcc\n\0"
					};
					
static const unsigned char menu_Request_TX_Mode_size = 2;
					
static const unsigned char menu_Request_TX_Rate [][80]
					= {
					            				
            					"Enter wireless transmit rate -->\n\0",
            					"      0 =  1Mbps,  1 =  2Mbps, 2 = 5.5Mbps, 3 = 11Mbps, 4 =  6Mbps\n\0",
            					"      5 =  9Mbps,  6 = 12Mbps, 7 =  18Mbps, 8 = 24Mbps, 9 = 36Mbps\n\0",
            					"     10 = 48Mbps, 11 = 54Mbps\n\0"
					};
					
static const unsigned char menu_Request_TX_Rate_size = 4;
					
static const unsigned char menu_Request_TX_Channel [][80]
					= {
            					"Enter wireless channel to transmit/receive on (1 - 49)\n\0",
					  };
					  
static const unsigned char menu_Request_TX_Channel_size = 1;

static const unsigned char menu_Request_TX_Frame_Period [][80]
					= {
            					"Enter TX frame period (0 - 100 msecs)\n\0"
					  };
					  
static const unsigned char menu_Request_TX_Frame_Period_size = 1;

static const unsigned char menu_Request_TX_Frame_Length [][80]
					= {
            					"Enter TX frame length -->\n\0",
						"     0 = 1024 bytes, 1 = 128 bytes\n\0"
					  };
					  
static const unsigned char menu_Request_TX_Frame_Length_size = 2;
					  
static const unsigned char submenu_Radio_Tx_Cont_Wir_Test [][80]
					= {
						"Type 'c' to change channel, followed by the new channel value\n\0",
						"Type 'm' to change transmit rate, followed by the new rate value\n\0", 
            					"Type (+/-) to increase/decrease power\n\0",
            					"Type 'p' to change power, followed by the new power value\n\0",
            					"Type q to quit\n\0"
					  };
					  
static const unsigned char submenu_Radio_Tx_Cont_Wir_Test_size = 5;

void printMenu (const unsigned char message[][80], const unsigned char num_of_lines);

int parse_line (char *line, char *argv[]);
int getSLongConsole(long *returned);
/*int getSFP8_8ConsoleNOREAD (char *auxConBuff, FIXED16_8 *returned, unsigned char decimalSeparator, char **p);*/
int getSLongConsoleNOREAD(char *auxConBuff, long *returned, char **p);
/*int retrieveLineConsole (lineDefinition *auxLine, unsigned char decimalSeparator, \
			    unsigned char valuesSeparator, unsigned char endProcessSeparator, \
			    unsigned char lineNumber);
*/

unsigned int validateValue(signed int *toValidate, enum validateInputs *inputType);
unsigned char setControlledTxPower (struct menuStatus *auxMenuStatus);
unsigned char setControlledChannel (struct menuStatus *auxMenuStatus);
unsigned char applyWirelessSettings (struct menuStatus *auxMenuStatus);


int menuRadioTxContinuousWirelessTest(struct menuStatus *auxMenuStatus);
void subMenuRadioTxContinuousWirelessTest(struct menuStatus *auxMenuStatus);
int menuRadioTxPeriodicWirelessTest(struct menuStatus *auxMenuStatus);
void subMenuRadioTxPeriodicWirelessTest(struct menuStatus *auxMenuStatus);
void menuRadioSilentRX(struct menuStatus *auxMenuStatus);
int menuRadioTxPeriodicControlLoop(struct menuStatus *auxMenuStatus);

int tst_wifi (struct menuStatus *auxMenuStatus);


#endif

#endif /* CFG_HAS_WIRELESS */
