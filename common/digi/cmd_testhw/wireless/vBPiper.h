#include <common.h>

#if (CONFIG_COMMANDS & CFG_CMD_BSP && \
     defined(CONFIG_UBOOT_CMD_BSP_TESTHW) && \
     defined(CFG_HAS_WIRELESS))

//
// Embedded 802.11a/g wireless network interface
// MAC layer hardware interface for ConnectCorewi9p9215 and Connectwiem9210
// Copyright 2008 Digi International
//

#ifndef _PIPER_H
#define _PIPER_H

#define IBSSOFF 1

typedef enum {FALSE, TRUE} boolean;

typedef unsigned int * __p32bits;
typedef unsigned short * __p16bits;

void __write32 (volatile unsigned int *addr, unsigned int  data, unsigned char operation);
void __read32  (volatile unsigned int *addr, unsigned int *data);
void waitUS (unsigned long usec);
void piperReset(void);

#define uint32 		unsigned int
#define uint16 		unsigned short
#define uint8  		unsigned char

#define WRITE 0
#define AND 1
#define OR 2

#define SIZE1024BYTES	0
#define SIZE128BYTES	1

#define	RATE_MASK_PSK		0x0003
#define	RATE_MASK_CCK		0x000c
#define	RATE_MASK_OFDM		0x0ff0

#define	RATE_MASK_BASIC		0x0153	// Ad hoc basic rates

/******************************************************************************/
/* HARDWARE REGISTER DEFINES **************************************************/
/******************************************************************************/
#if CONFIG_CCW9M2443
	#define MAC_BASE	0x20000000		// Register base address
#elif CONFIG_CCW9P9215
	#define MAC_BASE	0x70000000		// Register base address
#endif

#define MAC_CTRL_BASE   (  (volatile uint32 *) (MAC_BASE + (0x40)) )
#define	REG32(offset)	(  (volatile uint32 *) (MAC_BASE + (offset)) )

#define MAC_MASK		0xffffc001	// Size mask and enable bit

/* BASEBAND CONTROL REGISTERS *************************************************/
#define	HW_VERSION		REG32(0x00)	// Version
#define	HW_GEN_CONTROL		REG32(0x04)	// General control
#define	HW_GEN_STATUS		REG32(0x08)	// General status
#define	HW_RSSI_AES		REG32(0x0c)	// RSSI and AES status
#define	HW_INTR_MASK		REG32(0x10)	// Interrupt mask
#define	HW_INTR_STATUS		REG32(0x14)	// Interrupt status
#define	HW_SPI			REG32(0x18)	// RF SPI interface
#define HW_SPI_CTRL     	REG32(0x1C)	// RF SPI control
#define HW_SPI_CONTROL  	REG32(0x1C)     // SPI control baseband register
#define	HW_DATA_FIFO		REG32(0x20)	// Data FIFO

//CC CAT rev2
#define	HW_TRACK_CONTROL 	REG32(0x28)	// frequency-band specific tracking constant
//CC CAT rev2

#define HW_CONF1        	REG32(0x28)	// Configuration 1
#define HW_CONF2        	REG32(0x2C)	// Configuration 2
#define	HW_AES_FIFO		REG32(0x30)	// AES FIFO
#define	HW_AES_MODE		REG32(0x38)	// ARS mode
#define HW_OUT_CTRL     	REG32(0x3C)     //Output control

/* MAC CONTROL REGISTERS ******************************************************/
#define	HW_STAID0		REG32(0x40)	// Station ID (6 bytes)
#define	HW_STAID1		REG32(0x44)
#define	HW_BSSID0		REG32(0x48)	// BSS ID (6 bytes)
#define	HW_BSSID1		REG32(0x4c)
#define	HW_SSID_LEN		REG32(0x50)	// Basic rates (high 16 bits), SSID length (low 8 bits)
#define	HW_BACKOFF		REG32(0x54)	// Backoff period (16 bits)
#define	HW_LISTEN		REG32(0x58)	// Listen interval (16 bits), CFP (8 bits), DTIM (8 bits)
#define	HW_CFP_ATIM		REG32(0x5c)	// CFP max duration/ATIM period (16 bits), beacon interval (16 bits)
#define	HW_MAC_STATUS		REG32(0x60)	// MAC status (8 bits)
#define	HW_MAC_CONTROL		REG32(0x64)	// MAC control (8 bits)
#define	HW_REMAIN_BO		REG32(0x68)	// Remaining backoff (16 bits)
#define	HW_BEACON_BO		REG32(0x6c)	// Beacon backoff (16 bits), beacon mask (8 bits)
#define	HW_SSID			REG32(0x80)	// Service set ID (32 bytes)
#define	HW_STA2ID0		REG32(0xb0)	// Second Station ID (6 bytes)
#define	HW_STA2ID1		REG32(0xb4)
#define	HW_STA3ID0		REG32(0xb8)	// Third Station ID (6 bytes)
#define	HW_STA3ID1		REG32(0xbc)

/* FIFO SIZES IN BYTES ********************************************************/
#define	HW_TX_FIFO_SIZE		1792
#define	HW_RX_FIFO_SIZE		2048

#if (MAX_FRAME_SIZE > HW_TX_FIFO_SIZE)
	#error	MAX_FRAME_SIZE too big
#endif

// Min fragment size to allow interrupt latency between frames
#define	MIN_FRAG_SIZE		400

// Size of CTS frame in FIFO
#define	CTS_OVERHEAD		20

// Max overhead per fragment (CCMP is worst case)
#define	FRAG_OVERHEAD		(8+DATA_SIZE+CCMP_SIZE-FCS_SIZE)

// Max fragment size to fit CTS and 2 fragments in transmit FIFO
#define	MAX_FRAG_SIZE		(DATA_SIZE + (HW_TX_FIFO_SIZE-CTS_OVERHEAD-2*FRAG_OVERHEAD)/2)

#define DSP_LOAD_ENABLE         0x00000400          //1 << 10
#define MACASSIST_LOAD_ENABLE   0x00000200          //1 << 9
#define DSP_MACASSIST_ENABLE    0x00000800          //1 << 11

/* GENERAL CONTROL REGISTER BITS **********************************************/
#define	GEN_RXEN		0x00000001	// Receive enable
#define	GEN_ANTDIV		0x00000002	// Antenna diversity
#define	GEN_ANTSEL		0x00000004	// Antenna select
#define	GEN_5GEN		0x00000008	// 5 GHz band enable
#define	GEN_SHPRE		0x00000010	// Transmit short preamble
#define	GEN_RXFIFORST		0x00000020	// Receive FIFO reset
#define	GEN_TXFIFORST		0x00000040	// Transmit FIFO reset
#define	GEN_TXHOLD		0x00000080	// Transmit FIFO hold
#define	GEN_BEACEN		0x00000100	// Beacon enable
#define	GEN_TXFIFOEMPTY		0x00004000	// Transmit FIFO empty
#define	GEN_TXFIFOFULL		0x00008000	// Transmit FIFO full


#define	GEN_TESTMODE		0x80000000	// Test mode of operation 
#define	GEN_PA_ON		0x02000000	// 0 = 5.0Ghz PA; 1 = 2.4Ghz PA (Airoha AL7230)

#define CONT_TX 		0x00100000	//Piper continuous transmit, in General status Register
#define TX_CTL  		0x00006000	//Piper, tansmit control value, 11 DAC data is positive DC

//CC CAT rev2
#define TRACK_BG_BAND           0x00430000     // Tracking constant for 802.11 b/g frequency band
#define TRACK_4920_4980_A_BAND  0x00210000     // Tracking constant for 802.11 a sub-frequency band
#define TRACK_5150_5350_A_BAND  0x001F0000     // Tracking constant for 802.11 a sub-frequency band
#define TRACK_5470_5725_A_BAND  0x001D0000     // Tracking constant for 802.11 a sub-frequency band
#define TRACK_5725_5825_A_BAND  0x001C0000     // Tracking constant for 802.11 a sub-frequency band
//CC CAT rev2

/* GENERAL STATUS REGISTER BITS ***********************************************/
#define	STAT_RXFE		0x00000010	// Receive FIFO empty
#define STAT_DLLLOCK   		0x00000020      // DLL lock

/* AES STATUS REGISTER BITS ***************************************************/
#define	AES_EMPTY		0x00010000	// AES receive FIFO empty
#define	AES_FULL		0x00020000	// AES transmit FIFO full
#define	AES_BUSY		0x00040000	// AES engine busy
#define	AES_MIC			0x00080000	// AES MIC correct

/* INTERRUPT MASK AND STATUS REGISTER BITS ************************************/
#define	INTR_RXFIFO		0x00000001	// Receive FIFO not empty
#define	INTR_TXEND		0x00000002	// Transmit complete
#define	INTR_TIMEOUT		0x00000004	// CTS/ACK receive timeout
#define	INTR_ABORT		0x00000008	// CTS transmit abort
#define	INTR_TBTT		0x00000010	// Beacon transmission time
#define	INTR_ATIM		0x00000020	// ATIM interval end
#define	INTR_RXOVERRUN		0x00000040	// Receive FIFO overrun

/* MAC CONTROL REGISTER BITS **************************************************/
#define	CTRL_TXREQ		0x00000001	// Transmit request
#define	CTRL_AUTOTXDIS		0x00000002	// Auto-transmit disable
#define	CTRL_BEACONTX		0x00000004	// Beacon transmit enable
#define	CTRL_PROMISC		0x00000008	// Promiscuous mode
#define	CTRL_IBSS		0x00000010	// IBBS mode
#define	CTRL_MAC_FLTR		0x00000040	// Extra address filter mode

/* POWERSAVE REGISTER INDEX ***************************************************/
#define	INDX_GEN_CONTROL	0	// General control
#define	INDX_GEN_STATUS		1	// General status
#define	INDX_RSSI_AES		2	// RSSI and AES status
#define	INDX_INTR_MASK	    	3	// Interrupt mask
#define INDX_SPI_CTRL       	4	// RF SPI control
#define INDX_CONF1          	5	// Configuration 1
#define INDX_CONF2          	6	// Configuration 2
#define	INDX_AES_MODE		7	// ARS mode
#define INDX_OUT_CTRL       	8       // Output control
#define INDX_MAC_CONTROL    	9       // MAC control
#define INDX_TOTAL          	10

/* SOFTWARE TIMING REGISTERS **************************************************/


//#ifdef FCC_ENABLED
	#define	INTR_RXEND	0x00000001	//  Receive FIFO not empty
	#define HW_SSI_ADDR   	REG32(0x10)
	#define SSI_ADDR_WR   	REG32(0x14)
	#define HW_SSI_DATA0   	REG32(0x18)
	#define HW_SSI_DATA1   	REG32(0x18)
	#define HW_SSI_DATA2   	REG32(0x18)
	#define	REG16(offset)	(* (volatile uint16 *) (MAC_BASE + (offset)))  
	#define REG8(offset)	(* (volatile uint8  *) (MAC_BASE + (offset) + 1)) 

//	int HW_TEST_MODE = 0;
//	int HW_TX_BUFFER[2000];
//#endif

//#if BSP_RF_TESTING
//	#define	REG16(offset)	(* (volatile uint16 *) (MAC_BASE + (offset)))  
	
//	static wln_bss bss_save = WLN_BSS_NONE;

	// This buffer is used as t//#if BSP_RF_TESTING
//	#define	REG16(offset)	(* (volatile uint16 *) (MAC_BASE + (offset)))  
	
//	static wln_bss bss_save = WLN_BSS_NONE;

	// This buffer is used as the transmit buffer for testing the
	// continuous transmit FCC test.
//	static unsigned char fcc_txbuffer[64];
/*	unsigned char fcc_txbuffer2[36]={0x00,0x09,0x00,0x00,0x0a,0x04,0x00,0x01, 
		                         0x08,0x00,0x00,0x00,0x00,0x22,0x33,0x44,
		                         0x55,0x66,0x10,0x20,0x30,0x40,0x50,0x60,
		                         0x10,0x20,0x30,0x40,0x50,0x60,0x00,0x21) Set powersave time
		                         00,
		                         0x05,0x55,0x55,0x00};
	
	static unsigned char radio_txbuffer[1024];*/
//#endifhe transmit buffer for testing the
	// continuous transmit FCC test.
//	static unsigned char fcc_txbuffer[64];
/*	unsigned char fcc_txbuffer2[36]={0x00,0x09,0x00,0x00,0x0a,0x04,0x00,0x01, 
		                         0x08,0x00,0x00,0x00,0x00,0x22,0x33,0x44,
		                         0x55,0x66,0x10,0x20,0x30,0x40,0x50,0x60,
		                         0x10,0x20,0x30,0x40,0x50,0x60,0x00,0x00,
		                         0x05,0x55,0x55,0x00};
	
	static unsigned char radio_txbuffer[1024];*/
//#endif

//
// PIO pins
//
#define PIN_LED			PIN_INIT     		// Link status LED
#define INTR_ID			EXTERNAL0_INTERRUPT	//external interupt 0, active high
#define STATUS_A0		(((*(unsigned long *)(0xA0902088)) & 0x100) >> 8)



/******************************************************************************/
/* FUNCTIONS PROTOTYPES *******************************************************/
/******************************************************************************/

/* This funtions perform and initialization of the NS9215 CPU to a known state 
   after the initialization with U-Boot */
void initializeuPToKnowState (void);


/*
 Write data to an RF tranceiver register
 @param addr Register address (4 bits)
 @param data Data to write (20 bits, bit reversed)
*/
void WriteRF (uint8 addr, uint32 data);

//
// Load the baseband controller firmware
//
int LoadHW (void);

void InitializeRF(unsigned char band_selection);

//
// Initialize the wireless hardware
//
int InitHW (void);

//
// Shutdown the wireless hardware
//
void ShutdownHW (void);

//
// Select a channel
// @param channel Channel number: 1-22
//
//void SetChannel (void);
void SetChannel (unsigned char channel);
void MacRadioTXPeriodic (unsigned char active, unsigned int transmit_mode, unsigned int transmit_rate, int atim, int beacon, unsigned char frame_length);

void SetPromiscuousMode(boolean enable);
void MacSetQuiet(int enable_quiet);

void SetBSS (int bssCaps, uint8 *ssid, int ssid_len, uint16 basic, int atim);
void MacSetDirectTxPower(int value);
void MacRadioRXTest(int enable);

inline void rxBeforeLoop(unsigned int *rxFrames);
inline void rxLoopExecution(unsigned int *rxFrames);
inline void rxAfterLoop(void);
inline void MacReadPacketFromBuffer(unsigned int *rxFrames);
void MacContinuousTransmit(int enable, unsigned int transmit_mode, unsigned int transmit_rate);

#endif
#endif /* CFG_HAS_WIRELESS */
