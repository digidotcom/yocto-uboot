#include <common.h>

#if (CONFIG_COMMANDS & CFG_CMD_BSP && \
     defined(CONFIG_UBOOT_CMD_BSP_TESTHW) && \
     defined(CFG_HAS_WIRELESS))

//
// Embedded 802.11a/g wireless network interface
// MAC layer hardware interface for ConnectCorewi9p9215 and Connectwiem9210
// Copyright 2008 Digi International
//

//#include "wifi.h"
#include "mac.h"
#include "vBPiper.h"

//#include "malloc.h"
#include "AIROHA_7230.h"

/* Includes Baseband firmware image *******************************************/
#include "wifig_dsp_ucode.c"
#include "wifig_macassist_ucode.c"

int HW_TEST_MODE = 0;
int HW_TX_BUFFER[2000];

static unsigned char fcc_txbuffer[64];

#if 0
unsigned char fcc_txbuffer2[36]={0x00,0x09,0x00,0x00,0x0a,0x04,0x00,0x01, 
	                         0x08,0x00,0x00,0x00,0x00,0x22,0x33,0x44,
	                         0x55,0x66,0x10,0x20,0x30,0x40,0x50,0x60,
	                         0x10,0x20,0x30,0x40,0x50,0x60,0x00,0x00,
	                         0x05,0x55,0x55,0x00};
#endif
	
/* static unsigned char radio_txbuffer[1024]; */

#if CONFIG_CCW9M2443
void piperReset (void)
{
	(*(volatile unsigned int *)(0x56000010)) = 0x00000050;
	(*(volatile unsigned int *)(0x56000018)) = 0x002AAA2A;
	(*(volatile unsigned int *)(0x56000014)) = 0x00000000;
	waitUS(10);//10
	(*(volatile unsigned int *)(0x56000014)) = 0x0000000C;
	waitUS(500);//500
}
#elif CONFIG_CCW9P9215
void piperReset (void)
{
	(*(volatile unsigned int *)(0xA090205C)) = 0x1818181D;//0x1818181B;
	(*(volatile unsigned int *)(0xA0902074)) = 0x00240000;//0x10240000;
	waitUS(10);
	(*(volatile unsigned int *)(0xA0902074)) = 0x12240000;//0x10240000;
	waitUS(500);
}
#endif

inline void __write32 (volatile unsigned int *addr, unsigned int data, unsigned char operation)
{
	#if CONFIG_CCW9M2443
		typedef union
		{
			unsigned int reg32;
			struct
			{
				unsigned short LSB;
				unsigned short MSB;
			} reg16;
		} c32to16;
		
		c32to16 aux;
		
		volatile unsigned short *addr_16 = (volatile unsigned short *) addr;
	#elif CONFIG_CCW9P9215
		volatile unsigned int *addr_32 = (volatile unsigned int *) addr;
	#endif
	
	switch (operation)
	{
		case WRITE:
			#if CONFIG_CCW9M2443
				aux.reg32 = data;
				
				*(addr_16 + 1) = aux.reg16.MSB;
				*(addr_16)     = aux.reg16.LSB;

			/*printf("Pointer: %p, %p, %p\n", addr, addr_16, addr_16+1);
			printf("Data: %x, %x, %x\n", aux.reg32, aux.reg16.MSB, aux.reg16.LSB);*/
				
			#elif CONFIG_CCW9P9215
				*addr_32 = data;
			#endif
		break;
		case AND:
			#if CONFIG_CCW9M2443				
				aux.reg16.LSB = *(addr_16);
				aux.reg16.MSB = *(addr_16 + 1);
				
				aux.reg32 &= data;
				
				*(addr_16 + 1) = aux.reg16.MSB;
				*(addr_16)     = aux.reg16.LSB;
			#elif CONFIG_CCW9P9215
				*addr_32 &= data;
			#endif
		break;
		case OR:
			#if CONFIG_CCW9M2443				
				aux.reg16.LSB = *(addr_16);
				aux.reg16.MSB = *(addr_16 + 1);
				
				aux.reg32 |= data;
				
				*(addr_16 + 1) = aux.reg16.MSB;
				*(addr_16)     = aux.reg16.LSB;	
			#elif  CONFIG_CCW9P9215
				*addr_32 |= data;
			#endif
		break;
		default:
		break;
			
	}
}

void __read32 (volatile unsigned int *addr, unsigned int *data )
{
	#if CONFIG_CCW9M2443
		typedef union
		{
			unsigned int reg32;
			struct
			{
				unsigned short LSB;
				unsigned short MSB;
			} reg16;
		} c32to16;
		
		c32to16 aux;
		
		unsigned short *addr_16;
		addr_16 = (unsigned short *) addr;
			
		aux.reg16.LSB = *(addr_16);
		aux.reg16.MSB = *(addr_16 + 1);
		
		*data = aux.reg32;
	#elif CONFIG_CCW9P9215
		*data = (unsigned int) *addr;
	#endif		
}

#if 0
static void HWMemcpy (volatile void *dst_, volatile void *src_, int len)

{
	volatile uint32 *dst = (uint32 *) dst_;
	volatile uint32 *src = (uint32 *) src_;

	for (; len >= 16; len -= 16)
	{
    		__write32 (dst++, *src++, WRITE);
		__write32 (dst++, *src++, WRITE);
		__write32 (dst++, *src++, WRITE);
		__write32 (dst++, *src++, WRITE);
	}
    		
	for (; len > 0; len -= 4)
    		__write32 (dst++, *src++, WRITE);
}

//
// Copy from receive FIFO
//
static void HWReadFifo (void *dst_, int len)
{
	uint32 *dst = (uint32 *) dst_;

	for (; len >= 16; len -= 16) //ERRORROROROR: Increase dst for reading.
	{
		__read32 (HW_DATA_FIFO, dst);
		//ByteSwap(dst);
		//waitUS(50);
		__read32 (HW_DATA_FIFO, dst);
		//ByteSwap(dst);
		//waitUS(50);
		__read32 (HW_DATA_FIFO, dst);
		//ByteSwap(dst);
		//waitUS(50);
		__read32 (HW_DATA_FIFO, dst);
		//ByteSwap(dst);
		//waitUS(50);
	}
	for (; len > 0; len -= 4)
	{
		__read32 (HW_DATA_FIFO, dst);
		//ByteSwap(dst);
		//waitUS(50);
	}
}

#endif 

static void HWWriteFifo (void *src_, int len)
{
	uint32 *src = (uint32 *) src_;
  	
	__write32 (HW_GEN_CONTROL, GEN_TXHOLD, OR);
	//waitUS(5);

	for (; len >= 16; len -= 16)
	{
		__write32 (HW_DATA_FIFO, *src++, WRITE);
		__write32 (HW_DATA_FIFO, *src++, WRITE);
		__write32 (HW_DATA_FIFO, *src++, WRITE);
		__write32 (HW_DATA_FIFO, *src++, WRITE);
	}

	for (; len > 0; len -= 4)
		__write32 (HW_DATA_FIFO, *src++, WRITE);

	//waitUS(5);
	__write32 (HW_GEN_CONTROL, ~GEN_TXHOLD, AND);
	
}

#if CONFIG_CCW9M2443
void initializeuPToKnowState (void)
{
	(*(volatile unsigned int *)(0x4f000080)) = 0x0000000F;
	(*(volatile unsigned int *)(0x4f000084)) = 0x0000001F;
	(*(volatile unsigned int *)(0x4f000088)) = 0x0000001F;
	(*(volatile unsigned int *)(0x4f00008C)) = 0x0000000F;
	(*(volatile unsigned int *)(0x4f000090)) = 0x0000000F;
	(*(volatile unsigned int *)(0x4f000094)) = 0x0020A091;//0x00202011;
}
#elif CONFIG_CCW9P9215
/* This funtions perform and initialization of the NS9215 CPU to a known state 
   after the initialization with U-Boot */
void initializeuPToKnowState (void)
{
	/* Configure the following register from NS9215 */
	/* GPIO's involved */
	/*	PIPER_RESET in port GPIO_92 */
	(*(volatile unsigned int *)(0xA090205C)) = 0x1818181D;//0x1818181B;
	(*(volatile unsigned int *)(0xA0902074)) = 0x12240000;//0x10240000;

	/* Interruptions */
	/*	PIPER_INT in port GPIO_A0 */
	(*(volatile unsigned int *)(0xA0902068)) = 0x18181818;/* 0b00011000 */
	
	/* CS3 is used for wireless baseband registers. The registers start at address 0x00 - 
        the base address is defined by the configuration of the chip select.  */
	*((volatile unsigned int *)0xA0700260) = 0x00000082;
	/* SOFT RESET */
	(*(volatile unsigned int *)(0x70000004)) = 0x40000000;//0x10240000;	
}
#endif

/*
 * Write the Piper register to the documented reset values in the PM v1.3 
 */

void registersPiperToDefault (void)
{
	__write32 (HW_GEN_CONTROL, 0x377F4000, WRITE);
	__write32 (HW_GEN_STATUS, 0x30000010, WRITE);
	__write32 (HW_RSSI_AES, 0x0000007F, WRITE);
	__write32 (HW_INTR_MASK, 0x00000000, WRITE);
	__write32 (HW_INTR_STATUS, 0x00000000, WRITE);
	__write32 (HW_SPI_CTRL, 0x00000018, WRITE);
	__write32 (HW_DATA_FIFO, 0x00000000, WRITE);
	
	//HW_TRACK_CONTROL 	= 0x377F4000;
	
	__write32 (HW_CONF1, 0x8043002C, WRITE);
	__write32 (HW_CONF2, 0x0882B314, WRITE);
	__write32 (HW_AES_MODE, 0x00000000, WRITE);
	__write32 (HW_OUT_CTRL, 0x00000001, WRITE);
}

/*
// Delay for usec microseconds
*/
void waitUS (unsigned long usec)
{
	udelay(usec);
}

/*
// Write data to an RF tranceiver register
// @param addr Register address (4 bits)
// @param data Data to write (20 bits, bit reversed)
*/
void WriteRF (uint8 addr, uint32 data)
{
	__write32(HW_SPI, (data << 4) | addr, WRITE);
	waitUS (10);
}

void ByteSwap (unsigned int *x)
{
   *x = ((((*x) & 0xff) << 24) | (((*x) & 0xff00) << 8) | \
	   (((*x) >> 8) & 0xff00) | (((*x) >> 24) & 0xff));
}

/*
// Load the baseband controller firmware
*/
int LoadHW (void)
{
	static boolean loaded = FALSE;
	#if CONFIG_CCW9P9215
	volatile unsigned int *addr;
	#endif
	int i;
	
	//int timeout1 = 300, timeout2 = 300;

	// Load only once after reset
	if (loaded == TRUE) return 0;
	
	//__write32(HW_GEN_CONTROL, 0x377F40000, WRITE);
	__write32(HW_GEN_CONTROL, 0x377F4000, WRITE);
	
	#if CONFIG_CCW9M2443
	//printf("*************************************************\n");
	// Zero out MAC assist SRAM (put into known state before enabling MAC assist)
	for (i = 0; i <= 0x42; i += 0x04)//Tested 0x40 ok in w9m
	{	
		__write32(REG32(0x40 + i), 0, WRITE);
	}
	
	/*for (addr=MAC_CTRL_BASE; addr<(MAC_CTRL_BASE+ 0x20); addr += 0x04)
        {
                __write32(addr, 0, WRITE);
        }*/

	//printf("*************************************************\n");
	#elif CONFIG_CCW9P9215
	for (addr=MAC_CTRL_BASE; addr<(MAC_CTRL_BASE+ 0x20); addr += 0x04)
        {
                __write32(addr, 0, WRITE);
        }
	#endif

    	// Enable download the MAC Assist program RAM
	__write32(HW_GEN_CONTROL, MACASSIST_LOAD_ENABLE, OR);
	
	/* Load MAC Assist data */
	/* no byte swap is necessary */
	for (i = 0; i < macassist_data_len; i++)
	{
		__write32(HW_DATA_FIFO, wifi_macassist_ucode[i], WRITE);
        }

    	// disable MAC Assist download
    	__write32(HW_GEN_CONTROL, ~MACASSIST_LOAD_ENABLE, AND);

	// Enable download the DSP program RAM
    	__write32(HW_GEN_CONTROL, DSP_LOAD_ENABLE, OR);

	/* Load DSP data */
	/* no byte swap is necessary */
    	for (i = 0; i < dsp_data_len; i++)
    	{
	        __write32(HW_DATA_FIFO, wifi_dsp_ucode[i], WRITE);
    	}
    	// disable DSP download
    	__write32(HW_GEN_CONTROL, ~DSP_LOAD_ENABLE, AND);
	
	// set bit-11 in the general control register to a 1 to start the processors
    	//HW_GEN_CONTROL |= DSP_MACASSIST_ENABLE; // Here does not work not even with delays.
    	//__write32(HW_GEN_CONTROL, DSP_MACASSIST_ENABLE, OR);
    	/*for (i = 0; i <= 15; i++)
    		waitUS(1000000);*/
    	    	
#if 	0 //mike_spike_code
     /* OK, I finally have this timing alignment code working.  The new "magic"
      value is 0x63 at address 0xA62.  Bit-0 indicates the timing measurement
      is complete.  Bit-1 indicates that a second timing measurment was
      performed.  The upper nibble is the timing measurement value.

      This code should eliminate the possibility of spikes at the beginning
      of all PSK/CCK frames and eliminate the spikes at the end of all
      PSK (1M, 2M) frames.
      */
                
    // reset the timing value WrPortI(0xA62, NULL, 0x00); 
    HW_MAC_STATUS &= 0xffff00ff;
                    
    while ( (HW_MAC_STATUS & 0x0000ff00) != 0x00006300) //while (RdPortI(0xA62) != 0x63)
    {          
        // reset the timing value 
        HW_MAC_STATUS &= 0xffff00ff;   //WrPortI(0xA62, NULL, 0x00);    

        // issue WiFi soft reset     
        HW_GEN_STATUS = 0x40000000;    //_wc_write32(0xA08, 0x40000000);                        

        // Set TX_ON Low WrPortI(0xA3C, NULL, ((RdPortI(0xA3C) & ~0xC0) | 0x80));
        HW_OUT_CTRL &= 0xffffff3f;
        HW_OUT_CTRL |= 0x00000080;
    
        // Set PA_2G Low WrPortI(0xA3D, NULL, ((RdPortI(0xA3D) & ~0x0C) | 0x08)); 
        HW_OUT_CTRL &= 0xfffff0ff;
        HW_OUT_CTRL |= 0x00000a00;  //both PA_2G and PA_5G low
           
        // Set RX_ON low  WrPortI(0xA3F, NULL, ((RdPortI(0xA3F) & ~0x30) | 0x20)); 
        HW_OUT_CTRL &= 0xcfffffff;
        HW_OUT_CTRL |= 0x20000000;
           
        // start the WiFi mac & dsp
        HW_GEN_CONTROL = 0x37780820;                 

        timeout1= 500;

        // Wait for timing measurement to finish
        while ( (HW_MAC_STATUS & 0x0000ff00) != 0x00000100) //((RdPortI(0xA62) & 0x01) != 0x01)
        {
           waitUS(2);
           timeout1--;
           if (!timeout1)
               break;
        }

        timeout2--;
        if(!timeout2)
            return 1;
    }   

    // Set TX_ON/RXHP_ON and RX to normal wifi, restore the reset value to HW_OUT_CTRL
    HW_OUT_CTRL = 0x1;

#endif //mike's spike code   
                
       	// set the TX-hold bit
	/**///__write32(HW_GEN_CONTROL, 0x37780080, WRITE);
	__write32(HW_GEN_CONTROL, 0x00000080, OR);

	// clear the TX-FIFO memory
	for (i=0; i<448; i++)
		__write32(HW_DATA_FIFO, 0, WRITE);

	// reset the TX-FIFO
	/**///__write32(HW_GEN_CONTROL, 0x377800C0, WRITE); /*???????????????????????*/
	__write32(HW_GEN_CONTROL, 0x00000040, OR);
	
	// release the TX-hold and reset 	
	/**///__write32(HW_GEN_CONTROL, 0x377800C0, WRITE);
	/**///__write32(HW_GEN_CONTROL, 0x37780000, WRITE);
	__write32(HW_GEN_CONTROL, 0x000000C0, OR);
	__write32(HW_GEN_CONTROL, ~0x000000C0, AND);
	
	// AGC disable
	__write32(HW_GEN_CONTROL, ~0x00800000, AND);
		
	loaded = TRUE;
	return 0;
}


void PowerOnCalibrationRF (unsigned char band_selection, unsigned char one_time)
{
	const unsigned int cal_reg_RF [2][4] =	{
							/* BAND B/G */
							{0x9ABA8, 0x3ABA8, 0x1ABA8, 0x00000300},
       							/* BAND A */
							{0x9ABA8, 0x3ABA8, 0x12BAC, 0x00000300}//0x00000f00
	
						};
						
	unsigned int i;
	
	/* Power-on calibration procedure */
	if (one_time)
	{
		__write32(HW_OUT_CTRL, cal_reg_RF[band_selection][3], OR);
    		waitUS(150);
	}
	
	/* Calibration procedure */
	for (i=0;i<3;i++)
	{
		WriteRF(15,cal_reg_RF[band_selection][i]);
		waitUS(50);
	}
	
	//TESTING
	//__write32(HW_OUT_CTRL, ~cal_reg_RF[band_selection][3], AND);
	
}

void InitializeRF(unsigned char band_selection)
{
	
	/*
	For both bands use only the following configuretion:
		AND 0xfffff0ff
		OR  0x00000200
		OR  0x00000300
	
	Ignote NETOS configuration
	
	*/
	
	const unsigned int ini_reg_Piper [2][2] = {
							/* BAND B/G */
						  	{0xfffff0ff, 0x00000200},//cff,200 
	 						/* BAND A */
						  	{0xfffff0ff, 0x00000200}//0ff,a00
						  };
	
	const unsigned int ini_reg_RF [2][16] = {
							/* BAND B/G */
							{ 0x0037C, 0x13333, 0x841FF, 0x3FDFA, 0x7FD78, 0x802BF, 0x56AF3, 0xCE000, 
						  	  0x6EBC0, 0x221BB, 0xE0040, 0x08031, 0x000A3, 0xFFFFF, 0x00000, 0x1ABA8 },//E0040
	   						/* BAND A */
							{ 0x0FF52, 0x00000, 0x451FE, 0x5FDFA, 0x67f78, 0x853FF, 0x56AF3, 0xCE000,
						  	  0x6EBC0, 0x221BB, 0xE0600, 0x08031, 0x00143, 0xFFFFF, 0x00000, 0x12BAC } //0870
						};
						
	unsigned char i;
	
	/* Initial setting for Piper to control RF transceiver */

	/* Initial settings for 20 MHz reference frequency, 802.11b/g */
	__write32(HW_OUT_CTRL, ini_reg_Piper[band_selection][0], AND);
	__write32(HW_OUT_CTRL, ini_reg_Piper[band_selection][1], OR);
	waitUS(150);	

	
    	/* Register initialization of the RF transceiver */
    	for (i=0;i<=15;i++)
	{
		WriteRF(i,ini_reg_RF[band_selection][i]);
		//waitUS(10);
	}

	//PowerOnCalibrationRF(band_selection);
}

//
// Initialize the wireless hardware
//
int InitHW (void)
{
	//unsigned long aux32;

	// Load the baseband controller firmware
	if (LoadHW() != 0) return 1;

	/* Initialize baseband general control register */
	__write32(HW_GEN_CONTROL, GEN_INIT_AIROHA_24GHZ, WRITE); //0x31780005
	
	/* OPTIONAL? */
	__write32(HW_CONF1, 0xff00ffff, AND);
	__write32(HW_CONF1, TRACK_BG_BAND, WRITE);
	
	/* OPTIONAL? */
	__write32(HW_GEN_STATUS, 1 << 26, OR); // Set the RXHP RST bi
	          
	/* OPTIONAL? */
	__write32(HW_CONF2, 0xff00003f, AND);
	__write32(HW_CONF2, (0x0C << 18), OR);
	__write32(HW_CONF2, (0xA6B << 6), OR);  
	
	/* OPTIONAL? */
	__write32(HW_CONF2, 0x08329AD4, WRITE);
	
	//HW_GEN_CONTROL |= DSP_MACASSIST_ENABLE; /*****************************************************************************/
	
	/* Initialize the SPI word length */	   
	__write32(HW_SPI_CONTROL, SPI_INIT_AIROHA, WRITE);
		
	// Clear the Interrupt Mask Register before enabling external interrupts.
	// Also clear out any status bits in the Interrupt Status Register.
	__write32(HW_INTR_MASK, 0, WRITE);
	__write32(HW_INTR_STATUS, 0x00, WRITE);//0xff;
	__write32(HW_INTR_MASK, INTR_RXFIFO|INTR_TXEND|INTR_TIMEOUT|INTR_RXOVERRUN, WRITE);
	
	/*
	 * Make sure secondary MAC addresses are disabled and set to
	 * zero by default.
	 */
	__write32(HW_MAC_CONTROL, ~CTRL_MAC_FLTR, AND);
	
	/* make sure addresses are zero to start with
	 * to insure we don't start with extraneous bits */
	__write32(HW_STA2ID0, 0, WRITE);
	__write32(HW_STA2ID1, 0, WRITE);
	__write32(HW_STA3ID0, 0, WRITE);
	__write32(HW_STA3ID1, 0, WRITE);

	
	__write32(HW_STAID0, 0x00010203, WRITE);
	__write32(HW_STAID1, 0x04050000, OR);
	//__write32(HW_STAID0, 0x0004F301, WRITE);
	//__write32(HW_STAID1, 0x8E720000, OR);
	
	// Antenna Mapping
	__write32(HW_RSSI_AES, 0x1E000000, OR);
	// Select Primary Antenna 
	__write32(HW_GEN_CONTROL, 0xfffffffb, AND);  

	// reset RX and TX FIFOs
    	__write32(HW_GEN_CONTROL, (GEN_RXFIFORST | GEN_TXFIFORST), OR);
	__write32(HW_GEN_CONTROL, ~(GEN_RXFIFORST | GEN_TXFIFORST), AND);
	    	
	// DAC Quadrature-Inphase enable
	// Tracking cte 2412-2472 Mhz
	__write32(HW_CONF1, 0xC043002C, WRITE);
		
	/* Initialize RF transceiver */
	InitializeRF(RF_24GHZ);
	PowerOnCalibrationRF(RF_24GHZ,1);


/*#ifdef AIROHA_PWR_CALIBRATION
   	initPwrCal();
#endif*/

	// Host int inverted.
	__write32(HW_OUT_CTRL, 1 << 26, OR);
	  
	return 0;
}
void ShutdownHW (void)
{
	//printf ("HW shutdown\n");
    
	//HW_GEN_CONTROL = GEN_RESET; //this is causing the ccwi9c hang
	// Disable receiving
	__write32(HW_GEN_CONTROL, ~GEN_RXEN, AND);
	__write32(HW_MAC_CONTROL, 0, WRITE);
	__write32(HW_INTR_MASK, 0, WRITE);
}

//
// Select a channel
// @param channel Channel number: 1-22
//
void SetChannel (unsigned char channel)
{
	/* Disable the rx processing path */
    	__write32(HW_GEN_CONTROL, ~GEN_RXEN, AND);
	
	/* perform chip and frequency-band specific RF initialization */
	
	if ( CHAN_5G(channel) )
	{
		__write32(HW_GEN_CONTROL, ~GEN_PA_ON, AND);
		
		/* 12/03/2009 */
// 		/* Added from initial 5 Ghz configuration */
		//__write32(HW_GEN_CONTROL, 0x8, OR);
		
		/* 12/03/2009 */
		/* Non fully tested */
		__write32(HW_TRACK_CONTROL, 0xff00ffff, AND);
		if (CHAN_4920_4980(channel))
			__write32(HW_TRACK_CONTROL, TRACK_4920_4980_A_BAND, OR);
		else if (CHAN_5150_5350(channel))
			__write32(HW_TRACK_CONTROL, TRACK_5150_5350_A_BAND, OR);
		else if (CHAN_5470_5725(channel))
			__write32(HW_TRACK_CONTROL, TRACK_5470_5725_A_BAND, OR);
		else if (CHAN_5725_5825(channel))
			__write32(HW_TRACK_CONTROL, TRACK_5725_5825_A_BAND, OR);
		else
			__write32(HW_TRACK_CONTROL, TRACK_BG_BAND, OR);

		InitializeRF(RF_50GHZ);
		PowerOnCalibrationRF(RF_50GHZ, 1);
	}
	else
	{
		__write32(HW_GEN_CONTROL, GEN_PA_ON, OR);
		
		/* 12/03/2009 */
		/* Added from initial 2.4 Ghz configuration */
		//__write32(HW_GEN_CONTROL, ~0x8, AND);
		
		/* 12/03/2009 */
		/* Non fully tested */
		__write32(HW_TRACK_CONTROL, 0xff00ffff, AND);
		__write32(HW_TRACK_CONTROL, TRACK_BG_BAND, OR);
		
		InitializeRF(RF_24GHZ);
		PowerOnCalibrationRF(RF_24GHZ, 1);
	    	
		//__write32(HW_GEN_CONTROL, ~GEN_5GEN, AND);
	}

	/* Set the channel frequency */
	WriteRF (0, freqTableAiroha_7230[channel].integer);
	waitUS(1);
	WriteRF (1, freqTableAiroha_7230[channel].fraction);
	waitUS(11); //before waitUS(1);
	    	
	if ( CHAN_5G(channel) )
	{
		WriteRF (4, freqTableAiroha_7230[channel].addres4Airoha);
		waitUS(10);
		PowerOnCalibrationRF(RF_50GHZ, 1);
		// No effect detected __write32(HW_GEN_CONTROL, GEN_5GEN, OR);
		//__write32(HW_GEN_CONTROL, ~GEN_5GEN, AND);
	}
	else
	{
		WriteRF (4, freqTableAiroha_7230[channel].addres4Airoha);
		waitUS(10);
		PowerOnCalibrationRF(RF_24GHZ, 1);
		
		__write32(HW_GEN_CONTROL, ~GEN_5GEN, AND);
	}
	
	/* configure the baseband processing engine */		    
	//__write32(HW_GEN_CONTROL, ~GEN_5GEN, AND);
	
	/*Re-enable the rx processing path */
	__write32(HW_GEN_CONTROL, GEN_RXEN, OR);
}


//
// This function is used to fill the fcc_txbuffer with random data
// for the transmit test.
//
void MacUpdateFccBuffer(unsigned int transmit_mode, unsigned int transmit_rate)
{
	int i;
	unsigned long *aux;
	/* WARNING MODIFICATION */
	aux = (unsigned long *) fcc_txbuffer;/* (added unsigned long *) */
	const unsigned char auxArray[12][3] = 
				{
					{0x00, 0x10, 0x0A}, /*1mpbs*/
					{0x00, 0x10, 0x14}, /*2mbps*/
					{0x00, 0x10, 0x37}, /*5.5mbps*/
					{0x00, 0x10, 0x6E}, /*11mbps*/
					  /*** OFMD ***/
					{0xEE, 0x10, 0x0B}, /*6mbps*/
					{0xEE, 0x10, 0x0F}, /*9mbps*/
					{0xEE, 0x10, 0x0A}, /*12mbps*/
					{0xEE, 0x10, 0x0E}, /*18mbps*/
					{0xEE, 0x10, 0x09}, /*24mbps*/
					{0xEE, 0x10, 0x0D}, /*36mbps*/
					{0xEE, 0x10, 0x08}, /*48mbps*/
					{0xEE, 0x10, 0x0C}, /*54mbps*/
				};

	if (transmit_rate >= 0 && transmit_rate <= 11)
	{
		*aux = (auxArray[transmit_rate][0]  << 24) | (auxArray[transmit_rate][1]  << 16) | 0x00000000; 
		*(aux+1) = (auxArray[transmit_rate][2]  << 24) | 0x00000000;
		
		//printf ("%x\n", *aux);
		//printf ("%x", *(aux+1));
		//fcc_txbuffer[3] = (auxArray[transmit_rate][0]); 
	    	//fcc_txbuffer[2] = (auxArray[transmit_rate][1]); 
		//fcc_txbuffer[7] = (auxArray[transmit_rate][2]);
		
	}
	else
	{
		fcc_txbuffer[0] = (0x00); 
	    	fcc_txbuffer[1] = (0x10); 
		fcc_txbuffer[4] = (0x0A);
	}

	if (transmit_mode == 4)
	{
		/* for fcc transmit 01 pattern */
		for (i=8; i < 64; i++)
		{
		    fcc_txbuffer[i] = 0xaa;
		}
	}
	else
	{
		for (i=8; i < 64; i++)
		{
		    fcc_txbuffer[i] = (unsigned char) 0xaa;//rand();
		}
	}
	/* copy to transmit buffer */
	//HWMemcpy (HW_TX_BUFFER, fcc_txbuffer, 64);
	HWWriteFifo (&fcc_txbuffer, 16);
	
}



void MacUpdateBuffer(unsigned int transmit_mode, unsigned int transmit_rate)
{
	int i;
	/* WARNING */
	unsigned int aux;/* unsigned long *aux */
	
	
	const unsigned int auxArray[12][2] = 
		{
			{0x00001000, 0x0000000A}, /*1mpbs*/
  			{0x00001000, 0x00000014}, /*2mbps*/
    		 	{0x00001000, 0x00000037}, /*5.5mbps*/
     			{0x00001000, 0x0000006E}, /*11mbps*/
  			/*** OFMD ***/
			{0x000010EE, 0x0000000B}, /*6mbps*/
			{0x000010EE, 0x0000000F}, /*9mbps*/
			{0x000010EE, 0x0000000A}, /*12mbps*/
			{0x000010EE, 0x0000000E}, /*18mbps*/
			{0x000010EE, 0x00000009}, /*24mbps*/
			{0x000010EE, 0x0000000D}, /*36mbps*/
  			{0x000010EE, 0x00000008}, /*48mbps*/
     			{0x000010EE, 0x0000000C} /*54mbps*/
		};
	
	__write32 (HW_GEN_CONTROL, GEN_TXHOLD, OR);
	
	aux = auxArray[transmit_rate][0];
	ByteSwap(&aux);
	__write32 (HW_DATA_FIFO, aux, WRITE);

	aux = auxArray[transmit_rate][1];
	ByteSwap(&aux);
	__write32 (HW_DATA_FIFO, aux, WRITE);
	

	if (transmit_mode == 3) // Before 4
	{
		/* for fcc transmitmacStats 01 pattern */
		for (i=0; i < 14; i++)
		{
			__write32 (HW_DATA_FIFO, 0xAAAAAAAA, WRITE);
		}
	}
	else
	{
		for (i=0; i < 14; i++)
		{
			__write32 (HW_DATA_FIFO, 0xAAAAAAAA, WRITE); // This should be random
		}
	}
	
	__write32 (HW_GEN_CONTROL, ~GEN_TXHOLD, AND);
}

//
// This function puts the driver into continuous transfer mode.  When doing
// so it attempts to shutdown normal driver functionality so the driver
// will not interfere with the continuous transfer.  However I do not think
// the process of shutting down the driver while enabling continous transfer
// mode is completely void of interference, but I think it will work well
// enough for FCC testing.
//
void MacContinuousTransmit(int enable, unsigned int transmit_mode, unsigned int transmit_rate)
{
	//int level;
		
	const unsigned int tx_mode_on[4] = {
					 	0x30100000 , /* Random */
						0x30300000 , /* Zeros */
					  	0x30500000 , /* Ones */
					 	0x30700000   /* DC Baseband */
					    };
					 
	const unsigned int tx_mode_off[4] = {
					 	0x30000000 , /* Random */
						0x30200000 , /* Zeros */
					  	0x30400000 , /* Ones */
					 	0x30000000   /* DC Baseband */
					     }; 
	
	/* disable interrupts host */
	if (enable)
	{
		MacUpdateBuffer(transmit_mode, transmit_rate);

		/* disable mac interrupts */
		__write32(HW_INTR_MASK, 0, WRITE);
	     		
		// Disable IBSS mode
		__write32(HW_MAC_CONTROL, ~(CTRL_IBSS|CTRL_BEACONTX), AND);        

		if(transmit_mode == 3)
			//enable test mode of operation
		  	//HW_GEN_CONTROL = 0x93780001;
		 	// HW_GEN_CONTROL |= GEN_TESTMODE;
		 	__write32(HW_CONF1, TX_CTL, OR); /* Piper */
		
		/* set to continuous transmit; test bit too */
		if (transmit_mode >= 0 && transmit_mode <= 3) // before until 4
			__write32(HW_GEN_STATUS, tx_mode_on[transmit_mode], WRITE);
		else
			__write32(HW_GEN_STATUS, tx_mode_on[0], WRITE);
			  
		/* start transmiting */
		__write32(HW_MAC_CONTROL, CTRL_TXREQ, OR);
	}
	else
	{
		if (transmit_mode >= 0 && transmit_mode <= 4)
			__write32(HW_GEN_STATUS, tx_mode_off[transmit_mode], WRITE);
		else
			__write32(HW_GEN_STATUS, tx_mode_off[0], WRITE);
		
		if(transmit_mode == 3)
			/* Disable test mode of operation */
		 	__write32(HW_CONF1, ~TX_CTL, AND);
				
		/* enable common mac interrupts */
		__write32(HW_INTR_MASK, INTR_RXFIFO|INTR_TXEND|INTR_TIMEOUT|INTR_ABORT, OR);			
		
		__write32(HW_MAC_CONTROL, ~CTRL_TXREQ, AND);
	}
	/* enable interrupts host */
}

/* Set the power directly in the RF transceiver chip for RF testing purposes */
void MacSetDirectTxPower(int value)
{
	if (value < 0x40)
    		WriteRF(11, 0x08040 | value);
	else
    		WriteRF(11, 0x08040 | 0x3f);
}

void MacRadioRXTest(int enable)
{
    	/* disable interrupts */
	if (enable)
    	{
        	/* disable the mac driver */
        	SetPromiscuousMode(TRUE);   
         	MacSetQuiet(1); 
     	}
    	else
	{
        	SetPromiscuousMode(FALSE);
                MacSetQuiet(0);    
    	}//HW_MAC_CONTROL &= ~CTRL_PROMISC;
    	/* enable interrupts */
}

//
// This function is used to put the mac driver in a quiet mode where
// no transmitting is done, but the FPGA is still running.  It is my
// hope that by disabling interrupts the FPGA will not do any automatic
// acking of received data.
//
void MacSetQuiet(int enable_quiet)
{
    /* disable interrupts */

    if (enable_quiet)
    {
        /* disable mac interrupts */
        __write32(HW_INTR_MASK, 0, WRITE);
    }
    else
    {
        /* enable common mac interrupts */
        __write32(HW_INTR_MASK, INTR_RXEND|INTR_TXEND|INTR_TIMEOUT|INTR_ABORT, OR);

    }
    /* enable interrupts */
}

void SetPromiscuousMode(boolean enable)
{
	if (enable == TRUE)    // Enable PROMISC mode
    	{
		__write32(HW_MAC_CONTROL, CTRL_PROMISC, OR); // It was cancelled before.
	}
	else    //disable
    	{
		__write32(HW_MAC_CONTROL, ~CTRL_PROMISC, AND);
	}
}

void MacRadioPrepareTXBeacon(unsigned int tx_frame_period)
{
	/* set frame interval to tx_frame_period */ 
	__write32(HW_CFP_ATIM, (tx_frame_period << 16), WRITE);
    
	/* enable beacon transmission */ 
	__write32(HW_GEN_CONTROL, GEN_BEACEN, OR);
}

//
// Set BSS mode and IDs
// @param bssCaps BSS capabilities, OR of CAP_xxx
// @param bssid ID of BSS to join or start
// @param ssid Service set ID
// @param ssid_len Service set ID length
// @param basic Basic rates, OR of WLN_RATE_xxx
// @param atim IBSS ATIM window size in TU
//
void SetBSS (int bssCaps, uint8 *ssid, int ssid_len, uint16 basic, int atim)
{
	//unsigned char todelete1[4] = {0x00,0x13,0x1A,0x9F,0xDE,0xE0,0x00,0x00};
	//MacFrameHeader MacToSend;
	BeaconFrame BeaconToSend;
	
	static unsigned long todelete = 0; 
	//unsigned long aux;
	unsigned int aux2;
	
	//unsigned char todelete1[4] = {0x00, 0x00, 0xE0, 0xDE, 0x9F, 0x1A, 0x13, 0x00};
	// Set BSSID in hardware
	//HWMemcpy (HW_BSSID0, todelete1, 6);
	//__write32(HW_BSSID0, 0x00010203, WRITE); 
	//__write32(HW_BSSID1, 0x04050000, WRITE);
	
	//aux2 = 0xFFFFFFFF;
	//ByteSwap(&aux2);
	__write32(HW_BSSID0, 0xFFFFFFFF, WRITE);
	//aux2 = 0xFFFF0000;
	//ByteSwap(&aux2); 
	__write32(HW_BSSID1, 0xFFFF0000, WRITE);
	
	
	// Set SSID and basic rates in hardware
	//HWMemcpy (HW_SSID, 0x0a, 1);
	__write32(HW_SSID, 0x0a, WRITE);
	//aux2 = 0x00010203;
	//ByteSwap(&aux2);
	__write32(HW_STAID0, 0x00010203, WRITE);
	//aux2 = 0x04050000;
	//ByteSwap(&aux2); 
	__write32(HW_STAID1, 0x04050000, WRITE);

/*	HW_SSID_LEN = ssid_len |
		((basic & RATE_MASK_OFDM) << 20) |
		((basic & (RATE_MASK_PSK|RATE_MASK_CCK)) << 16);*/
	//__write32(HW_SSID_LEN, ssid_len | ((basic & RATE_MASK_OFDM) << 20) | ((basic & (RATE_MASK_PSK|RATE_MASK_CCK)) << 16), WRITE);
	__write32(HW_SSID_LEN, 0xFF0F000A, OR);
	__write32(HW_SSID, 0x33333333, WRITE);
	__write32(HW_SSID+1, 0x33343132, WRITE);
	__write32(HW_SSID+2, 0x34360000, WRITE);
	//__write32(HW_SSID+8, 0x00003333, WRITE);
	
	//_write32(HW_REMAIN_BO, 100, WRITE);
	__write32(HW_BEACON_BO, 1000, WRITE);
		
	// IBSS mode
	if (bssCaps)
	{
		// If starting IBSS, set beacon and ATIM intervals
		//HW_CFP_ATIM = atim | (BEACON_INT << 16);
		__write32(HW_CFP_ATIM, 1000 | (100 << 16), WRITE);
//		MacRadioPrepareTXBeacon(atim);

		// Write beacon frame to beacon buffer
		if (1)//bcnframe
		{
			__write32(HW_GEN_CONTROL, GEN_BEACEN, OR);
			__write32 (HW_GEN_CONTROL, GEN_TXHOLD, OR);
			
			aux2 = SET_TXFRAMEHEADER(1, 0xee);//0x00
			ByteSwap(&aux2);
			//aux2 = 0xee000000;
			//BeaconToSend.frameHdr = aux2;
			__write32 (HW_DATA_FIFO, aux2, WRITE);
			
			//aux2 = SET_PSKCCKHEADER(0, 0x26, 0x6E);
			aux2 = 0x0026006e; // works
			aux2 = 0x01A0000A; // works
			aux2 = 0x00540037; // works
			aux2 = 0x00D00014; // works
			
			aux2 = SET_PSKOFDMHEADER(0, 0x400, 0xC);//13*4
			
			//printf("FC.Protocol: 0x%08x\n", aux2);
			ByteSwap(&aux2);			
			//aux2 = 0x8c600000;
			//aux2 = 0x03;
			//printf("FC.Protocol: 0x%08x\n", aux2);
			//deserializer(&aux2);
			//ByteSwap(&aux2);
			//printf("FC.Protocol: 0x%08x\n", aux2);
			BeaconToSend.plcpHdr = aux2;
			__write32 (HW_DATA_FIFO, aux2, WRITE);
			
			//aux2 = SET_FC_DUR(0, 0, 0, 0, 0, 0, 0, 0, 0, 0x8, 0, 0);
			//printf("FC.Protocol: 0x%08x\n", aux2);
			
			aux2 = 0x00000080;
			ByteSwap(&aux2);
			BeaconToSend.fc_duration = aux2;
			__write32 (HW_DATA_FIFO, aux2, WRITE);
			
			aux2 = 0xFFFFFFFF;
			//ByteSwap(&aux2 );
			BeaconToSend.addr1 = aux2;
			__write32 (HW_DATA_FIFO, aux2, WRITE);
			
			aux2 = SET_MAC1_MAC2 (0xFFFF, 0x0001);
			aux2 = 0xFFFF0001;
			//ByteSwap(&aux2);
			BeaconToSend.addr1_2 = aux2;
			__write32 (HW_DATA_FIFO, aux2, WRITE);
			
			aux2 = 0x02030405;
			//ByteSwap(&aux2);
			BeaconToSend.addr2   = aux2;
			__write32 (HW_DATA_FIFO, aux2, WRITE);
			
			aux2 = 0x00010203;
			//ByteSwap(&aux2);
			BeaconToSend.addr3 = aux2;
			__write32 (HW_DATA_FIFO, aux2, WRITE);
			
			aux2 = SET_MAC2_MAC3 (0x0405, 0x0000);
			aux2 = 0x04050000;
			//ByteSwap(&aux2);
			BeaconToSend.addr3_squ = aux2;
			__write32 (HW_DATA_FIFO, aux2, WRITE);
			
			//BeaconToSend.macHdr = MacToSend;
			BeaconToSend.timestamp[0] = todelete;
			__write32 (HW_DATA_FIFO, todelete, WRITE);
			BeaconToSend.timestamp[1] = 0;
			__write32 (HW_DATA_FIFO, 0, WRITE);
			aux2 = 0x00010200;
			//ByteSwap(&aux2);
			//BeaconToSend.interval_capinfo = aux2;
			__write32 (HW_DATA_FIFO, aux2, WRITE);
			
			__write32 (HW_DATA_FIFO, 0x000A3333, WRITE);
			//BeaconToSend.nameSSID[0] = 0x0A0B0C0D;
			//__write32 (HW_DATA_FIFO, 0x04, WRITE);
			//BeaconToSend.nameSSID[4] = 0x0E;
			__write32 (HW_DATA_FIFO, 0x33333334, WRITE);
			__write32 (HW_DATA_FIFO, 0x31323436, WRITE);
			
			//__write32 (HW_DATA_FIFO, 0x000A3333, WRITE);
			//BeaconToSend.nameSSID[0] = 0x0A0B0C0D;
			//__write32 (HW_DATA_FIFO, 0x04, WRITE);
			
			//HWWriteFifo (&(BeaconToSend), (sizeof(BeaconFrame)-4)/4);
			//MacUpdateFccBuffer(0,11);
			
			__write32 (HW_GEN_CONTROL, ~GEN_TXHOLD, AND);
			__write32(HW_GEN_CONTROL, ~GEN_BEACEN, AND);
			
			//MacUpdateFccBuffer(0,11);
			//todelete++;
		}

		// Set interrupt mask to enable TBTT and ATIM interrupts
		__write32(HW_INTR_STATUS, INTR_TBTT|INTR_ATIM, OR);//WRITE
		
		//intrMask |= INTR_TBTT|INTR_ATIM;      // enable
		//HW_INTR_MASK = intrMask;
		__write32(HW_INTR_MASK, INTR_TBTT|INTR_ATIM, OR);
		
		// Enable IBSS mode
		__write32(HW_MAC_CONTROL, CTRL_IBSS|CTRL_BEACONTX, OR);
		//__write32(HW_MAC_CONTROL, ~0x2, AND);
	}

	// ESS mode
	else
	{
		// Set interrupt mask to disable TBTT and ATIM interrupts
		//intrMask &= ~(INTR_TBTT|INTR_ATIM); // disable
		//HW_INTR_MASK = intrMask;
		__write32(HW_INTR_MASK, ~(INTR_TBTT|INTR_ATIM), AND);

		// Disable IBSS mode
		//HW_MAC_CONTROL &= ~(CTRL_IBSS|CTRL_BEACONTX);
		__write32(HW_MAC_CONTROL, ~(CTRL_IBSS|CTRL_BEACONTX), AND);
	}
}

void MacRadioTXPeriodic (unsigned char active, unsigned int transmit_mode, unsigned int transmit_rate, int atim, int beacon, unsigned char frame_length)
{
	unsigned int aux;
	unsigned int loop;
	
	/*1024 bytes packet*/
	const unsigned int aux1024ByPacket[12][2] = 
				{
					/* Received frame header, PLCP frame*/
					{0x00010000, 0x2000040A}, /*1mpbs*/
     					{0x00010000, 0x10000014}, /*2mbps*/
					{0x00010000, 0x06610437}, /*5.5mbps*/
					{0x00010000, 0x02E8046E}, /*11mbps*/
					/*** OFMD ***/
					{0x000100EE, 0x0000800B}, /*6mbps*/
					{0x000100EE, 0x0000800F}, /*9mbps*/
					{0x000100EE, 0x0000800A}, /*12mbps*/
					{0x000100EE, 0x0000800E}, /*18mbps*/
					{0x000100EE, 0x00008009}, /*24mbps*/
					{0x000100EE, 0x0000800D}, /*36mbps*/
					{0x000100EE, 0x00008008}, /*48mbps*/
					{0x000100EE, 0x0000800C} /*54mbps*/
				};

	const unsigned int aux128ByPacket[12][2] = 
				{
					/* Received frame header, PLCP frame*/
					{0x00002000, 0x03E8040A}, /*1mpbs*/
					{0x00002000, 0x01F40414}, /*2mbps*/
					{0x00002000, 0x00B5D437}, /*5.5mbps*/
					{0x00002000, 0x005AE46E}, /*11mbps*/
					/*** OFMD ***/
					{0x000020EE, 0x0000100B}, /*6mbps*/
					{0x000020EE, 0x0000100F}, /*9mbps*/
					{0x000020EE, 0x0000100A}, /*12mbps*/
					{0x000020EE, 0x0000100E}, /*18mbps*/
					{0x000020EE, 0x00001009}, /*24mbps*/
					{0x000020EE, 0x0000100D}, /*36mbps*/
					{0x000020EE, 0x00001008}, /*48mbps*/
     					{0x000020EE, 0x0000100C} /*54mbps*/
				};
				
	// Set BSSID in hardware
	// Does not filter any stations.
	__write32(HW_BSSID0, 0xFFFFFFFF, WRITE);
	__write32(HW_BSSID1, 0xFFFF0000, WRITE);
	
	// Set SSID and basic rates in hardware
	// SSID Lenght 10 octects.
	__write32(HW_SSID, 0x0a, WRITE);

	// MAC Address => 01:02:03:04:05
	__write32(HW_STAID0, 0x00010203, WRITE);
	__write32(HW_STAID1, 0x04050000, WRITE);

	// Basic rates are all PSK/CCK and all OFDM
	__write32(HW_SSID_LEN, 0xFF0F000A, OR);
	
	// SSID name, just to fill it with something.
	__write32(HW_SSID, 0x33333333, WRITE);
	__write32(HW_SSID+1, 0x33343132, WRITE);
	__write32(HW_SSID+2, 0x34360000, WRITE);
		
	//__write32(HW_BEACON_BO, 1000, WRITE);
		
	// IBSS mode
	if (active == 1)
	{
		// If starting IBSS, set beacon and ATIM intervals
		__write32(HW_CFP_ATIM, (beacon << 16), WRITE);//atim | (beacon << 16)

		// Write beacon frame to beacon buffer
		//__write32(HW_GEN_CONTROL, GEN_BEACEN, OR);
		__write32 (HW_GEN_CONTROL, GEN_TXHOLD, OR);
		
		
		/********************************************************/
		/* It looks like the Beacon buffer is limted to 128 bytes.
		   If you try to go over that limitation, the device will
		   directly not transmit. If we transmit more than 128 bytes
		   using the Beacon buffer, what we transfer after that
		   limted amount of data is the data buffer itself, bit the
		   the beacon buffer. So, in order to modifiy this buffer, first
		   we should access the data buffer, modifiy the required data
		   and then access the beacon buffer. */
		
		aux = aux1024ByPacket[transmit_rate][0];
		ByteSwap(&aux);
		__write32 (HW_DATA_FIFO, aux, WRITE);
			
		aux = aux1024ByPacket[transmit_rate][1];
		ByteSwap(&aux);
		__write32 (HW_DATA_FIFO, aux, WRITE);
		
		for (loop=256; loop != 0; loop--)//249
		{
			__write32 (HW_DATA_FIFO, 0, WRITE);
		}
		
		/********************************************************/
		__write32(HW_GEN_CONTROL, GEN_BEACEN, OR);
		
		if (frame_length == SIZE1024BYTES)
		{
			aux = aux1024ByPacket[transmit_rate][0];
			ByteSwap(&aux);
			__write32 (HW_DATA_FIFO, aux, WRITE);
			
			aux = aux1024ByPacket[transmit_rate][1];
			ByteSwap(&aux);
			__write32 (HW_DATA_FIFO, aux, WRITE);
		}
		else
		{
			aux = aux128ByPacket[transmit_rate][0];
			ByteSwap(&aux);
			__write32 (HW_DATA_FIFO, aux, WRITE);
			
			aux = aux128ByPacket[transmit_rate][1];
			ByteSwap(&aux);
			__write32 (HW_DATA_FIFO, aux, WRITE);
		}

		// Beacon frame: 0x80
		
		// Data frame.
		aux = 0x00000008;
		ByteSwap(&aux);
		__write32 (HW_DATA_FIFO, aux, WRITE);
			
		// MAC1, MAC2, MAC3 from MAC FRAME
		__write32 (HW_DATA_FIFO, 0xFFFFFFFF, WRITE);
		__write32 (HW_DATA_FIFO, 0xFFFF0001, WRITE);
		__write32 (HW_DATA_FIFO, 0x02030405, WRITE);
		__write32 (HW_DATA_FIFO, 0x00010203, WRITE);
		__write32 (HW_DATA_FIFO, 0x04050000, WRITE);
			
		// Timestamp
		/*__write32 (HW_DATA_FIFO, 0, WRITE);
		__write32 (HW_DATA_FIFO, 0, WRITE);*/
		
		// Cap_Info
		/*__write32 (HW_DATA_FIFO, 0x00010200, WRITE);*/
			
		// SSID
		/*__write32 (HW_DATA_FIFO, 0x000A3333, WRITE);
		__write32 (HW_DATA_FIFO, 0x33333334, WRITE);
		__write32 (HW_DATA_FIFO, 0x31323436, WRITE);*/
			
		// Filling up with something.
		if (frame_length == SIZE1024BYTES)
		{
			for (loop=24; loop != 0; loop--)//249
			{
				__write32 (HW_DATA_FIFO, 0, WRITE);
			}
		}
		else
		{
			for (loop=24; loop != 0; loop--)//33
			{
				__write32 (HW_DATA_FIFO, 0, WRITE);
			}
		}
		
		__write32 (HW_GEN_CONTROL, ~GEN_TXHOLD, AND);
		__write32(HW_GEN_CONTROL, ~GEN_BEACEN, AND);
	
		/////// TESTING !!!!!!!
		// Set interrupt mask to enable TBTT and ATIM interrupts
		/*__write32(HW_INTR_STATUS, INTR_TBTT|INTR_ATIM, OR);//WRITE
		__write32(HW_INTR_MASK, INTR_TBTT|INTR_ATIM, OR);*/
		__write32(HW_INTR_MASK, 0, WRITE);
		
		// Enable IBSS mode
		__write32(HW_MAC_CONTROL, CTRL_IBSS|CTRL_BEACONTX, OR);
	}

	// ESS mode
	else
	{
		// Set interrupt mask to disable TBTT and ATIM interrupts
		__write32(HW_INTR_MASK, ~(INTR_TBTT|INTR_ATIM), AND);

		// Disable IBSS mode
		__write32(HW_MAC_CONTROL, ~(CTRL_IBSS|CTRL_BEACONTX), AND);
	}
}


void PrintMacHeader(MacFrameHeader *aux)
{
#if 0
	//MacFrameHeader *aux = &(aux1->macHdr); 
	/* FRAME Info */
	printf("FH MacRXTestmodType: 0x%02x\n", GET_MODTYP(aux->frameHdr));
	printf("FH ant: 0x%01x\n",GET_ANTENNA( aux->frameHdr));
	printf("FH rssiLNA: 0x%01x\n", GET_RSSILNA(aux->frameHdr));
	printf("FH rssiVGA: 0x%01x\n", GET_RSSIVGA(aux->frameHdr));
	printf("FH freqOff: 0x%04x\n", GET_FREQOFF(aux->frameHdr));
	
	if (GET_MODTYP(aux->frameHdr) == 0xEE)
	{
		/* OFDM / PLCP Info */
		printf("PLCP length: 0x%04x\n", GET_OFDMHEADER_LENGTH(aux->plcpHdr));
		printf("PLCP rate: 0x%02x\n", GET_OFDMHEADER_RATE(aux->plcpHdr));
		printf("PLCP parity: 0x%01x\n", GET_OFDMHEADER_PARITY(aux->plcpHdr));
	}
	else
	{
		/* PSK / PLCP Info */
		printf("PLCP length: 0x%02x\n", GET_PSKHEADER_LENGTH(aux->plcpHdr));
		printf("PLCP service: 0x%02x\n", GET_PSKHEADER_SERVICE(aux->plcpHdr));
		printf("PLCP signal: 0x%04x\n", GET_PSKHEADER_SIGNAL(aux->plcpHdr));	
	}
		
	/* MAC Info */
	printf("FC.Protocol: 0x%08x\n", GET_FC_DUR_PROTOCOL(aux->macHdr.fc_duration));
	printf("FC.type:     0x%08x\n", GET_FC_DUR_TYPE((aux->macHdr.fc_duration)));
	printf("FC.subtype:  0x%08x\n", GET_FC_DUR_SUBTYPE(aux->macHdr.fc_duration));
	printf("FC.order:    0x%08x\n", GET_FC_DUR_ORDER((aux->macHdr.fc_duration)));
	printf("FC._prot:    0x%08x\n", GET_FC_DUR_PROTECTEDFRAME((aux->macHdr.fc_duration)));
	printf("FC.moredata: 0x%08x\n", GET_FC_DUR_MOREDATA((aux->macHdr.fc_duration)));
	printf("FC.pwrMgt:   0x%08x\n", GET_FC_DUR_PWRMGMT((aux->macHdr.fc_duration)));
	printf("FC.retry:    0x%08x\n", GET_FC_DUR_RETRY((aux->macHdr.fc_duration)));
	printf("FC.moreFrag: 0x%08x\n", GET_FC_DUR_MOREFRAG((aux->macHdr.fc_duration)));
	printf("FC.fromDS:   0x%08x\n", GET_FC_DUR_FROMDS((aux->macHdr.fc_duration)));
	printf("FC.toDS:     0x%08x\n", GET_FC_DUR_TODS((aux->macHdr.fc_duration)));
	
	printf("Duration:    0x%08x\n", GET_FC_DUR_DURATION(aux->macHdr.fc_duration));

	deserializer(&(aux->macHdr.addr1));
	deserializer(&(aux->macHdr.addr1_2));
	deserializer(&(aux->macHdr.addr2));
	deserializer(&(aux->macHdr.addr3));
	deserializer(&(aux->macHdr.addr3_squ));

	printf("MAC1: %08x%04x\n", aux->macHdr.addr1, GET_PART_MAC1(aux->macHdr.addr1_2));
	printf("MAC2: %04x%08x\n", GET_PART_MAC2(aux->macHdr.addr1_2), aux->macHdr.addr2);
	printf("MAC3: %08x%04x\n", aux->macHdr.addr3, GET_PART_MAC3(aux->macHdr.addr3_squ));
	
	//printf("SQU: %08x\n", aux->macHdr.squ.sq16);

#endif
}
void deserializer(unsigned long *value)
{
	unsigned long aux = 0;
	unsigned int i;
	unsigned char * charAux1 = (unsigned char *) value;
	unsigned char * charAux2 = (unsigned char *) &aux;
	
	for (i=0; i < 4; i++)
	{
		charAux2[i] = charAux1[3 - i];		
	}
	
	*value = aux;
}

void MacRXTest (unsigned long *numRxFrames)
{
	unsigned int aux;
	/* unsigned long aux2; */
	
	*numRxFrames = 0;
	
	// Clean buffer.
	__write32(HW_GEN_CONTROL, GEN_RXFIFORST, OR);
	__write32(HW_GEN_CONTROL, ~GEN_RXFIFORST, AND);
	
	waitUS(10);
	
	__write32(HW_INTR_MASK, 0x1, OR);
	
	// Clear interrupts
	__write32(HW_INTR_STATUS, 0, WRITE);
	
	do
	{
		__read32 (HW_INTR_STATUS, &aux);	
		if ( (aux & 0x01) == 0x01)
		{	
			//__read32 (HW_MAC_STATUS, &aux);
			//if ((aux & 0x04) == 0x04)
				(*numRxFrames)++;
			//__write32 (HW_MAC_STATUS, ~0x00000004, AND);
		}
		// Clean buffer.
		__write32(HW_GEN_CONTROL, GEN_RXFIFORST, OR);
		__write32(HW_GEN_CONTROL, ~GEN_RXFIFORST, AND);
	
		// Clear interrupts
		__write32(HW_INTR_STATUS, 0, OR);
	} while (!tstc());
	
	getc();
}

inline void rxBeforeLoop(unsigned int *rxFrames)
{
	/* Frame counter */
	*rxFrames = 0;
	
	/* Force max. RX sensitivity */
	__write32(HW_OUT_CTRL, 0x0000F000, OR);
	
	/* Force min. RX sensitivity */
	//__write32(HW_OUT_CTRL, 0x0000A000, OR);
	
	/* Force antenna 1*/
	__write32(HW_GEN_CONTROL, 0x00000004, OR);
	
	waitUS(1);
	
	/* Reset FIFO */
	__write32(HW_GEN_CONTROL, GEN_RXFIFORST, OR);
	waitUS(1);
	__write32(HW_GEN_CONTROL, ~GEN_RXFIFORST, AND);
	waitUS(100);
}

inline void rxLoopExecution(unsigned int *rxFrames)
{
	unsigned long length, i;
	unsigned int aux, intrStatus;

	__read32 (HW_INTR_STATUS, &intrStatus);	
		
	if ( (intrStatus & 0x1) == 0x1)
	{	
		__read32 (HW_DATA_FIFO, &aux);
		
		if ((aux & 0xFF000000) == 0xEE000000)
		{
			__read32 (HW_DATA_FIFO, &aux);
			ByteSwap(&aux);
				
			length = GET_OFDMHEADER_LENGTH(aux);
					
			for (i=length/4; i>0; i--)
				__read32 (HW_DATA_FIFO, &aux);
			
			(*rxFrames)++;
		}
		else if ((aux & 0xFF000000) == 0x00000000)
		{
			__read32 (HW_DATA_FIFO, &aux);
			ByteSwap(&aux);
			length = (aux & 0xFFFF0000) >> 16;
			switch (aux & 0x000000FF)
			{	
				case 0xA:
					length /= 8;
				break;
				case 0x14:
					length /= 4;
				break;
				case 0x37:
					length = (11 * length)/16;
				break;
				case 0x6E:
					length = (11 * length)/8;
				break;
			}
			
			for (i=length/4; i>0; i--)
				__read32 (HW_DATA_FIFO, &aux);
			
			(*rxFrames)++;
		}
	}	
		// Clear interrupts
		__write32(HW_INTR_STATUS, 0xF, WRITE);
}

inline void rxAfterLoop(void)
{
	/* Force antenna 1*/
	__write32(HW_GEN_CONTROL, ~0x00000004, AND);
}

inline void MacReadPacketFromBuffer(unsigned int *rxFrames)
{
	unsigned long length, i;
	unsigned int aux, intrStatus;
	
	// Received a frame

	*rxFrames = 0;
	
	/* Force max. RX sensitivity */
	//__write32(HW_OUT_CTRL, 0x0000F000, OR);
	
	/* Force min. RX sensitivity */
	//__write32(HW_OUT_CTRL, 0x0000A000, OR);
	
	/* Force antenna 1*/
	__write32(HW_GEN_CONTROL, 0x00000004, OR);
	
	waitUS(1);
	
	__write32(HW_GEN_CONTROL, GEN_RXFIFORST, OR);
	waitUS(1);
	__write32(HW_GEN_CONTROL, ~GEN_RXFIFORST, AND);
	waitUS(100);
	
	do
	{
		__read32 (HW_INTR_STATUS, &intrStatus);	
		
		if ( (intrStatus & 0x1) == 0x1)
		{	
			__read32 (HW_DATA_FIFO, &aux);
			
			if ((aux & 0xFF000000) == 0xEE000000)
			{
				__read32 (HW_DATA_FIFO, &aux);
				ByteSwap(&aux);
						
				length = GET_OFDMHEADER_LENGTH(aux);
						
				for (i=length/4; i>0; i--)
					__read32 (HW_DATA_FIFO, &aux);
				
				(*rxFrames)++;
			}
			else if ((aux & 0xFF000000) == 0x00000000)
			{
				__read32 (HW_DATA_FIFO, &aux);
				ByteSwap(&aux);
				length = (aux & 0xFFFF0000) >> 16;
				switch (aux & 0x000000FF)
				{	
					case 0xA:
						length /= 8;
					break;
					case 0x14:
						length /= 4;
					break;
					case 0x37:
						length = (11 * length)/16;
					break;
					case 0x6E:
						length = (11 * length)/8;
					break;
				}
				
				for (i=length/4; i>0; i--)
					__read32 (HW_DATA_FIFO, &aux);
				
				(*rxFrames)++;
			}

		}
		
		// Clear interrupts
		__write32(HW_INTR_STATUS, 0xF, WRITE);
		
	} while (!tstc());
	
	/* Force antenna 1*/
	__write32(HW_GEN_CONTROL, ~0x00000004, AND);
	
	// Flush buffer from last character.
	getc();
}

void ConstRead(void)
{
	static int k=0;
	/* static int  p=0; */
	//static unsigned long superbuffer[100];
	
	// Get interrupt status
	uint32 intrStatus, rxFrames;
	//uint32 aux2, aux3, aux4;
	//unsigned char i;
	
	//MacFrameHeader aux;
	
	rxFrames = 0;
	
	//intrStatus &= intrMask;
	// Received a frame
	while (1)
	{
		__read32 (HW_INTR_STATUS, &intrStatus);	
		if ( (intrStatus & 0x01) == 0x01)
		{	
#if 0			
			k=0;
			__read32(HW_MAC_STATUS, &aux3);
			__read32(HW_MAC_CONTROL, &aux4);
			do {
				__write32 (HW_INTR_STATUS, 1, OR);
				__read32 (HW_DATA_FIFO, (superbuffer + k));
				if (k>0)
				{
					ByteSwap(superbuffer + k);
				}
				k++;
				if (k > 99) break;//> sizeof (MacFrameHeader)) break;
				__read32 (HW_GEN_STATUS, &aux2);
																
			}while ((aux2 & 0x10) != 0x10);
			// Process the frame
			//HandleRxEnd (&rxFrame);
#endif			
			__write32(HW_GEN_CONTROL, GEN_RXFIFORST, OR);
			__write32(HW_GEN_CONTROL, ~GEN_RXFIFORST, AND);
			rxFrames++;
	
			// Clear interrupts
			__write32(HW_INTR_STATUS, 0, WRITE);
			//PrintMacHeader(&(aux));
			
		}
		if (k>=0)
		{
#if 0
			printf("\n **** INI FRAME **** \n");
			//printf("k: %08x\n", k);
			p=0;
			//HWMemcpy(&aux, superbuffer, sizeof(aux));
			//if (GET_FC_DUR_TYPE(aux.macHdr.fc_duration) == 0x02 )
			PrintMacHeader((MacFrameHeader *) (superbuffer));
			printf("MAC C: %08x\n", aux4);
			printf("MAC S: %08x\n", aux3);
			
			/*if (aux3 == 0x8)
				printf("DF\n");
			if (aux3 == 0x4)
				printf("BF\n");
			if (aux3 == 0x2)
				printf("GO\n");
			if (aux3 == 0x1)
				printf("NGO\n");*/
			while (p <= k)
			{
				printf("%08x\n", (superbuffer[p]));
				p++;
			}
			printf("\n **** END FRAME **** \n");
#endif
			printf("FRAMES : %u", rxFrames);
			k = -1;
		}
	}
}

#endif /* CFG_HAS_WIRELESS */
