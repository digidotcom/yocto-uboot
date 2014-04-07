/*
 * (C) Copyright 2014 Digi International, Inc.
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

#include <common.h>
#include <command.h>
#include <fuse.h>
#include <asm/errno.h>

extern void board_print_hwid(u32 *hwid);

static int strtou32(const char *str, unsigned int base, u32 *result)
{
	char *ep;

	*result = simple_strtoul(str, &ep, base);
	if (ep == str || *ep != '\0')
		return -EINVAL;

	return 0;
}

static int confirm_prog(void)
{
	puts("Warning: Programming fuses is an irreversible operation!\n"
			"         This may brick your system.\n"
			"         Use this command only if you are sure of "
					"what you are doing!\n"
			"\nReally perform this fuse programming? <y/N>\n");

	if (getc() == 'y') {
		int c;

		putc('y');
		c = getc();
		putc('\n');
		if (c == '\r')
			return 1;
	}

	puts("Fuse programming aborted\n");
	return 0;
}

__weak void board_print_hwid(u32 *hwid)
{
	int i;

	for (i = CONFIG_HWID_WORDS_NUMBER - 1; i >= 0; i--)
		printf(" %.8x", hwid[i]);
	printf("\n");
}

static int do_hwid(cmd_tbl_t *cmdtp, int flag, int argc, char *const argv[])
{
	const char *op = argc >= 2 ? argv[1] : NULL;
	int confirmed = argc >= 3 && !strcmp(argv[2], "-y");
	u32 bank = CONFIG_HWID_BANK;
	u32 word = CONFIG_HWID_START_WORD;
	u32 cnt = CONFIG_HWID_WORDS_NUMBER;
	u32 val[8];
	int ret, i;

	argc -= 2 + confirmed;
	argv += 2 + confirmed;

	if (!strcmp(op, "read")) {
		printf("Reading HWID: ");
		for (i = 0; i < cnt; i++, word++) {
			ret = fuse_read(bank, word, &val[i]);
			if (ret)
				goto err;
		}
		board_print_hwid(val);
	} else if (!strcmp(op, "sense")) {
		printf("Sensing HWID: ");
		for (i = 0; i < cnt; i++, word++) {
			ret = fuse_sense(bank, word, &val[i]);
			if (ret)
				goto err;
		}
		board_print_hwid(val);
	} else if (!strcmp(op, "prog")) {
		if (argc < CONFIG_HWID_WORDS_NUMBER)
			return CMD_RET_USAGE;

		if (!confirmed && !confirm_prog())
			return CMD_RET_FAILURE;
		printf("Programming HWID... ");
		/* Write backwards, from MSB to LSB */
		word = CONFIG_HWID_START_WORD + CONFIG_HWID_WORDS_NUMBER - 1;
		for (i = 0; i < CONFIG_HWID_WORDS_NUMBER; i++, word--) {
			if (strtou32(argv[i], 16, &val[i]))
				return CMD_RET_USAGE;

			ret = fuse_prog(bank, word, val[i]);
			if (ret)
				goto err;
		}
		printf("OK\n");
	} else if (!strcmp(op, "override")) {
		if (argc < CONFIG_HWID_WORDS_NUMBER)
			return CMD_RET_USAGE;

		printf("Overriding HWID... ");
		/* Write backwards, from MSB to LSB */
		word = CONFIG_HWID_START_WORD + CONFIG_HWID_WORDS_NUMBER - 1;
		for (i = 0; i < CONFIG_HWID_WORDS_NUMBER; i++, word--) {
			if (strtou32(argv[i], 16, &val[i]))
				return CMD_RET_USAGE;

			ret = fuse_override(bank, word, val[i]);
			if (ret)
				goto err;
		}
		printf("OK\n");
	} else {
		return CMD_RET_USAGE;
	}

	return 0;

err:
	puts("ERROR\n");
	return ret;
}

U_BOOT_CMD(
	hwid, CONFIG_SYS_MAXARGS, 0, do_hwid,
	"HWID on fuse sub-system",
	     "read - read HWID from shadow registers\n"
	"hwid sense - sense HWID from fuses\n"
	"hwid prog [-y] <hexval MSB> [.. <hexval LSB>] - program HWID (PERMANENT)\n"
	"hwid override <hexval MSB> [.. <hexval LSB>] - override HWID"
);
