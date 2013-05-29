#include <common.h>

#if (CONFIG_COMMANDS & CFG_CMD_BSP && \
     defined(CONFIG_UBOOT_CMD_BSP_TESTHW) && \
     defined(CFG_HAS_WIRELESS))

//
// Embedded 802.11b wireless network interface
// MAC layer internal defines
// Copyright 2003 Digi International
//

#ifndef _MAC_H
#define _MAC_H

#define uint32_2 		unsigned long
#define uint16 		unsigned short
#define uint8  		unsigned char

#define wln_rand    0


#define WLN_MACSTR "%02x:%02x:%02x:%02x:%02x:%02x"
#define WLN_MAC2STR(a) (a)[5], (a)[4], (a)[3], (a)[2], (a)[1], (a)[0]

//
// Implementation constants
//
#define	BUF_SIZE			512*3+128	// Max IP size (1500) + LLC/SNAP size (8)

// For the WiWave we need more MAC buffers, we need to constantly poll
// the WiWave and need buffers available to do this, otherwise
// the WiWave will lock up.
#define	BEACON_MISS			6		// Missed beacons to start new scan
#define	RSSI_SCALE			32		// RSSI samples to average

// Preferred SSID if searching for any SSID
#define	PREF_SSID			"Connect"
#define	PREF_SSID_LEN		((sizeof PREF_SSID)-1)

//
// 802.11 MIB constants
//
#define	SHORT_RETRY_LIMIT	7			// Small frame transmit retry limit
#define	LONG_RETRY_LIMIT	4			// Large frame transmit retry limit

#define	CW_MIN				31			// Min contention window size
#define	CW_MAX				1023		// Max contention window size

#define TU					1024L/1000	// Time unit (in msecs)
#define	MAX_TX_LIFETIME		(512*TU)	// Transmit lifetime limit (in msecs)
#define	MAX_RX_LIFETIME		(512*TU)	// Receive lifetime limit (in msecs)

/******************************************************************************/
/* 802.11 MAC FRAME FORMATS ***************************************************/
/******************************************************************************/

#define	IV_SIZE		4		// Initialization vector size
#define	EXTIV_SIZE	8		// IV and extended IV size
#define	SNAP_SIZE	8		// LLC/SNAP header size
#define	ICV_SIZE	4		// Integrity check value size
#define	MIC_SIZE	8		// Message integrity check size
#define	FCS_SIZE	4		// FCS (CRC-32) size

#define	WEP_SIZE	(IV_SIZE+ICV_SIZE)	// Total WEP size
#define	TKIP_SIZE	(EXTIV_SIZE+ICV_SIZE)	// Total TKIP size
#define	CCMP_SIZE	(EXTIV_SIZE+MIC_SIZE)	// Total CCMP size

#define TKIP_KEY_SIZE		32		// TKIP/MIC key size
#define CCMP_KEY_SIZE		16		// CCMP key size

#define	CHAL_SIZE		128		// Authentication challenge size

#define	RTS_SIZE		20		// RTS frame size
#define	CTS_SIZE		14		// CTS frame size
#define	DATA_SIZE		28		// Data frame header+FCS size
#define	ACK_SIZE		14		// ACK frame size

// Max total MAC frame size
#define	MAX_FRAME_SIZE		(DATA_SIZE+BUF_SIZE+TKIP_SIZE+MIC_SIZE)

// Max number of fragments
#define	MAX_FRAGS			16

// Frame header modulation type field
#define	MOD_PSKCCK		0x00	// PSK/CCK modulation
#define	MOD_OFDM		0xEE	// OFDM modulation

// PSK/CCK PLCP service field bits
#define	SERVICE_LOCKED		0x04	// Locked clocks
#define	SERVICE_MODSEL		0x08	// Modulation selection
#define	SERVICE_LENEXT		0x80	// Length extension

// MAC type field values
#define	TYPE_ASSOC_REQ		0x00	// Association request
#define	TYPE_ASSOC_RESP		0x10	// Association response
#define	TYPE_REASSOC_REQ	0x20	// Reassociation request
#define	TYPE_REASSOC_RESP	0x30	// Reassociation response
#define	TYPE_PROBE_REQ		0x40	// Probe request
#define	TYPE_PROBE_RESP		0x50	// Probe response

#define	TYPE_BEACON		0x80	// Beacon
#define	TYPE_ATIM		0x90	// Annoucement traffice indication
#define	TYPE_DISASSOC		0xA0	// Disassociation
#define	TYPE_AUTH		0xB0	// Authentication
#define	TYPE_DEAUTH		0xC0	// Deauthentication

#define TYPE_RTS		0xB4	// Request to send
#define TYPE_CTS		0xC4	// Clear to send
#define TYPE_ACK		0xD4	// Acknowledgement
#define TYPE_PSPOLL		0xA4	// Power Save(PS)-Poll

#define TYPE_DATA		0x08	// Data
#define TYPE_NULL_DATA      	0x48    // Null Data

// TRUE if buf is data or management frame
#define	IS_DATA(buf)		(((buf)->macHdr.fc.type & 0xcf) == TYPE_DATA)
#define	IS_MGMT(buf)		(((buf)->macHdr.fc.type & 0x0f) == 0)
// Authentication algorithm number field values
#define	AUTH_OPEN			0x00	// Open system
#define	AUTH_SHAREDKEY		0x01	// Shared key
#define	AUTH_LEAP			0x80	// LEAP

// Capability information field bits
#define	CAP_ESS				0x0001	// Extended service set (infrastructure)
#define	CAP_IBSS			0x0002	// Independent BSS (ad hoc)
#define	CAP_POLLABLE		0x0004	// Contention free pollable
#define	CAP_POLLREQ			0x0008	// Contention free poll request
#define	CAP_PRIVACY			0x0010	// Privacy (WEP) required
#define	CAP_SHORTPRE		0x0020	// Short preambles allowed
#define	CAP_PBCC			0x0040	// PBCC modulation allowed
#define	CAP_AGILITY			0x0080	// Channel agility in use
#define	CAP_SHORTSLOT		0x0400	// Short slot time in use
#define	CAP_DSSSOFDM		0x2000	// DSSS-OFDM in use

// Status code field values
#define	STAT_SUCCESS		0

// Reason code field values
#define	REAS_NOLONGERVALID	2
#define	REAS_DEAUTH_LEAVING	3
#define	REAS_INACTIVITY		4
#define REAS_INCORRECT_FRAME_UNAUTH 6
#define REAS_INCORRECT_FRAME_UNASSO 7

// Information element IDs
#define	ELEM_SSID			0		// Service set ID
#define	ELEM_SUPRATES		1		// Supported rates
#define	ELEM_DSPARAM		3		// DS parameter set
#define	ELEM_IBSSPARAM		6		// IBSS parameter set
#define ELEM_COUNTRY        7       // Country information
#define	ELEM_CHALLENGE		16		// Challenge text
#define ELEM_ERPINFO		42		// Extended rate PHY info
#define ELEM_RSN			48		// Robust security network (WPA2)
#define	ELEM_EXTSUPRATES	50		// Extended supported rates
#define ELEM_VENDOR			221		// Vendor extension (WPA)
// Use short preamble if allowed in BSS and params and rate > 1 mbps.
#define	USE_SHORTPRE(rate)	((rate) > 0 && \
							 !(erpInfo & ERP_BARKER) && \
							 (macParams.options & WLN_OPT_SHORTPRE))
	
// MAC address macros
#define	MAC_GROUP			0x01	// Broadcast or multicast address
#define	MAC_LOCAL			0x02	// Locally administered address

#define	IS_GROUP_ADDR(addr)	(addr[0] & MAC_GROUP)
#define	EQUAL_ADDR(a1, a2)	(memcmp (a1, a2, WLN_ADDR_SIZE) == 0)
#define	SET_ADDR(a1, a2)	(memcpy (a1, a2, WLN_ADDR_SIZE))

// Authentication algorithm number field values
#define	AUTH_OPEN			0x00	// Open system
#define	AUTH_SHAREDKEY		0x01	// Shared key
#define	AUTH_LEAP			0x80	// LEAP

// Capability information field bits
#define	CAP_ESS				0x0001	// Extended service set (infrastructure)
#define	CAP_IBSS			0x0002	// Independent BSS (ad hoc)
#define	CAP_POLLABLE		0x0004	// Contention free pollable
#define	CAP_POLLREQ			0x0008	// Contention free poll request
#define	CAP_PRIVACY			0x0010	// Privacy (WEP) required
#define	CAP_SHORTPRE		0x0020	// Short preambles allowed
#define	CAP_PBCC			0x0040	// PBCC modulation allowed
#define	CAP_AGILITY			0x0080	// Channel agility in use
#define	CAP_SHORTSLOT		0x0400	// Short slot time in use
#define	CAP_DSSSOFDM		0x2000	// DSSS-OFDM in use

// Status code field values
#define	STAT_SUCCESS		0

// Reason code field values
#define	REAS_NOLONGERVALID	2
#define	REAS_DEAUTH_LEAVING	3
#define	REAS_INACTIVITY		4
#define REAS_INCORRECT_FRAME_UNAUTH 6
#define REAS_INCORRECT_FRAME_UNASSO 7

// Information element IDs
#define	ELEM_SSID			0		// Service set ID
#define	ELEM_SUPRATES		1		// Supported rates
#define	ELEM_DSPARAM		3		// DS parameter set
#define	ELEM_IBSSPARAM		6		// IBSS parameter set
#define ELEM_COUNTRY        7       // Country information
#define	ELEM_CHALLENGE		16		// Challenge text
#define ELEM_ERPINFO		42		// Extended rate PHY info
#define ELEM_RSN			48		// Robust security network (WPA2)
#define	ELEM_EXTSUPRATES	50		// Extended supported rates
#define ELEM_VENDOR			221		// Vendor extension (WPA)

// 802.11d related defines
// minimum length field value in country information elelment
#define COUNTRY_INFO_MIN_LEN   6

// Supported rates bits
#define	RATE_BASIC			0x80	// Bit set if basic rate

// TRUE if channel number in 5 GHz band
//#define	CHAN_5G(chan)		((chan) > 14)

// ERP info bits
#define	ERP_NONERP			0x01	// Non-ERP present
#define	ERP_USEPROTECT		0x02	// Use protection
#define	ERP_BARKER			0x04	// Barker (long) preamble mode

// WPA/RSN info length field
#define	wpa_info_len		wpa_info[1]
#define rsn_info_len        rsn_info[1]

// Key ID byte in data frame body
#define	EXT_IV				0x20	// Extended IV is present

// Correct CRC-32 check value
#define	GOOD_CRC32			0x2144df1c


#define GET_UPPER_LONG(x)	(x >> 16)
#define GET_LOWER_LONG(x)	(x & 0x0000FFFF)

//
// 802.11 MAC frame structures.
// These structures lay out a complete PLCP/MAC frame in a single memory region.
// They are compiler dependent.
//

/******************************************************************************/
/* RECEIVE FRAME HEADER --> PIPER Programming Manual 1.3 - Page 46 ************/
/******************************************************************************/
typedef uint32_2 RxFrameHeader;

#define GET_MODTYP_RX(x)	(x >> 24)
#define GET_ANTENNA(x)		((x & 0x00800000) >> 22)	
#define GET_RSSIVGA(x)		((x & 0x00600000) >> 20)
#define GET_RSSILNA(x)		((x & 0x001F8000) >> 18)
#define GET_FREQOFF(x)		( x & 0x00007FFF)

/******************************************************************************/
/* TRANSMIT FRAME HEADER --> PIPER Programming Manual 1.3 - Page 48 ***********/
/******************************************************************************/
typedef uint32_2 TxFrameHeader;

#define GET_LSBLENGTH(x)	(x >> 24)
#define GET_MSBLENGTH(x)	GET_LOWER_LONG(x)
#define GET_MODTYP_TX(x)	(x & 0x000000FF)

#define SET_TXFRAMEHEADER(length, mod) (((0x00000000 | (0x1F & length)) << 8) | (0xFF & mod))

/* Union of frame header types ************************************************/
typedef union {
	RxFrameHeader rx;		// Receive frame header
	TxFrameHeader tx;		// Transmit frame header
} FrameHeader;

/******************************************************************************/
/* PSK/CCK PLCP HEADER (WORD 2) --> PIPER Programming Manual 1.3 - Page 45|48 */
/* This frame is common for Tx and Rx buffers for PSK/CCK Frames.	      */
/******************************************************************************/
typedef uint32_2 PskCckHeader;

/* #define GET_SIGNAL(x)		(x >> 24)
#define GET_SERVICE(x)		((x & 0x00FF0000) >> 16) 
#define GET_LENGTH(x)		(x & 0x0000FFFF)*/
#define GET_PSKHEADER_LENGTH(x)		( (x) >> 16 )
#define GET_PSKHEADER_SERVICE(x)	( (x & 0x0000FF00) >> 8 )
#define GET_PSKHEADER_SIGNAL(x)		( x & 0x000000FF )

#define SET_PSKCCKHEADER(length, service, signal) ( ( ( ( (0x00000000 | (0xFFFF & length)) << 8 ) | (0xFF & service) ) << 8 ) | (0xFF & signal) ) 

/******************************************************************************/
/* OFMD PLCP HEADER (WORD 2) --> PIPER Programming Manual 1.3 - Page 46       */
/* This frame is common for Tx and Rx buffers for OFDM Frames.   	      */
/******************************************************************************/
typedef uint32_2 OfdmHeader;

/******************************************************************************/
/* __________________________________________________________________________ */
/* | P | LENGTH | R | RATE | ________________________________________________ */
/* |17 | 16 - 5 | 4 |3 - 0 | ________________________________________________ */
/******************************************************************************/

#define GET_OFDMHEADER_PARITY(x)	( (x & 0x00020000) >> 18 )
#define GET_OFDMHEADER_LENGTH(x)	( (x & 0x0001FFE0) >> 5 )
#define GET_OFDMHEADER_RATE(x)		( x & 0x0000000F )

#define SET_PSKOFDMHEADER(parity, length, rate) ( ( ( ( (0x00000000 | (0x1 & parity)) << 12 ) | (0xFFF & length) ) <<  5) | (0xF & rate) )

/* Union of PLCP header types *************************************************/
typedef union {
	PskCckHeader 	pskcck;	// PLCP header for PSK/CCK
	OfdmHeader 	ofdm;	// PLCP header for OFDM
} PlcpHeader;

/******************************************************************************/
/* FRAME CONTROL / DURATION FIELD -->  802.11 - Chapter 3 - Page 47 & 50      */
/******************************************************************************/
typedef uint32_2 FC_DUR;

#define GET_FC_DUR_DURATION(x)		( ((x) & 0xFFFF0000) >> 16 )
#define GET_FC_DUR_ORDER(x)		( ((x) & 0x00008000) >> 15 )
#define GET_FC_DUR_PROTECTEDFRAME(x)	( ((x) & 0x00004000) >> 14 )
#define GET_FC_DUR_MOREDATA(x)		( ((x) & 0x00002000) >> 13 )
#define GET_FC_DUR_PWRMGMT(x)		( ((x) & 0x00001000) >> 12 )
#define GET_FC_DUR_RETRY(x)		( ((x) & 0x00000800) >> 11 )
#define GET_FC_DUR_MOREFRAG(x)		( ((x) & 0x00000400) >> 10 )
#define GET_FC_DUR_FROMDS(x)		( ((x) & 0x00000200) >> 9 )
#define GET_FC_DUR_TODS(x)		( ((x) & 0x00000100) >> 8 )
#define GET_FC_DUR_SUBTYPE(x)		( ((x) & 0x000000F0) >> 4 )
#define GET_FC_DUR_TYPE(x)		( ((x) & 0x0000000C) >> 2 )
#define GET_FC_DUR_PROTOCOL(x)		( ((x) & 0x00000003) )

#define SET_FC_DUR(duration, order, procframe, moredata, pwrmanag, retry, morefrag, fds, tds, subtype, type, protocol) \
	((((((((((((( 0xFFFF & duration ) << 1 ) | ( 0x1 & order ) << 1 ) | ( 0x1 & procframe ) << 1 ) | ( 0x1 & moredata ) << 1 ) | \
	( 0x1 & pwrmanag ) << 1 ) | ( 0x1 & retry ) << 1 ) | ( 0x1 & morefrag ) << 1 ) | ( 0x1 & fds ) << 1 ) | \
	( 0x1 & tds ) << 4 ) | ( 0xF & subtype ) << 2 ) | ( 0x3 & type ) << 2 ) | ( 0x3 & protocol ) )

/* MAC address */
typedef unsigned char MacAddr[6];

/******************************************************************************/
/* SEQUENCE CONTROL FIELD -->  802.11 - Chapter 3 - Page 52                   */
/******************************************************************************/
typedef struct {
	unsigned seq		:12;	// Sequence number
	unsigned frag		:4;	// Fragment number
} SeqControl;

/* Union of sequence control types ********************************************/
typedef union {
	SeqControl 	sq;		// Sequence control fields
	uint16 		sq16;		// Sequence control as 16-bit int
} SeqControlU;

//#pragma pack(1)


/******************************************************************************/
/* GENERIC MAC FRAME -->  802.11 - Chapter 3 - Page 52                        */
/******************************************************************************/
typedef struct {
	FC_DUR 	fc_duration;		// Frame control
					// Duration/ID
	uint32_2 	addr1;			// Address 1
	uint32_2 	addr1_2;		// Address 1_2
	uint32_2 	addr2;			// Address 2
	uint32_2 	addr3;			// Address 2_3
	uint32_2 	addr3_squ;		// Address 3_Sequence control fields
}  MacHeader;

#define GET_PART_MAC1(x)	GET_UPPER_LONG(x)
#define GET_PART_MAC2(x)	GET_LOWER_LONG(x)
#define GET_PART_MAC3(x)	GET_UPPER_LONG(x)
#define GET_SQU(x)		GET_LOWER_LONG(x)

#define SET_MAC1_MAC2(mac1, mac2) ( ( ( 0xFFFF & mac1 ) << 16 ) | ( 0xFFFF & mac2 ) )
#define SET_MAC2_MAC3(mac2, mac3) ( ( ( 0xFFFF & mac2 ) << 16 ) | ( 0xFFFF & mac3 ) )
#define SET_MAC3_SQU(mac3, squ)  ( ( ( 0xFFFF & mac3 ) << 16 ) | ( 0xFFFF & squ ) )

// MAC buffer, including complete MAC frame
typedef struct MacFrameHeader 
{
	uint32_2  	frameHdr;	// Frame header
	uint32_2		plcpHdr;	// PLCP header
	
	uint32_2 	fc_duration;		// Frame control
					// Duration/ID
	uint32_2 	addr1;			// Address 1
	uint32_2 	addr1_2;		// Address 1_2
	uint32_2 	addr2;			// Address 2
	uint32_2 	addr3;			// Address 2_3
	uint32_2 	addr3_squ;		// Address 3_Sequence control fields
	//MacHeader	macHdr;		// MAC header*/
	
} MacFrameHeader;

typedef struct BeaconStruct
{
	uint32_2		timestamp[2];
	uint32_2 		interval_capinfo;
	uint8		nameSSID[10];
} BeaconStruct;

typedef struct BeaconFrame
{
	//MacFrameHeader 	macHdr;
	uint32_2  	frameHdr;	// Frame header
	uint32_2		plcpHdr;	// PLCP header
	
	uint32_2 		fc_duration;		// Frame control
					// Duration/ID
	uint32_2 		addr1;			// Address 1
	uint32_2 		addr1_2;		// Address 1_2
	uint32_2 		addr2;			// Address 2
	uint32_2 		addr3;			// Address 2_3
	uint32_2 		addr3_squ;		// Address 3_Sequence control fields
	uint32_2		timestamp[2];
	uint32_2 		interval_capinfo;
	uint8		nameSSID[10];
	//BeaconStruct	beaconFrm;	
} BeaconFrame;


// Rate values
#define	RATE_MASK_A		0x0ff0
#define	RATE_MASK_B		0x000f
#define	RATE_MASK_G		0x0fff

#define	RATE_MASK_PSK		0x0003
#define	RATE_MASK_CCK		0x000c
#define	RATE_MASK_OFDM		0x0ff0

#define	RATE_MASK_BASIC		0x0153	// Ad hoc basic rates

#define	RATE_MIN_A		4
#define	RATE_MIN_B		0

#endif

#endif /* CFG_HAS_WIRELESS */ 

