/*
 *	Copyright 1994, 1995, 2000 Neil Russell.
 *	(See License)
 *	Copyright 2000, 2001 DENX Software Engineering, Wolfgang Denk, wd@denx.de
 */

#include <common.h>
#include <command.h>
#include <net.h>
#include "tftp.h"
#include "bootp.h"
#ifdef CONFIG_TFTP_UPDATE_ONTHEFLY
# include "../common/digi/cmd_bsp.h"          	/* shares 6 variables with tftp_direct_to_flash */
# include "../common/digi/cmd_nvram/partition.h"	/* used for PartWrite() , PartVerify() */
# define UBOOT
# include "../common/digi/cmd_nvram/lib/include/nvram.h"
#endif /* CONFIG_TFTP_UPDATE_ONTHEFLY */

#if defined(CONFIG_CMD_NET)

#define WELL_KNOWN_PORT	69		/* Well known TFTP port #		*/
#define TIMEOUT		5000UL		/* Millisecs to timeout for lost pkt */
#ifndef	CONFIG_NET_RETRY_COUNT
# define TIMEOUT_COUNT	10		/* # of timeouts before giving up  */
#else
# define TIMEOUT_COUNT  (CONFIG_NET_RETRY_COUNT * 2)
#endif
					/* (for checking the image size)	*/
#define HASHES_PER_LINE	65		/* Number of "loading" hashes per line	*/

#define printf(args...)	PRINT_INFO(args)
#define puts(args...)	PUTS_INFO(args)

/*
 *	TFTP operations.
 */
#define TFTP_RRQ	1
#define TFTP_WRQ	2
#define TFTP_DATA	3
#define TFTP_ACK	4
#define TFTP_ERROR	5
#define TFTP_OACK	6

static ulong TftpTimeoutMSecs = TIMEOUT;
static int TftpTimeoutCountMax = TIMEOUT_COUNT;
extern int NetSilent;		/* Whether to silence the net commands output */
extern int DownloadingAutoScript;	/* To know if we are trying to download the autoscript */
int TftpErrorCount;		/* The number of erroneous tries */

/*
 * These globals govern the timeout behavior when attempting a connection to a
 * TFTP server. TftpRRQTimeoutMSecs specifies the number of milliseconds to
 * wait for the server to respond to initial connection. Second global,
 * TftpRRQTimeoutCountMax, gives the number of such connection retries.
 * TftpRRQTimeoutCountMax must be non-negative and TftpRRQTimeoutMSecs must be
 * positive. The globals are meant to be set (and restored) by code needing
 * non-standard timeout behavior when initiating a TFTP transfer.
 */
ulong TftpRRQTimeoutMSecs = TIMEOUT;
int TftpRRQTimeoutCountMax = TIMEOUT_COUNT;

#ifdef CONFIG_TFTP_UPDATE_ONTHEFLY
char bTftpToFlashStatus = 0;			/* Signaling flags */
size_t iFlashEraseSize = 0;			/* for TftpHandler; set by cmd_bsp.c */
size_t iFlashPageSize = 0;			/* for TftpHandler; set by cmd_bsp.c */
uint64_t iPartitionStartAdress = 0;		/* Start address of partition to flash; set by cmd_bsp.c */
uint64_t iPartitionSize = 0;			/* Size of partition to flash; set by cmd_bsp.c */
const struct nv_param_part* pPartToWrite;	/* Partition to flash; set by cmd_bsp.c */
uint uiBlocksWrittenToFlash = 0;
ulong ulRamOffset = 0;
ulong ulLastRamAddressWritten = 0;
ulong ulBytesCounter = 0;

#if defined(CONFIG_CMD_UBI)
extern int ubi_volume_off_write(char *volume, void *buf, size_t size, int isFirstPart, int isLastPart);
extern int ubi_volume_verify(char *volume, char *buf, loff_t offset, size_t size, char skipUpdFlagCheck);
#endif

#define FLASH_SECTORS_BUFFERED_IN_RAM	3	/* define # of flash sectors that are buffered before
						 * writing to flash */
#endif /* CONFIG_TFTP_UPDATE_ONTHEFLY */

enum {
	TFTP_ERR_UNDEFINED            = 0,
	TFTP_ERR_FILE_NOT_FOUND       = 1,
	TFTP_ERR_ACCESS_DENIED        = 2,
	TFTP_ERR_DISK_FULL            = 3,
	TFTP_ERR_UNEXPECTED_OPCODE    = 4,
	TFTP_ERR_UNKNOWN_TRANSFER_ID  = 5,
	TFTP_ERR_FILE_ALREADY_EXISTS  = 6,
};

static IPaddr_t TftpServerIP;
static int	TftpServerPort;		/* The UDP port at their end		*/
static int	TftpOurPort;		/* The UDP port at our end		*/
static int	TftpTimeoutCount;
static ulong	TftpBlock;		/* packet sequence number		*/
static ulong	TftpLastBlock;		/* last packet sequence number received */
static ulong	TftpBlockWrap;		/* count of sequence number wraparounds */
static ulong	TftpBlockWrapOffset;	/* memory offset due to wrapping	*/
static int	TftpState;

#define STATE_RRQ	1
#define STATE_DATA	2
#define STATE_TOO_LARGE	3
#define STATE_BAD_MAGIC	4
#define STATE_OACK	5

#define TFTP_BLOCK_SIZE		512		    /* default TFTP block size	*/
#define TFTP_SEQUENCE_SIZE	((ulong)(1<<16))    /* sequence number is 16 bit */

#define DEFAULT_NAME_LEN	(8 + 4 + 1)
static char default_filename[DEFAULT_NAME_LEN];

#ifndef CONFIG_TFTP_FILE_NAME_MAX_LEN
#define MAX_LEN 128
#else
#define MAX_LEN CONFIG_TFTP_FILE_NAME_MAX_LEN
#endif

static char tftp_filename[MAX_LEN];

#ifdef CONFIG_SYS_DIRECT_FLASH_TFTP
extern flash_info_t flash_info[];
#endif

/* 512 is poor choice for ethernet, MTU is typically 1500.
 * Minus eth.hdrs thats 1468.  Can get 2x better throughput with
 * almost-MTU block sizes.  At least try... fall back to 512 if need be.
 */
#define TFTP_MTU_BLOCKSIZE 1468
static unsigned short TftpBlkSize=TFTP_BLOCK_SIZE;
static unsigned short TftpBlkSizeOption=TFTP_MTU_BLOCKSIZE;


#ifdef CONFIG_MCAST_TFTP
#include <malloc.h>
#define MTFTP_BITMAPSIZE	0x1000
static unsigned *Bitmap;
static int PrevBitmapHole,Mapsize=MTFTP_BITMAPSIZE;
static uchar ProhibitMcast=0, MasterClient=0;
static uchar Multicast=0;
extern IPaddr_t Mcast_addr;
static int Mcast_port;
static ulong TftpEndingBlock; /* can get 'last' block before done..*/

static void parse_multicast_oack(char *pkt,int len);

static void
mcast_cleanup(void)
{
	if (Mcast_addr) eth_mcast_join(Mcast_addr, 0);
	if (Bitmap) free(Bitmap);
	Bitmap=NULL;
	Mcast_addr = Multicast = Mcast_port = 0;
	TftpEndingBlock = -1;
}

#endif	/* CONFIG_MCAST_TFTP */

static __inline__ void
store_block (unsigned block, uchar * src, unsigned len)
{
	ulong offset = block * TftpBlkSize + TftpBlockWrapOffset;
	ulong newsize = offset + len;
#ifdef CONFIG_SYS_DIRECT_FLASH_TFTP
	int i, rc = 0;

	for (i=0; i<CONFIG_SYS_MAX_FLASH_BANKS; i++) {
		/* start address in flash? */
		if (flash_info[i].flash_id == FLASH_UNKNOWN)
			continue;
		if (load_addr + offset >= flash_info[i].start[0]) {
			rc = 1;
			break;
		}
	}

	if (rc) { /* Flash is destination for this packet */
		rc = flash_write ((char *)src, (ulong)(load_addr+offset), len);
		if (rc) {
			flash_perror (rc);
			NetState = NETLOOP_FAIL;
			return;
		}
	}
	else
#endif /* CONFIG_SYS_DIRECT_FLASH_TFTP */
	{
		(void)memcpy((void *)(load_addr + offset), src, len);
	}
#ifdef CONFIG_MCAST_TFTP
	if (Multicast)
		ext2_set_bit(block, Bitmap);
#endif

	if (NetBootFileXferSize < newsize)
		NetBootFileXferSize = newsize;
}

#ifdef CONFIG_TFTP_UPDATE_ONTHEFLY
static __inline__ void
store_block_to_ram (ulong ramAddress, uchar * src, unsigned len)
{
	/* count received bytes here to calculate FileSize later */
	ulBytesCounter += len;

	/*copy TftpBlock into RAM buffer*/
	(void)memcpy( (void *)(ramAddress), src, len );

	if( NetBootFileXferSize < ulBytesCounter)
		NetBootFileXferSize = ulBytesCounter;
}

static __inline__ void
store_block_to_flash (void)
{
	ulong offset = uiBlocksWrittenToFlash * iFlashEraseSize;
	ulong tempRamOff = 0;
	int iRes = 0, i;

	for(i = 0; i < FLASH_SECTORS_BUFFERED_IN_RAM; i++){
#if defined(CONFIG_CMD_UBI)
		if (bTftpToFlashStatus & B_PARTITION_IS_UBIFS) {
			iRes = !ubi_volume_off_write((char *)pPartToWrite->szName, (void *)(load_addr + tempRamOff),
					     iFlashEraseSize, uiBlocksWrittenToFlash == 0, 0);
			if (iRes) {
				iRes = !ubi_volume_verify((char *)pPartToWrite->szName, (char *)(load_addr + tempRamOff),
							 offset, iFlashEraseSize, 1);
			}
		}
		else
#endif
		{
			if( PartHasBadBlock( pPartToWrite, iPartitionStartAdress + (uint64_t)offset) ){
				/* skip the bad blocks here, not in PartWrite()
				* because we need to write block after block
				* and need to know if we skiped a block for the next loop */
				offset += iFlashEraseSize;
				uiBlocksWrittenToFlash++;
			}

			/* Write RAM buffer to partition and verify it */
			iRes = PartWrite( pPartToWrite, iPartitionStartAdress + (uint64_t)offset,
					(void *)(load_addr + tempRamOff), iFlashEraseSize, 1);
			iRes = PartVerify(pPartToWrite, iPartitionStartAdress + (uint64_t)offset,
					(void *)(load_addr + tempRamOff), iFlashEraseSize, 1);
		}
		if(!iRes)
			goto error;

		offset += iFlashEraseSize;
		tempRamOff += iFlashEraseSize;
		uiBlocksWrittenToFlash++;
	}

	/* calculate buffer offset (bytes that were received but don't fit into flash sector anymore)*/
	ulRamOffset = ulLastRamAddressWritten - (uint)iFlashEraseSize * FLASH_SECTORS_BUFFERED_IN_RAM ;
	/* then copy them to the start of buffer to flash them in the next loop */
	(void)memcpy( (void *)load_addr,
			(void *)(load_addr + (iFlashEraseSize * FLASH_SECTORS_BUFFERED_IN_RAM)),
			(ulRamOffset - load_addr) );

	/* finally remember the last address of RAM buffer */
	ulLastRamAddressWritten = ulRamOffset;

	return;
error:
	bTftpToFlashStatus |= B_ERROR_DURING_FLASH;
}


static __inline__ void
store_last_block_to_flash (void)
{
	/* we received the last Tftp package, now handle it */
	ulong offset = uiBlocksWrittenToFlash * iFlashEraseSize;
	int iRes = 0;

#if defined(CONFIG_CMD_UBI)
	if (bTftpToFlashStatus & B_PARTITION_IS_UBIFS) {
		size_t size = ulLastRamAddressWritten - load_addr;

		iRes = !ubi_volume_off_write((char *)pPartToWrite->szName, (void *)load_addr,
					    ulLastRamAddressWritten - load_addr, uiBlocksWrittenToFlash == 0, 1);
		if (iRes && (size != 0)) {
			iRes = !ubi_volume_verify((char *)pPartToWrite->szName, (char *)load_addr,
						 offset, size, 1);
		}
	}
	else
#endif
	{
		/* do the padding in the RAM buffer */
		int iFreeBytesInBlock = iFlashPageSize - ( (ulLastRamAddressWritten - load_addr) % iFlashPageSize);
		if( iFreeBytesInBlock > 0 ){
			if( (bTftpToFlashStatus & B_PARTITION_IS_JFFS2) == B_PARTITION_IS_JFFS2 )
				memset( (void *) ulLastRamAddressWritten, 0x0, iFreeBytesInBlock);
			else
				memset( (void *) ulLastRamAddressWritten, 0xff, iFreeBytesInBlock);
			ulLastRamAddressWritten += iFreeBytesInBlock;
		}

		/* then write the last bytes to flash and verify */
		iRes = PartWrite(pPartToWrite, iPartitionStartAdress + (uint64_t)offset,
				(void *)load_addr, ulLastRamAddressWritten - load_addr, 1 );
		iRes = PartVerify(pPartToWrite, iPartitionStartAdress + (uint64_t)offset,
				(void *)load_addr, ulLastRamAddressWritten - load_addr, 1 );
	}
	if(!iRes)
		goto error;

	printf( "\nWriting blocks:   complete                                      " );
	printf( "\nVerifying blocks: complete                                      " );

	return;

error:
	bTftpToFlashStatus |= B_ERROR_DURING_FLASH;
}
#endif /* CONFIG_TFTP_UPDATE_ONTHEFLY */

static void TftpSend (void);
static void TftpTimeout (void);

/**********************************************************************/

static void
TftpSend (void)
{
	volatile uchar *	pkt;
	volatile uchar *	xp;
	int			len = 0;
	volatile ushort *s;

#ifdef CONFIG_MCAST_TFTP
	/* Multicast TFTP.. non-MasterClients do not ACK data. */
	if (Multicast
	 && (TftpState == STATE_DATA)
	 && (MasterClient == 0))
		return;
#endif
	/*
	 *	We will always be sending some sort of packet, so
	 *	cobble together the packet headers now.
	 */
	pkt = NetTxPacket + NetEthHdrSize() + IP_HDR_SIZE;

	switch (TftpState) {

	case STATE_RRQ:
		xp = pkt;
		s = (ushort *)pkt;
		*s++ = htons(TFTP_RRQ);
		pkt = (uchar *)s;
		strcpy ((char *)pkt, tftp_filename);
		pkt += strlen(tftp_filename) + 1;
		strcpy ((char *)pkt, "octet");
		pkt += 5 /*strlen("octet")*/ + 1;
		strcpy ((char *)pkt, "timeout");
		pkt += 7 /*strlen("timeout")*/ + 1;
		sprintf((char *)pkt, "%lu", TIMEOUT / 1000);
		debug("send option \"timeout %s\"\n", (char *)pkt);
		pkt += strlen((char *)pkt) + 1;
		/* try for more effic. blk size */
		pkt += sprintf((char *)pkt,"blksize%c%d%c",
				0,TftpBlkSizeOption,0);
#ifdef CONFIG_MCAST_TFTP
		/* Check all preconditions before even trying the option */
		if (!ProhibitMcast
		 && (Bitmap=malloc(Mapsize))
		 && eth_get_dev()->mcast) {
			free(Bitmap);
			Bitmap=NULL;
			pkt += sprintf((char *)pkt,"multicast%c%c",0,0);
		}
#endif /* CONFIG_MCAST_TFTP */
		len = pkt - xp;
		break;

	case STATE_OACK:
#ifdef CONFIG_MCAST_TFTP
		/* My turn!  Start at where I need blocks I missed.*/
		if (Multicast)
			TftpBlock=ext2_find_next_zero_bit(Bitmap,(Mapsize*8),0);
		/*..falling..*/
#endif
	case STATE_DATA:
		xp = pkt;
		s = (ushort *)pkt;
		*s++ = htons(TFTP_ACK);
		*s++ = htons(TftpBlock);
		pkt = (uchar *)s;
		len = pkt - xp;
		break;

	case STATE_TOO_LARGE:
		xp = pkt;
		s = (ushort *)pkt;
		*s++ = htons(TFTP_ERROR);
		*s++ = htons(3);
		pkt = (uchar *)s;
		strcpy ((char *)pkt, "File too large");
		pkt += 14 /*strlen("File too large")*/ + 1;
		len = pkt - xp;
		break;

	case STATE_BAD_MAGIC:
		xp = pkt;
		s = (ushort *)pkt;
		*s++ = htons(TFTP_ERROR);
		*s++ = htons(2);
		pkt = (uchar *)s;
		strcpy ((char *)pkt, "File has bad magic");
		pkt += 18 /*strlen("File has bad magic")*/ + 1;
		len = pkt - xp;
		break;
	}

	NetSendUDPPacket(NetServerEther, TftpServerIP, TftpServerPort, TftpOurPort, len);
}


static void
TftpHandler (uchar * pkt, unsigned dest, unsigned src, unsigned len)
{
	ushort proto;
	ushort *s;
	int i;

	if (dest != TftpOurPort) {
#ifdef CONFIG_MCAST_TFTP
		if (Multicast
		 && (!Mcast_port || (dest != Mcast_port)))
#endif
		return;
	}
	if (TftpState != STATE_RRQ && src != TftpServerPort) {
		return;
	}

	if (len < 2) {
		return;
	}
	len -= 2;
	/* warning: don't use increment (++) in ntohs() macros!! */
	s = (ushort *)pkt;
	proto = *s++;
	pkt = (uchar *)s;
	switch (ntohs(proto)) {

	case TFTP_RRQ:
	case TFTP_WRQ:
	case TFTP_ACK:
		break;
	default:
		break;

	case TFTP_OACK:
		debug("Got OACK: %s %s\n", pkt, pkt+strlen(pkt)+1);
		TftpState = STATE_OACK;
		TftpServerPort = src;
		/*
		 * Check for 'blksize' option.
		 * Careful: "i" is signed, "len" is unsigned, thus
		 * something like "len-8" may give a *huge* number
		 */
		for (i=0; i+8<len; i++) {
			if (strcmp ((char*)pkt+i,"blksize") == 0) {
				TftpBlkSize = (unsigned short)
					simple_strtoul((char*)pkt+i+8,NULL,10);
				debug ("Blocksize ack: %s, %d\n",
					(char*)pkt+i+8,TftpBlkSize);
				break;
			}
		}
#ifdef CONFIG_MCAST_TFTP
		parse_multicast_oack((char *)pkt,len-1);
		if ((Multicast) && (!MasterClient))
			TftpState = STATE_DATA;	/* passive.. */
		else
#endif
		TftpSend (); /* Send ACK */
		break;
	case TFTP_DATA:
		if (len < 2)
			return;
		len -= 2;
		TftpBlock = ntohs(*(ushort *)pkt);

		/*
		 * RFC1350 specifies that the first data packet will
		 * have sequence number 1. If we receive a sequence
		 * number of 0 this means that there was a wrap
		 * around of the (16 bit) counter.
		 */
		if (TftpBlock == 0) {
			TftpBlockWrap++;
			TftpBlockWrapOffset += TftpBlkSize * TFTP_SEQUENCE_SIZE;
			printf("\n\t %lu MB received\n\t ", TftpBlockWrapOffset>>20);
		} else {
			if (((TftpBlock - 1) % 10) == 0) {
				printf("#");
			} else if ((TftpBlock % (10 * HASHES_PER_LINE)) == 0) {
				printf("\n\t ");
			}
		}

		if (TftpState == STATE_RRQ) {
			debug("Server did not acknowledge timeout option!\n");
		}

		if (TftpState == STATE_RRQ || TftpState == STATE_OACK) {
			/* first block received */
			TftpState = STATE_DATA;
			TftpServerPort = src;
			TftpLastBlock = 0;
			TftpBlockWrap = 0;
			TftpBlockWrapOffset = 0;

#ifdef CONFIG_MCAST_TFTP
			if (Multicast) { /* start!=1 common if mcast */
				TftpLastBlock = TftpBlock - 1;
			} else
#endif
			if (TftpBlock != 1) {	/* Assertion */
				printf("\nTFTP error: "
					"First block is not block 1 (%ld)\n"
					"Starting again\n\n",
					TftpBlock);
				NetStartAgain ();
				break;
			}
		}

		if (TftpBlock == TftpLastBlock) {
			/*
			 *	Same block again; ignore it.
			 */
			break;
		}

		TftpLastBlock = TftpBlock;
		TftpTimeoutMSecs = TIMEOUT;
		TftpTimeoutCountMax = TIMEOUT_COUNT;
		TftpTimeoutCount = 0;	/* Force timeouts to be consecutive */
		NetSetTimeout (TftpTimeoutMSecs, TftpTimeout);

#ifdef CONFIG_TFTP_UPDATE_ONTHEFLY
		if( (bTftpToFlashStatus & B_WRITE_IMG_TO_FLASH) == B_WRITE_IMG_TO_FLASH ){
			/* new TFTP on-the-fly update function:
			 * buffer image in RAM and write it directly to flash */

			if(ulLastRamAddressWritten == 0)
				ulLastRamAddressWritten = load_addr;

			/* capture packets, buffer it into RAM and remember last RAM address*/
			store_block_to_ram(ulLastRamAddressWritten, pkt + 2, len);
			ulLastRamAddressWritten += len;

			if( (load_addr + iFlashEraseSize * FLASH_SECTORS_BUFFERED_IN_RAM) <= ulLastRamAddressWritten){
				/* we received enough packets to write in a flash sector,
				 * so do it unless the partition is full*/
				if( iPartitionSize < (TftpBlkSize * TftpBlock) ){
					printf("\nERROR: Image does not fit into partition!");
					bTftpToFlashStatus &= ~B_WRITE_IMG_TO_FLASH;
					NetState = NETLOOP_FAIL;
					return;
				}
				store_block_to_flash();

				if( (bTftpToFlashStatus & B_ERROR_DURING_FLASH) == B_ERROR_DURING_FLASH ){
					printf("\nERROR: occurred during update of partition at RAM address 0x%lx.\n", ulLastRamAddressWritten);
					ulLastRamAddressWritten = 0;
					uiBlocksWrittenToFlash = 0;
					ulBytesCounter = 0;
					bTftpToFlashStatus &= ~B_PARTITION_IS_JFFS2;
					bTftpToFlashStatus &= ~B_WRITE_IMG_TO_FLASH;
					NetState = NETLOOP_FAIL;
					return;
				}
			}
		} else
#endif /* CONFIG_TFTP_UPDATE_ONTHEFLY */
		{
			/* standard TFTP function: write image to RAM */
			store_block (TftpBlock - 1, pkt + 2, len);
		}

		/*
		 *	Acknowledge the block just received, which will prompt
		 *	the server for the next one.
		 */
#ifdef CONFIG_MCAST_TFTP
		/* if I am the MasterClient, actively calculate what my next
		 * needed block is; else I'm passive; not ACKING
		 */
		if (Multicast) {
			if (len < TftpBlkSize)  {
				TftpEndingBlock = TftpBlock;
			} else if (MasterClient) {
				TftpBlock = PrevBitmapHole =
					ext2_find_next_zero_bit(
						Bitmap,
						(Mapsize*8),
						PrevBitmapHole);
				if (TftpBlock > ((Mapsize*8) - 1)) {
					printf("tftpfile too big\n");
					/* try to double it and retry */
					Mapsize<<=1;
					mcast_cleanup();
					NetStartAgain ();
					return;
				}
				TftpLastBlock = TftpBlock;
			}
		}
#endif
		TftpSend ();

#ifdef CONFIG_MCAST_TFTP
		if (Multicast) {
			if (MasterClient && (TftpBlock >= TftpEndingBlock)) {
				printf("\nMulticast tftp done\n");
				mcast_cleanup();
				NetState = NETLOOP_SUCCESS;
			}
		}
		else
#endif
		if (len < TftpBlkSize) {
			/*
			 *	We received the whole thing.  Try to
			 *	run it.
			 */

#ifdef CONFIG_TFTP_UPDATE_ONTHEFLY
			if( (bTftpToFlashStatus & B_WRITE_IMG_TO_FLASH) == B_WRITE_IMG_TO_FLASH ){
				/* TFTP transfer complete, write last received TftpBlocks to flash */
				store_last_block_to_flash();
				/* and reset counters and flags */
				ulLastRamAddressWritten = 0;
				uiBlocksWrittenToFlash = 0;
				ulBytesCounter = 0;
				ulRamOffset = 0;
				bTftpToFlashStatus &= ~B_PARTITION_IS_JFFS2;
			}
#endif /* CONFIG_TFTP_UPDATE_ONTHEFLY */

			printf("\ndone\n");
			NetState = NETLOOP_SUCCESS;
		}
		break;

	case TFTP_ERROR:
		printf ("\nTFTP error: '%s' (%d)\n",
					pkt + 2, ntohs(*(ushort *)pkt));

		switch (ntohs(*(ushort *)pkt)) {
		case TFTP_ERR_FILE_NOT_FOUND:
		case TFTP_ERR_ACCESS_DENIED:
			puts("Not retrying...\n");
			/* Do not halt here... it causes problems on some platforms
			 * eth_halt();
			 */
			NetState = NETLOOP_FAIL;
			break;
		case TFTP_ERR_UNDEFINED:
		case TFTP_ERR_DISK_FULL:
		case TFTP_ERR_UNEXPECTED_OPCODE:
		case TFTP_ERR_UNKNOWN_TRANSFER_ID:
		case TFTP_ERR_FILE_ALREADY_EXISTS:
		default:
#if defined(CONFIG_TFTP_RETRIES_ON_ERROR)
			if (++TftpErrorCount > CONFIG_TFTP_RETRIES_ON_ERROR) {
				puts("\nRetry count exceeded; aborting\n");
				/* Do not halt here... it causes problems on some platforms
				 * eth_halt();
				 */
				NetState = NETLOOP_FAIL;
				break;
			}
			printf("Starting again (try %d of %d)\n\n",
			TftpErrorCount, CONFIG_TFTP_RETRIES_ON_ERROR);
#else
			puts("Starting again\n\n");
#endif

#ifdef CONFIG_MCAST_TFTP
			mcast_cleanup();
#endif
			NetStartAgain();
		}
		break;
	}
}


static void
TftpTimeout (void)
{
	if (++TftpTimeoutCount > TftpTimeoutCountMax) {
		printf ("\nRetry count exceeded; starting again\n");
#ifdef CONFIG_MCAST_TFTP
		mcast_cleanup();
#endif
#if defined(CONFIG_CMD_AUTOSCRIPT) &&		\
    defined(CONFIG_AUTOLOAD_BOOTSCRIPT) &&	\
    defined(CONFIG_TFTP_RETRIES_ON_ERROR)
		if (DownloadingAutoScript) {
			if (++TftpErrorCount > CONFIG_TFTP_RETRIES_ON_ERROR) {
				printf ("\nRetry count exceeded; aborting\n");
				NetState = NETLOOP_FAIL;
				return;
			}
			printf("Starting again (try %d of %d)\n\n",
				TftpErrorCount, CONFIG_TFTP_RETRIES_ON_ERROR);
		}
#endif
		NetStartAgain ();
	} else {
		printf ("T ");
		NetSetTimeout (TftpTimeoutMSecs, TftpTimeout);
		TftpSend ();
	}
}


void
TftpStart (void)
{
#ifdef CONFIG_TFTP_PORT
	char *ep;             /* Environment pointer */
#endif

	TftpServerIP = NetServerIP;
	if (BootFile[0] == '\0') {
		sprintf(default_filename, "%02lX%02lX%02lX%02lX.img",
			NetOurIP & 0xFF,
			(NetOurIP >>  8) & 0xFF,
			(NetOurIP >> 16) & 0xFF,
			(NetOurIP >> 24) & 0xFF	);

		strncpy(tftp_filename, default_filename, MAX_LEN);
		tftp_filename[MAX_LEN-1] = 0;

		printf ("*** Warning: no boot file name; using '%s'\n",
			tftp_filename);
	} else {
		char *p = strchr (BootFile, ':');

		if (p == NULL) {
			strncpy(tftp_filename, BootFile, MAX_LEN);
			tftp_filename[MAX_LEN-1] = 0;
		} else {
			TftpServerIP = string_to_ip (BootFile);
			strncpy(tftp_filename, p + 1, MAX_LEN);
			tftp_filename[MAX_LEN-1] = 0;
		}
	}

#if defined(CONFIG_NET_MULTI)
	printf ("Using %s device\n", eth_get_name());
#endif
	printf("TFTP from server %pI4"
		"; our IP address is %pI4", &TftpServerIP, &NetOurIP);

	/* Check if we need to send across this subnet */
	if (NetOurGatewayIP && NetOurSubnetMask) {
	    IPaddr_t OurNet	= NetOurIP    & NetOurSubnetMask;
	    IPaddr_t ServerNet	= TftpServerIP & NetOurSubnetMask;

	    if (OurNet != ServerNet)
		printf("; sending through gateway %pI4", &NetOurGatewayIP);
	}
	printf("\n");

	printf("Filename '%s'.", tftp_filename);

	if (NetBootFileSize) {
		printf (" Size is 0x%x Bytes = ", NetBootFileSize<<9);
		print_size (NetBootFileSize<<9, "");
	}

	printf ("\n");

	printf ("Load address: 0x%lx\n", load_addr);

#ifdef CONFIG_TFTP_UPDATE_ONTHEFLY
	if( (bTftpToFlashStatus & B_WRITE_IMG_TO_FLASH) == B_WRITE_IMG_TO_FLASH ){
		ulLastRamAddressWritten = 0;
		uiBlocksWrittenToFlash = 0;
		ulBytesCounter = 0;
		ulRamOffset = 0;

		printf("Loading and updating on-the-fly: \n\t");
	}
	else
#endif /* CONFIG_TFTP_UPDATE_ONTHEFLY */
	{
		printf("Loading: ");
	}

	TftpTimeoutMSecs = TftpRRQTimeoutMSecs;
	TftpTimeoutCountMax = TftpRRQTimeoutCountMax;

	NetSetTimeout (TftpTimeoutMSecs, TftpTimeout);
	NetSetHandler (TftpHandler);

	TftpServerPort = WELL_KNOWN_PORT;
	TftpTimeoutCount = 0;
	TftpState = STATE_RRQ;
	/* Use a pseudo-random port unless a specific port is set */
	TftpOurPort = 1024 + (get_timer(0) % 3072);

#ifdef CONFIG_TFTP_PORT
	if ((ep = getenv("tftpdstp")) != NULL) {
		TftpServerPort = simple_strtol(ep, NULL, 10);
	}
	if ((ep = getenv("tftpsrcp")) != NULL) {
		TftpOurPort= simple_strtol(ep, NULL, 10);
	}
#endif
	TftpBlock = 0;

	/* zero out server ether in case the server ip has changed */
	memset(NetServerEther, 0, 6);
	/* Revert TftpBlkSize to dflt */
	TftpBlkSize = TFTP_BLOCK_SIZE;
#ifdef CONFIG_MCAST_TFTP
	mcast_cleanup();
#endif

	TftpSend ();
}

#ifdef CONFIG_MCAST_TFTP
/* Credits: atftp project.
 */

/* pick up BcastAddr, Port, and whether I am [now] the master-client. *
 * Frame:
 *    +-------+-----------+---+-------~~-------+---+
 *    |  opc  | multicast | 0 | addr, port, mc | 0 |
 *    +-------+-----------+---+-------~~-------+---+
 * The multicast addr/port becomes what I listen to, and if 'mc' is '1' then
 * I am the new master-client so must send ACKs to DataBlocks.  If I am not
 * master-client, I'm a passive client, gathering what DataBlocks I may and
 * making note of which ones I got in my bitmask.
 * In theory, I never go from master->passive..
 * .. this comes in with pkt already pointing just past opc
 */
static void parse_multicast_oack(char *pkt, int len)
{
 int i;
 IPaddr_t addr;
 char *mc_adr, *port,  *mc;

	mc_adr=port=mc=NULL;
	/* march along looking for 'multicast\0', which has to start at least
	 * 14 bytes back from the end.
	 */
	for (i=0;i<len-14;i++)
		if (strcmp (pkt+i,"multicast") == 0)
			break;
	if (i >= (len-14)) /* non-Multicast OACK, ign. */
		return;

	i+=10; /* strlen multicast */
	mc_adr = pkt+i;
	for (;i<len;i++) {
		if (*(pkt+i) == ',') {
			*(pkt+i) = '\0';
			if (port) {
				mc = pkt+i+1;
				break;
			} else {
				port = pkt+i+1;
			}
		}
	}
	if (!port || !mc_adr || !mc ) return;
	if (Multicast && MasterClient) {
		printf ("I got a OACK as master Client, WRONG!\n");
		return;
	}
	/* ..I now accept packets destined for this MCAST addr, port */
	if (!Multicast) {
		if (Bitmap) {
			printf ("Internal failure! no mcast.\n");
			free(Bitmap);
			Bitmap=NULL;
			ProhibitMcast=1;
			return ;
		}
		/* I malloc instead of pre-declare; so that if the file ends
		 * up being too big for this bitmap I can retry
		 */
		if (!(Bitmap = malloc (Mapsize))) {
			printf ("No Bitmap, no multicast. Sorry.\n");
			ProhibitMcast=1;
			return;
		}
		memset (Bitmap,0,Mapsize);
		PrevBitmapHole = 0;
		Multicast = 1;
	}
	addr = string_to_ip(mc_adr);
	if (Mcast_addr != addr) {
		if (Mcast_addr)
			eth_mcast_join(Mcast_addr, 0);
		if (eth_mcast_join(Mcast_addr=addr, 1)) {
			printf ("Fail to set mcast, revert to TFTP\n");
			ProhibitMcast=1;
			mcast_cleanup();
			NetStartAgain();
		}
	}
	MasterClient = (unsigned char)simple_strtoul((char *)mc,NULL,10);
	Mcast_port = (unsigned short)simple_strtoul(port,NULL,10);
	printf ("Multicast: %s:%d [%d]\n", mc_adr, Mcast_port, MasterClient);
	return;
}

#endif /* Multicast TFTP */

#endif
