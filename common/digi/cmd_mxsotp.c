/*
 * cmd_imxotp.c - interface to Freescale's iMX One-Time-Programmable memory
 *
 * Copyright (c) 2012 Digi International Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
 */

#include <config.h>
#include <common.h>
#include <command.h>
#include <asm/io.h>
#include <asm/arch/regs-ocotp.h>
#include <asm/mxs_otp.h>
#include <net.h>

#ifdef CONFIG_MXS_OTP

#define MAX_OTP_REGS	0x28

#ifdef CONFIG_PLATFORM_HAS_HWID
void parse_hwid(const char *str, u8 *hwid)
{
	char *end;
	int i;

	for (i = 0; i < CONFIG_HWID_LENGTH; ++i) {
		hwid[i] = str ? simple_strtoul(str, &end, 16) : 0;
		if (str)
			str = (*end) ? end + 1 : end;
	}
}
#endif

int do_otp(cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
	int i;
	unsigned char addr = 0;
	unsigned char count = 1;
	unsigned long data = 0;

	if (argc < 2 || argc > 5)
		goto usage;

	if (strcmp(argv[1], "dump") == 0) {
		if (open_otp_bank())
			goto errotp;
		dump_otp_regs();
		close_otp_bank();
	}
	else if (strcmp(argv[1], "read") == 0) {
		if (argc < 3)
			goto usage;
#ifdef CONFIG_PLATFORM_HAS_HWID
		if (!strcmp(argv[2], "hwid")) {
			/* Read hwid */
			/*
			 *       | 31..           HWOTP_CUST1            ..0 | 31..          HWOTP_CUST0             ..0 |
			 *       +----------+----------+----------+----------+----------+----------+----------+----------+
			 * HWID: |    --    | TF (loc) | variant  | HV |Cert |   Year   | Mon |     Serial Number        |
			 *       +----------+----------+----------+----------+----------+----------+----------+----------+
			 * Byte: 0          1          2          3          4          5          6          7
			 */
			unsigned char hwid[CONFIG_HWID_LENGTH];

			if (read_otp_hwid(hwid))
				goto errotp;
			printf("HWID: ");
			for (i=0; i < CONFIG_HWID_LENGTH; i++) {
				printf("%02x", hwid[i]);
				if (i != 7)
					printf(":");
			}
			printf("\n");
			printf("    TF (location): 0x%02x\n", hwid[1]);
			printf("    Variant:       0x%02x\n", hwid[2]);
			printf("    HW Version:    %d\n", (hwid[3] & 0xf0) >> 4);
			printf("    Cert:          0x%x\n", hwid[3] & 0xf);
			printf("    Year:          20%02d\n", hwid[4]);
			printf("    Month:         %02d\n", (hwid[5] & 0xf0) >> 4);
			printf("    S/N:           %d\n", ((hwid[5] & 0xf) << 16) | (hwid[6] << 8) | hwid[7]);
		}
		else
#endif
		{
			/* Read OTP register(s) */
			addr = (unsigned char)simple_strtol(argv[2], NULL, 16);
			if (addr >= MAX_OTP_REGS) {
				printf("Invalid address. Address must be in the range [0..0x%x]\n",
					MAX_OTP_REGS - 1);
				goto usage;
			}
			if (argc == 4)
				count = (unsigned char)simple_strtol(argv[3], NULL, 16);
			if (open_otp_bank())
				goto errotp;
			for (i=addr; i < MAX_OTP_REGS && i < addr + count; i++) {
				printf("HW_OCOTP[0x%02x]: 0x%08x\n", i,
					(unsigned int)read_otp_reg(i));
			}
			close_otp_bank();
		}
	}
	else if (strcmp(argv[1], "blow") == 0) {
		if (argc != 4) {
			printf("Invalid number of arguments\n");
			goto usage;
		}
#ifdef CONFIG_PLATFORM_HAS_HWID
		if(!strcmp(argv[2], "hwid")) {
			/* Write hwid */
			u8 hwid[CONFIG_HWID_LENGTH] = {0};
			/*
			 *       | 31..           HWOTP_CUST1            ..0 | 31..          HWOTP_CUST0             ..0 |
			 *       +----------+----------+----------+----------+----------+----------+----------+----------+
			 * HWID: |    --    | TF (loc) | variant  | HV |Cert |   Year   | Mon |     Serial Number        |
			 *       +----------+----------+----------+----------+----------+----------+----------+----------+
			 * Byte: 0          1          2          3          4          5          6          7
			 */
			parse_hwid(argv[3], hwid);
			if (blow_otp_hwid(hwid))
				goto errotp;
		}
		else
#endif
		{
			/* Write an OTP register */
			addr = (unsigned char)simple_strtol(argv[2], NULL, 16);
			if (addr >= MAX_OTP_REGS) {
				printf("Invalid address. Address must be in the range [0..0x%x]\n",
					MAX_OTP_REGS - 1);
				goto usage;
			}
			data = simple_strtol(argv[3], NULL, 16);
			if (blow_otp_reg(addr, data))
				goto errotp;
		}
	}
	else if (strcmp(argv[1], "lock") == 0) {
		if (argc != 3) {
			printf("Invalid number of arguments\n");
			goto usage;
		}
		addr = (unsigned char)simple_strtol(argv[2], NULL, 16);
		if (addr >= MAX_OTP_REGS) {
			printf("Invalid address. Address must be in the range [0..0x%x]\n",
				MAX_OTP_REGS - 1);
			goto usage;
		}
		if (lock_otp_reg(addr))
			goto errotp;
	}
	else
		goto usage;

	return 0;
usage:
	printf("Invalid parameters!\n");
	cmd_usage(cmdtp);
	return 1;
errotp:
	printf("Error accessing OTP bits\n");
	return 1;
}

U_BOOT_CMD(otp, 4, 0, do_otp,
	"One-Time-Programmable sub-system",
	"\n"
	"Warning: all numbers in parameter are in hex format!\n"
	"otp dump                                - Dump all OTP bits\n"
	"otp read <addr> [count]                 - Read 'count' OTP registers (32-bit wide) starting at 'addr'\n"
#ifdef CONFIG_PLATFORM_HAS_HWID
	"otp read hwid                           - Read hwid value\n"
#endif
	"otp blow <addr> <value>                 - Blow OTP register at 'addr' with 'value'\n"
	"otp blow hwid ##:##:##:##:##:##:##:##   - Blow hwid value\n"
	"otp lock <addr>                         - Lock register at 'addr'\n"
);

#endif /* CONFIG_MXS_OTP */
