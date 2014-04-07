/*
 *  common/digi/cmd_video.c
 *
 *  Copyright (C) 2010 by Digi International Inc.
 *  All rights reserved.
 *
 *  This program is free software; you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License version2  as published by
 *  the Free Software Foundation.
*/

#include <common.h>
#if defined(CONFIG_CMD_BSP) && (defined(CONFIG_DISPLAY1_ENABLE) || defined(CONFIG_DISPLAY2_ENABLE))

#include <command.h>
#include <vscanf.h>
#include "env.h"
#include "nvram.h"
#include "cmd_bsp.h"
#include "cmd_video.h"

#define CTRL_C		0x3
#define USER_CANCEL	(~0)

enum {
	MENU_TITLE,
	MENU_OPTION,
	MENU_PROMPT,
};

static int active_vif = 0;

typedef struct menu_opt {
	int		key;
	const char	*text;
	int		(*func)(const struct menu_opt *);
	void		*param;
} menu_opt_t;

static int dummy_func(const struct menu_opt *menu);
#ifdef CONFIG_DISPLAY1_ENABLE
static int select_video_if1(const struct menu_opt *);
#endif
#ifdef CONFIG_DISPLAY2_ENABLE
static int select_video_if2(const struct menu_opt *);
#endif
static int select_video_disabled(const struct menu_opt *);
#ifdef VIDEO_OPTIONS_VGA
static int select_video_vga(const struct menu_opt *);
static int set_video_vga(const struct menu_opt *);
#endif
#ifdef VIDEO_OPTIONS_HDMI
static int select_video_hdmi(const struct menu_opt *);
static int set_video_hdmi(const struct menu_opt *);
#endif
#ifdef VIDEO_OPTIONS_LCD
static int select_video_lcd(const struct menu_opt *);
static int set_video_lcd(const struct menu_opt *);
#endif
static int set_custom_nv_settings(const struct menu_opt *menu);
#if defined(CONFIG_DISPLAY1_ENABLE) && defined(CONFIG_DISPLAY2_ENABLE)
static int select_primary_disp(const struct menu_opt *menu);
static int set_primary_disp(const struct menu_opt *menu);
#endif

#ifdef CONFIG_DISPLAY_LVDS_ENABLE
static int select_video_ldb_config(const struct menu_opt *menu);
static int select_datamap_ldb(const struct menu_opt *menu);
static int set_datamap_ldb(const struct menu_opt *menu);
static char ldbvar[40] = "";

#define LCD_TYPE_PARALLEL	0
#define LCD_TYPE_LVDS		1
static char lcd_type;
static int set_lcd_type(const struct menu_opt *menu);

static menu_opt_t select_ldb_datamap_menu[] =
{
	{ MENU_TITLE, "Select the data mapping", NULL, NULL},
	{ MENU_OPTION, "CH 0 SPWG", set_datamap_ldb, ",ch0_map=SPWG"},
	{ MENU_OPTION, "CH 0 JEIDA", set_datamap_ldb, ",ch0_map=JEIDA"},
	{ MENU_OPTION, "CH 1 SPWG", set_datamap_ldb, ",ch1_map=SPWG"},
	{ MENU_OPTION, "CH 1 JEIDA", set_datamap_ldb, ",ch1_map=JEIDA"},
	{ MENU_OPTION, "CH 0 SPWG, CH1 SPWG", set_datamap_ldb, ",ch0_map=SPWG,ch1_map=SPWG"},
	{ MENU_OPTION, "CH 0 JEIDA, CH1 SPWG", set_datamap_ldb, ",ch0_map=JEIDA,ch1_map=SPWG"},
	{ MENU_OPTION, "CH 0 SPWG, CH1 JEIDA", set_datamap_ldb, ",ch0_map=SPWG,ch1_map=JEIDA"},
	{ MENU_OPTION, "CH 0 JEIDA, CH1 JEIDA", set_datamap_ldb, ",ch0_map=JEIDA,ch1_map=JEIDA"},
	{ MENU_PROMPT, "Select the data mapping", NULL, NULL},
};

static menu_opt_t select_ldb_menu[] =
{
	{ MENU_TITLE, "LDVS bridge mode", NULL, NULL},
	{ MENU_OPTION, "Disabled", select_datamap_ldb,
				"off"},
	{ MENU_OPTION, "Single Channel DI0", select_datamap_ldb,
				"single,di=0"},
	{ MENU_OPTION, "Single Channel DI1", select_datamap_ldb,
				"single,di=1"},
	{ MENU_OPTION, "Separate Channels (dual independent)", select_datamap_ldb,
				"separate"},
	{ MENU_OPTION, "Dual Channel DI0", select_datamap_ldb,
				"dual,di=0"},
	{ MENU_OPTION, "Dual Channel DI1", select_datamap_ldb,
				"dual,di=1"},
	{ MENU_OPTION, "Split Channel DI0", select_datamap_ldb,
				"split,di=0"},
	{ MENU_OPTION, "Split Channel DI1", select_datamap_ldb,
				"split,di=1"},
	{ MENU_PROMPT, "Select LVDS bridge mode", NULL, NULL},
};

static menu_opt_t select_lcd_type_menu[] =
{
	{ MENU_TITLE, "Type of LCD", NULL, NULL},
	{ MENU_OPTION, "Parallel", set_lcd_type, "para"},
	{ MENU_OPTION, "LVDS", set_lcd_type, "lvds"},
	{ MENU_OPTION, "Cancel", dummy_func, NULL},
	{ MENU_PROMPT, "Select LCD type", NULL, NULL},
};

#endif

static menu_opt_t select_vif_menu[] =
{
	{ MENU_TITLE, "Video interface", NULL, NULL},
#ifdef CONFIG_DISPLAY1_ENABLE
	{ MENU_OPTION, "Video 1", select_video_if1, NULL},
#endif
#ifdef CONFIG_DISPLAY2_ENABLE
	{ MENU_OPTION, "Video 2", select_video_if2, NULL},
#endif
#ifdef CONFIG_DISPLAY_LVDS_ENABLE
	{ MENU_OPTION, "LVDS bridge", select_video_ldb_config, NULL},
#endif
#if defined(CONFIG_DISPLAY1_ENABLE) && defined(CONFIG_DISPLAY2_ENABLE)
	{ MENU_OPTION, "Primary display", select_primary_disp, NULL},
#endif
	{ MENU_OPTION, "Cancel", dummy_func, NULL},
	{ MENU_PROMPT, "Select interface", NULL, NULL},
};

#ifdef CONFIG_DISPLAY1_ENABLE
static menu_opt_t select_if1_menu[] =
{
	{ MENU_TITLE, "Displays/video mode", NULL, NULL},
	{ MENU_OPTION, "Disabled", select_video_disabled, "0"},
#ifdef VIDEO_OPTIONS_DISPLAY1
	VIDEO_OPTIONS_DISPLAY1,
#endif
	{ MENU_OPTION, "Cancel", dummy_func, NULL},
	{ MENU_PROMPT, "Select displays/video mode", NULL, NULL},
};
#endif

#ifdef CONFIG_DISPLAY2_ENABLE
static menu_opt_t select_if2_menu[] =
{
	{ MENU_TITLE, "Displays/video mode", NULL, NULL},
	{ MENU_OPTION, "Disabled", select_video_disabled, "1"},
#ifdef VIDEO_OPTIONS_DISPLAY2
	VIDEO_OPTIONS_DISPLAY2,
#endif
	{ MENU_OPTION, "Cancel", dummy_func, NULL},
	{ MENU_PROMPT, "Select displays/video mode", NULL, NULL},
};
#endif

#ifdef VIDEO_OPTIONS_VGA
static menu_opt_t select_vga_menu[] =
{
	{ MENU_TITLE, "VGA video mode", NULL, NULL},
	VIDEO_OPTIONS_VGA,
	{ MENU_OPTION, "custom1 configuration", set_video_vga, "custom1"},
	{ MENU_OPTION, "custom2 configuration", set_video_vga, "custom2"},
	{ MENU_OPTION, "Cancel", dummy_func, NULL},
	{ MENU_PROMPT, "Select VGA mode", NULL, NULL},
};
#endif

#ifdef VIDEO_OPTIONS_HDMI
static menu_opt_t select_hdmi_menu[] =
{
	{ MENU_TITLE, "HDMI video mode", NULL, NULL},
	{ MENU_OPTION, "Auto", set_video_hdmi, "auto"},
	VIDEO_OPTIONS_HDMI,
	{ MENU_OPTION, "custom1 configuration", set_video_hdmi, "custom1"},
	{ MENU_OPTION, "custom2 configuration", set_video_hdmi, "custom2"},
	{ MENU_OPTION, "Cancel", dummy_func, NULL},
	{ MENU_PROMPT, "Select HDMI mode", NULL, NULL},
};
#endif

static menu_opt_t select_lcd_menu[] =
{
	{ MENU_TITLE, "LCD display", NULL, NULL},
#ifdef VIDEO_OPTIONS_LCD
	VIDEO_OPTIONS_LCD,
#endif
	{ MENU_OPTION, "custom1 configuration", set_video_lcd, "custom1"},
	{ MENU_OPTION, "custom2 configuration", set_video_lcd, "custom2"},
	{ MENU_OPTION, "custom3 configuration in NVRAM", set_custom_nv_settings, "custom3_nv"},
	{ MENU_OPTION, "custom4 configuration in NVRAM", set_custom_nv_settings, "custom4_nv"},
	{ MENU_OPTION, "Cancel", dummy_func, NULL},
	{ MENU_PROMPT, "Select LCD display", NULL, NULL},
};

#if defined(CONFIG_DISPLAY1_ENABLE) && defined(CONFIG_DISPLAY2_ENABLE)
static menu_opt_t select_primary_disp_menu[] =
{
	{ MENU_TITLE, "Primary display", NULL, NULL},
	{ MENU_OPTION, "Video 1", set_primary_disp, VIDEO_VAR},
	{ MENU_OPTION, "Video 2", set_primary_disp, VIDEO2_VAR},
	{ MENU_OPTION, "Cancel", dummy_func, NULL},
	{ MENU_PROMPT, "Select primary display", NULL, NULL},
};
#endif

static void print_menu(const menu_opt_t menu[], const int entries)
{
	int i, opt = 1;

	for (i = 0; i < entries; i++) {
		switch(menu[i].key) {
		case MENU_TITLE:
			printf("\n%s\n", menu[i].text);
			break;
		case MENU_PROMPT:
			printf("%s: ", menu[i].text);
			break;
		case MENU_OPTION:
			//printf("  %d) %s\n", opt++, menu[i].text);
			printf("  %c) %s\n", ((opt > 9) ? ('a' - 10) : '0') + opt,
			       menu[i].text);
			opt++;
			break;
		}
	}
}

static int process_menu_otpion(const menu_opt_t menu[], const int entries, int opt)
{
	int i, ret = 1;

	for (i = 0; i < entries; i++) {
		if ((MENU_OPTION == menu[i].key) && i != (opt - ((opt > '9') ? ('a' - 10) : '0')))
			continue;
		if (menu[i].func) {
			ret = menu[i].func((menu_opt_t *)&menu[i]);
			break;
		}
	}
	return ret;
}

static int read_opt(void)
{
	/* wait for key to be pressed */
	while (!tstc())
		;
	return getc();
}

static int run_menu(const menu_opt_t menu[], const int entries)
{
        int ret, exit_menu = 0;

	do {
		print_menu(menu, entries);

		ret = read_opt();
                if (ret ==  CTRL_C)
			exit_menu = 1;
		else {
			printf("%c\n", ret);
			ret = process_menu_otpion(menu, entries, ret);
			if (CTRL_C == ret || ret == USER_CANCEL)
				exit_menu = 1;
			else
				exit_menu = !ret;
		}
	} while (!exit_menu);
	return ret;
}

static int dummy_func(const struct menu_opt *menu)
{
	return USER_CANCEL;
}

#ifdef CONFIG_DISPLAY1_ENABLE
static int select_video_if1(const struct menu_opt *menu)
{
	active_vif = 0;
	return run_menu(select_if1_menu, ARRAY_SIZE(select_if1_menu));
}
#endif

#ifdef CONFIG_DISPLAY2_ENABLE
static int select_video_if2(const struct menu_opt *menu)
{
	active_vif = 1;
	return run_menu(select_if2_menu, ARRAY_SIZE(select_if2_menu));
}
#endif

#ifdef VIDEO_OPTIONS_VGA
static int set_video_vga(const struct menu_opt *menu)
{
	char var[30];

	strcpy(var, VGA_PREFIX);
	strcat(var, menu->param);
	setenv(active_vif == 0 ? VIDEO_VAR : VIDEO2_VAR, var);

	return 0;
}
#endif

#ifdef VIDEO_OPTIONS_HDMI
static int set_video_hdmi(const struct menu_opt *menu)
{
	char var[30];

	strcpy(var, HDMI_PREFIX);
	strcat(var, menu->param);
	setenv(active_vif == 0 ? VIDEO_VAR : VIDEO2_VAR, var);

	return 0;
}
#endif

#ifdef VIDEO_OPTIONS_LCD
static int set_video_lcd(const struct menu_opt *menu)
{
	char var[30];

#ifdef CONFIG_DISPLAY_LVDS_ENABLE
	if (lcd_type == LCD_TYPE_LVDS)
		strcpy(var, LVDS_PREFIX);
	else
#endif
		strcpy(var, LCD_PREFIX);
	strcat(var, menu->param);
	setenv(active_vif == 0 ? VIDEO_VAR : VIDEO2_VAR, var);

	return 0;
}
#endif

static int select_video_disabled(const struct menu_opt *menu)
{
	if (menu->param)
		setenv(*((char *)(menu->param)) == '0' ?
			VIDEO_VAR : VIDEO2_VAR, VIDEO_DISABLED);

	return 0;
}

#ifdef VIDEO_OPTIONS_VGA
static int select_video_vga(const struct menu_opt *menu)
{
	return run_menu(select_vga_menu, ARRAY_SIZE(select_vga_menu));
}
#endif

#ifdef VIDEO_OPTIONS_HDMI
static int select_video_hdmi(const struct menu_opt *menu)
{
	return run_menu(select_hdmi_menu, ARRAY_SIZE(select_hdmi_menu));
}
#endif


#ifdef CONFIG_DISPLAY_LVDS_ENABLE
static int set_lcd_type(const struct menu_opt *menu)
{
	if (!menu->param)
		return 1;

	if (!strcmp(menu->param, "lvds"))
		lcd_type = LCD_TYPE_LVDS;
	else
		lcd_type = LCD_TYPE_PARALLEL;

	return 0;
}
#endif

static int select_video_lcd(const struct menu_opt *menu)
{
#ifdef CONFIG_DISPLAY_LVDS_ENABLE
	int ret;

	ret = run_menu(select_lcd_type_menu, ARRAY_SIZE(select_lcd_type_menu));
	if (ret != 0)
		return ret;
#else

#endif

	return run_menu(select_lcd_menu, ARRAY_SIZE(select_lcd_menu));
}

#ifdef CONFIG_DISPLAY_LVDS_ENABLE
static int select_video_ldb_config(const struct menu_opt *menu)
{
	return run_menu(select_ldb_menu, ARRAY_SIZE(select_ldb_menu));
}

static int set_datamap_ldb(const struct menu_opt *menu)
{
	if (menu->param)
		strcat(ldbvar, menu->param);
	setenv(LDB_VAR, ldbvar);
	return 0;
}

static int select_datamap_ldb(const struct menu_opt *menu)
{
	if (!menu->param)
		return 0;

	if (!strcmp(menu->param, "off")) {
		setenv(LDB_VAR, menu->param);
		return 0;
	}
	strcpy(ldbvar, menu->param);
	return run_menu(select_ldb_datamap_menu, ARRAY_SIZE(select_ldb_datamap_menu));
}
#endif

#if defined(CONFIG_DISPLAY1_ENABLE) && defined(CONFIG_DISPLAY2_ENABLE)
static int select_primary_disp(const struct menu_opt *menu)
{
	return run_menu(select_primary_disp_menu, ARRAY_SIZE(select_primary_disp_menu));
}

static int set_primary_disp(const struct menu_opt *menu)
{
	setenv(FBPRIMARY_VAR, menu->param);

	return 0;
}
#endif

typedef struct help_info {
	const char*	szFlag;
	const char*	szDescr;
} help_info_t;

static const help_info_t syncHelpTable[] = {
	{ "0x00000001", "FB_SYNC_HOR_HIGH_ACT" },
	{ "0x00000002", "FB_SYNC_VERT_HIGH_ACT" },
	{ "0x00000004", "FB_SYNC_EXT" },
	{ "0x00000008", "FB_SYNC_COMP_HIGH_ACT" },
	{ "0x00000010", "FB_SYNC_BROADCAST" },
	{ "0x00000020", "FB_SYNC_ON_GREEN" },
	{ "0x04000000", "FB_SYNC_SWAP_RGB" },
	{ "0x08000000", "FB_SYNC_SHARP_MODE" },
	{ "0x10000000", "FB_SYNC_CLK_IDLE_EN" },
	{ "0x20000000", "FB_SYNC_DATA_INVERT" },
	{ "0x40000000", "FB_SYNC_CLK_LAT_FALL" },
	{ "0x80000000", "FB_SYNC_OE_LOW_ACT" },
};

static const help_info_t vmodeHelpTable[] = {
	{ "0", "FB_VMODE_NONINTERLACED" },
	{ "1", "FB_VMODE_INTERLACED" },
	{ "2", "FB_VMODE_DOUBLE" },
	{ "4", "FB_VMODE_ODD_FLD_FIRST" },
};

static void help_table_print(const help_info_t *help_table, size_t size)
{
        int i;

        for( i = 0; i < size; i++, help_table++ )
                printf( "   %s) %s\n",
                        help_table->szFlag,
                        help_table->szDescr );
}

static int get_str(char *buffer, size_t buffer_size)
{
	int ret = 0;
	char ch;
	uint8_t loop = 1;
	char *s;
	int i = 0;

	s = buffer;

	do {
		ch = read_opt();
		switch (ch) {
			case '\r':
				/* enter */
				*s = 0;
				loop = 0;
				break;

			case CTRL_C:
				loop = 0;
				ret = USER_CANCEL;

			case 0x08:
				/* backspace */
				if ( i ) {
					printf("\b \b");
					i--;
					s--;
				}
				break;

			default:
				/* Don't let buffer to overrun */
				if ( i != (buffer_size - 1) )
				{
					printf("%c", ch);
					*s = ch;
					i++;
					s++;
				}
				break;
		}
	} while ( loop );

	return ret;
}

static int get_uint(const char *msg, uint32_t *var, uint32_t base, uint8_t modify,
		    const help_info_t *help_table, size_t help_table_size)
{
	int ret;
	uint8_t loop = 1;
	char buffer[256] = "";

	do {
		printf("%s", msg);

		if ( (help_table != NULL) && (help_table_size != 0) ) {
			printf(" (? for help)");
		}

		if ( modify ) {
			if ( base == 16 ) {
				printf(" [0x%x]: ", *var);
			}
			else {
				printf(" [%d]: ", *var);
			}
		}
		else {
			printf(": ");
		}

		ret = get_str(buffer, 256);
		switch (ret) {
			case 0:
				/* OK */

				/* If string is empty, it was just an enter */
				if ( buffer[0] == 0 ) {
					/* If modify, than keep the old value */
					/* Otherwise ask for data again */
					if ( modify ) {
						loop = 0;
						ret = 0;
					}
				}
				else if ( (buffer[0] == '?') &&
					(help_table != NULL) &&
					(help_table_size != 0) ) {
					printf("\n");
					help_table_print(help_table, help_table_size);

					/* Skip "wrong value" message and ask for data again */
					continue;
				}
				else {
					char *endp;
					*var = simple_strtoul(buffer, &endp, base);
					if ( strlen(buffer) == (endp - buffer) ) {
						loop = 0;
						ret = 0;
					}
					/* Else ask for data again */
				}

				break;

			case USER_CANCEL:
				/* CTRL-C */
				loop = 0;
				ret = USER_CANCEL;
				break;

			default:
				/* Ask for data again */
				break;
		}

		if ( loop ) {
			printf("\n*** Wrong value, try again\n");

		}

	} while ( loop );

	printf("\n");

	return ret;
}

static int set_custom_nv_settings(const struct menu_opt *menu)
{
	int ret = 0;
	nv_lcd_config_t lcd_config;
	nv_lcd_config_data_t *lcd_config_data;
	uint8_t config_index;
	uint32_t temp;
	char ch;

	ret = LCDConfigRead(&lcd_config);
	switch ( ret )
	{
		/* OK */
		case 0:
			break;

		/* LCD configuration read error, try to create it */
		case 1:
			printf("\n***No valid LCD configuration found!\n");

			break;

		/* LCD config has an invalid size, resize it */
		case 2:
			do {
				printf("\n***Invalid LCD configuration! It is necessary to recreate " \
				"the configuration.\nIf you continue, you will lose your existing settings.\n");
				printf("Do you want to continue (y/n)? ");

				ch = read_opt();
				printf("%c\n", ch);
				if ( (ch == USER_CANCEL) || (ch == 'n') ) {
					goto error;
				}

			} while ( ch != 'y' );

			break;

		default:
			goto error;
	}

	if ( ret > 0 ) {
		printf("\nCreating LCD configuration... ");
		LCDConfigAdd();

		/* Try again to read config */
		if ( LCDConfigRead(&lcd_config) > 0 ) {
			printf("*** Couldn't read LCD configuration from NVRAM\n");
			goto error;
		}
		printf("OK\n\n");
	}

	if ( lcd_config.header.version < NV_LCD_CONFIG_VERSION ) {
		printf("\n*** Invalid LCD configuration version (%d instead of %d).\nConfiguration is resetted!\n\n",
		       lcd_config.header.version, NV_LCD_CONFIG_VERSION);
		LCDConfigReset(&lcd_config);
	}

	if ( !strncmp(menu->param, "custom4_nv", 13) ) {
		lcd_config_data = &lcd_config.lcd2;
		config_index = 2;
	}
	else {
		lcd_config_data = &lcd_config.lcd1;
		config_index = 1;
	}

	/* Get refresh rate */
	if ( (ret = get_uint("  Refresh rate (Hz)", &lcd_config_data->video_mode.refresh, 10, 1,
		NULL, 0)) != 0 ) {
		goto error;
	}

	/* Get Xres */
	if ( (ret = get_uint("  X-resolution (pixels)", &lcd_config_data->video_mode.xres, 10, 1,
		NULL, 0)) != 0 ) {
		goto error;
	}

	/* Get Yres */
	if ( (ret = get_uint("  Y-resolution (pixels)", &lcd_config_data->video_mode.yres, 10, 1,
		NULL, 0)) != 0 ) {
		goto error;
	}

	/* Get pixel clock */
	if ( (ret = get_uint("  Pixel clock (picoseconds)", &lcd_config_data->video_mode.pixclock, 10, 1,
		NULL, 0)) != 0 ) {
		goto error;
	}

	/* Get left margin */
	if ( (ret = get_uint("  Left margin (clocks)", &lcd_config_data->video_mode.left_margin, 10, 1,
		NULL, 0)) != 0 ) {
		goto error;
	}

	/* Get right margin */
	if ( (ret = get_uint("  Right margin (clocks)", &lcd_config_data->video_mode.right_margin, 10, 1,
		NULL, 0)) != 0 ) {
		goto error;
	}

	/* Get upper margin */
	if ( (ret = get_uint("  Upper margin (lines)", &lcd_config_data->video_mode.upper_margin, 10, 1,
		NULL, 0)) != 0 ) {
		goto error;
	}

	/* Get lower margin */
	if ( (ret = get_uint("  Lower margin (lines)", &lcd_config_data->video_mode.lower_margin, 10, 1,
		NULL, 0)) != 0 ) {
		goto error;
	}

	/* Get HSync */
	if ( (ret = get_uint("  H-sync (clocks)", &lcd_config_data->video_mode.hsync_len, 10, 1,
		NULL, 0)) != 0 ) {
		goto error;
	}

	/* Get VSync */
	if ( (ret = get_uint("  V-sync (lines)", &lcd_config_data->video_mode.vsync_len, 10, 1,
		NULL, 0)) != 0 ) {
		goto error;
	}

	/* Get sync */
	if ( (ret = get_uint("  Sync", &lcd_config_data->video_mode.sync, 16, 1,
		syncHelpTable, sizeof(syncHelpTable) / sizeof(help_info_t))) != 0 ) {
		goto error;
	}

	/* Get vmode */
	if ( (ret = get_uint("  V-Mode", &lcd_config_data->video_mode.vmode, 10, 1,
		vmodeHelpTable, sizeof(vmodeHelpTable) / sizeof(help_info_t))) != 0 ) {
		goto error;
	}

	/* Get flags */
	if ( (ret = get_uint("  Flags", &lcd_config_data->video_mode.flag, 10, 1,
		NULL, 0)) != 0 ) {
		goto error;
	}

	/* Backlight function is enabled */
	temp = lcd_config_data->is_bl_enable_func;
	if ( (ret = get_uint("  Backlight function enabled", &temp, 10, 1,
		NULL, 0)) != 0 ) {
		goto error;
	}
	lcd_config_data->is_bl_enable_func = (temp != 0);

	printf("\nWindows CE specific settings:\n");

	/* Get pixel data offset */
	if ( (ret = get_uint("  Pixel Data Offset", &lcd_config_data->pix_cfg.pix_data_offset, 10, 1,
		NULL, 0)) != 0 ) {
		goto error;
	}

	/* Get pixel clock up */
	if ( (ret = get_uint("  Pixel Clock Up", &lcd_config_data->pix_cfg.pix_clk_up, 10, 1,
		NULL, 0)) != 0 ) {
		goto error;
	}

	/* Get pixel clock down */
	if ( (ret = get_uint("  Pixel Clock Down", &lcd_config_data->pix_cfg.pix_clk_down, 10, 1,
		NULL, 0)) != 0 ) {
		goto error;
	}

	if ( config_index == 1 ) {
		lcd_config.header.lcd1_valid = 1;
	}
	else {
		lcd_config.header.lcd2_valid = 1;
	}

	if ( LCDConfigWrite(&lcd_config) == 0 ) {
		printf("\n***Couldn't write config into the NVRAM!\n");
		ret = 1;
		goto error;
	}

	if ( NvEnvUpdateLCDConfig() == 0 ) {
		printf("\n***Couldn't update shared memory\n");
		ret = 1;
		goto error;
	}

	return set_video_lcd(menu);

error:
	return ret;
}


/*! \brief Sets video (and video2) environment variables to one of the supported displays */
/*! \return 1 on failure otherwise 0 */
static int do_video(cmd_tbl_t* cmdtp, int flag, int argc, char* argv[])
{
	int ret = 1;
	const char *s;

	if (argc != 1)
		return ret;

	ret = run_menu(select_vif_menu, ARRAY_SIZE(select_vif_menu));
	if (!ret) {
		puts("\n");
#ifdef CONFIG_DISPLAY1_ENABLE
		if ((s = GetEnvVar(VIDEO_VAR, 1)) != NULL)
			printf("%s=%s\n", VIDEO_VAR, s);
#endif
#ifdef CONFIG_DISPLAY2_ENABLE
		if ((s = GetEnvVar(VIDEO2_VAR, 1)) != NULL)
			printf("%s=%s\n", VIDEO2_VAR, s);
#endif
#ifdef CONFIG_DISPLAY_LVDS_ENABLE
		if ((s = GetEnvVar(LDB_VAR, 1)) != NULL)
			printf("%s=%s\n", LDB_VAR, s);
#endif
#if defined(CONFIG_DISPLAY1_ENABLE) && defined(CONFIG_DISPLAY2_ENABLE)
		if ((s = GetEnvVar(FBPRIMARY_VAR, 1)) != NULL)
			printf("%s=%s\n", FBPRIMARY_VAR, s);
#endif
		puts("\nRemember to save the configuration with 'saveenv'\n\n");
	} else if (ret == USER_CANCEL)
		ret = 0;	/* Selection cancelled by user, not error */

	return ret;
}

U_BOOT_CMD(
	video, 1, 0, do_video,
	"Sets video related environment variables.",
	"\n"
	"  - Sets "VIDEO_VAR"/"VIDEO2_VAR" environment variables to one of the available\n"
	"    displays/video modes.\n"
	"  - Sets, if supported by the hardware, the "LDB_VAR" variable with the LVDS\n"
	"    bridge configuration.\n"
	"  - Sets, if supported by the hardware, the "FBPRIMARY_VAR" variable that\n"
	"    selects the primary display.\n"
);

#endif /* defined(CONFIG_CMD_BSP) && (defined(CONFIG_DISPLAY1_ENABLE) || defined(CONFIG_DISPLAY2_ENABLE)) */
