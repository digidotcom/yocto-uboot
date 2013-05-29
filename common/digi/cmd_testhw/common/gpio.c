/*
 *  /common/digi/cmd_testhw/common/gpio.o
 *
 *  Copyright (C) 2010 by Digi International Inc.
 *  All rights reserved.
 *
 *  This program is free software; you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License version2  as published by
 *  the Free Software Foundation.
*/
/*
 *  !Revision:   $Revision$
 *  !Author:     Robert Hodaszi
 *  !Descr:      Implements commands:
 *                   gpio.r, gpio.w, gpio.c gpio.i
*/

#include <common.h>
#include <command.h>

#if defined(CONFIG_CMD_BSP) && \
    defined(CONFIG_UBOOT_CMD_BSP_TESTHW) && \
    defined(CONFIG_CCIMX51)

#include <dvt.h>                /* DVTError */

#include <cmd_testhw/testhw.h>

#ifdef CONFIG_CCIMX51

# include <asm-arm/arch-mx51/gpio.h>
# include <asm-arm/arch-mx51/mx51.h>
# include <asm-arm/arch-mx51/mx51_pins.h>

# define NUM_OF_GPIOS	(GPIO_NUM_PIN * GPIO_PORT_NUM + 9)	/* + 9 for the port 3's multiplexes */

# define PORT_LIST "1, 2, 3, 4, and 5 for the mulitplexed pins on port 3\n" \
			"     ( DI1_PIN12 (3/1), DI1_PIN13 (3/2), DI1_D0_CS (3/3), DI1_D1_CS (3/4), DISPB2_SER_DIN (3/5),\n"	\
			"     DISPB2_SER_DIO (3/6), DISPB2_SER_CLK (3/7), DISPB2_SER_RS (3/8) and GPIO_NAND (3/12) )\n"	\
			"     Multiplexed pins don't work simultaneously."

#endif // CONFIG_CCIMX51


/* ********** local functions ********** */

static long port_to_gpio(char *portStr, char *gpioStr)
{
	uint gpio_index;

	// Get GPIO index
	gpio_index = simple_strtoul(gpioStr, NULL, 10);

#if defined(CONFIG_CCIMX51)
	uint port = simple_strtoul(portStr, NULL, 10);

	if ( (port >= 1) && (port <= 4) )
	{
		if ( gpio_index >= GPIO_NUM_PIN )
		{
			goto err_inv_gpio;
		}

		gpio_index += (port - 1) * GPIO_NUM_PIN;
	}
	else if ( port == 5 )
	{
		if ( gpio_index >= 9 )
		{
			goto err_inv_gpio;
		}

		gpio_index += (port - 1) * GPIO_NUM_PIN;
	}
	else
	{
	    goto err_inv_port;
	}
#endif // CONFIG_CCIMX51

	return gpio_index;

err_inv_gpio:
	eprintf( "Invalid GPIO pin\n" );
	return -1;

err_inv_port:
	eprintf( "Unknown port\nAvailable ports: " PORT_LIST "\n");
	return -1;
}

static uint gpio_to_uboot_index(uint gpio_index)
{
	uint uboot_index = 0;

#ifdef CONFIG_CCIMX51
	uint port;
	uint index;

	port = GPIO_TO_PORT(gpio_index);
	index = GPIO_TO_INDEX(gpio_index);

	switch ( port )
	{
		case 0:
			if ( index <= 1 )
			{
				uboot_index = MX51_PIN_GPIO1_0 + (index << MUX_IO_I) + ( (index * 4) << PAD_I ) + ( (index * 4) << MUX_I );
			}
			else if ( index <= 3 )
			{
				uboot_index = MX51_PIN_GPIO1_2 + ((index - 2) << MUX_IO_I) + ( ((index - 2) * 4) << PAD_I ) + ( ((index - 2) * 4) << MUX_I );
			}
			else if ( index <= 9 )
			{
				uboot_index = MX51_PIN_GPIO1_4 + ((index - 4) << MUX_IO_I) + ( ((index - 4) * 4) << PAD_I ) + ( ((index - 4) * 4) << MUX_I );
			}
			else if ( index == 10 )
			{
				uboot_index = MX51_PIN_DISP2_DAT11;
			}
			else if ( index <= 18 )
			{
				uboot_index = MX51_PIN_USBH1_DATA0 + ((index - 11) << MUX_IO_I) + ( ((index - 11) * 4) << PAD_I ) + ( ((index - 11) * 4) << MUX_I );
			}
			else if ( index == 19 )
			{
				uboot_index = MX51_PIN_DISP2_DAT6;
			}
			else if ( index <= 24 )
			{
				uboot_index = MX51_PIN_UART2_RXD + ((index - 20) << MUX_IO_I) + ( ((index - 20) * 4) << PAD_I ) + ( ((index - 20) * 4) << MUX_I );
			}
			else if ( index <= 28 )
			{
				uboot_index = MX51_PIN_USBH1_CLK + ((index - 25) << MUX_IO_I) + ( ((index - 25) * 4) << PAD_I ) + ( ((index - 25) * 4) << MUX_I );
			}
			else if ( index <= 31 )
			{
				uboot_index = MX51_PIN_DISP2_DAT7 + ((index - 29) << MUX_IO_I) + ( ((index - 29) * 4) << PAD_I ) + ( ((index - 29) * 4) << MUX_I );
			}

			break;

		case 1:
			if ( index <= 8 )
			{
				uboot_index = MX51_PIN_EIM_D16 + (index << MUX_IO_I) + ( (index * 4) << PAD_I ) + ( (index * 4) << MUX_I );
			}
			else if ( index == 9 )
			{
				uboot_index = MX51_PIN_EIM_D27;
			}
			else if ( index <= 21 )
			{
				uboot_index = MX51_PIN_EIM_A16 + ((index - 10) << MUX_IO_I) + ( ((index - 10) * 4) << PAD_I ) + ( ((index - 10) * 4) << MUX_I );
			}
			else if ( index <= 31 )
			{
				uboot_index = MX51_PIN_EIM_EB2 + ((index - 22) << MUX_IO_I) + ( ((index - 22) * 4) << PAD_I ) + ( ((index - 22) * 4) << MUX_I );
			}

			break;

		case 2:
			if ( index == 0 )
			{
				uboot_index = MX51_PIN_DI1_PIN11;
			}
			else if ( index <= 2 )
			{
				uboot_index = MX51_PIN_EIM_LBA + ((index - 1) << MUX_IO_I) + ( ((index - 1) * 4) << PAD_I ) + ( ((index - 1) * 4) << MUX_I );
			}
			else if ( index <= 11 )
			{
				uboot_index = MX51_PIN_NANDF_WE_B + ((index - 3) << MUX_IO_I) + ( ((index - 3) * 4) << PAD_I ) + ( ((index - 3) * 4) << MUX_I );
			}
			else if ( index <= 13 )
			{
				uboot_index = MX51_PIN_CSI1_D8 + ((index - 12) << MUX_IO_I) + ( ((index - 12) * 4) << PAD_I ) + ( ((index - 12) * 4) << MUX_I );
			}
			else if ( index <= 15 )
			{
				uboot_index = MX51_PIN_CSI1_VSYNC + ((index - 14) << MUX_IO_I) + ( ((index - 14) * 4) << PAD_I ) + ( ((index - 14) * 4) << MUX_I );
			}
			else if ( index <= 31 )
			{
				uboot_index = MX51_PIN_NANDF_CS0 + ((index - 16) << MUX_IO_I) + ( ((index - 16) * 4) << PAD_I ) + ( ((index - 16) * 4) << MUX_I );
			}

			break;

		case 3:
			if ( index <= 8 )
			{
				uboot_index = MX51_PIN_NANDF_D8 + (index << MUX_IO_I) + ( (index * 4) << PAD_I ) + ( (index * 4) << MUX_I );
			}
			else if ( index <= 10 )
			{
				uboot_index = MX51_PIN_CSI2_D12 + ((index - 9) << MUX_IO_I) + ( ((index - 9) * 4) << PAD_I ) + ( ((index - 9) * 4) << MUX_I );
			}
			else if ( index <= 31 )
			{
				uboot_index = MX51_PIN_CSI2_D18 + ((index - 11) << MUX_IO_I) + ( ((index - 11) * 4) << PAD_I ) + ( ((index - 11) * 4) << MUX_I );
			}

			break;

		case 4:
			// MX51_PIN_DI1_PIN12 (port 3, pin 1) and MX51_PIN_DI1_PIN13 (port 3, pin 2)
			if ( index <= 1 )
			{
				uboot_index = MX51_PIN_DI1_PIN12 + (index << MUX_IO_I) + ( (index * 4) << PAD_I ) + ( (index * 4) << MUX_I );
			}
			// MX51_PIN_DI1_D0_CS (port 3, pin 3), MX51_PIN_DI1_D1_CS (port 3, pin 4), MX51_PIN_DISPB2_SER_DIN (port 3, pin 5)
			// MX51_PIN_DISPB2_SER_DIO (port 3, pin 6), MX51_PIN_DISPB2_SER_CLK (port 3, pin 7), and MX51_PIN_DISPB2_SER_RS (port 3, pin 8)
			else if ( index <= 7 )
			{
				uboot_index = MX51_PIN_DI1_D0_CS + ((index - 2) << MUX_IO_I) + ( ((index - 2) * 4) << PAD_I ) + ( ((index - 2) * 4) << MUX_I );
			}
			// MX51_PIN_GPIO_NAND (port 3, pin 12)
			else if ( index == 8 )
			{
				uboot_index = MX51_PIN_GPIO_NAND;
			}

			break;

		default:
			break;
	}

#endif // CONFIG_CCIMX51

	return uboot_index;
}

static int do_testhw_gpio_r(cmd_tbl_t *cmdtp, int flag, int argc, char* argv[])
{
	uint gpio_index;
	uint uboot_index;
	uchar value;

	if( (argc > 3) || (argc < 2) ) {
                eprintf( "Usage:\n%s\n", cmdtp->usage );
                return 1;
        }

	if ( argc == 2 )
	{
		// Get GPIO index
		gpio_index = simple_strtoul(argv[1], NULL, 10);
		if ( gpio_index >= NUM_OF_GPIOS )
		{
			eprintf( "Invalid GPIO pin\n" );
			return 1;
		}
	}
	else
	{
		long ret;

		// Get port and GPIO index
		ret = port_to_gpio(argv[1], argv[2]);
		if ( ret < 0 )
		{
			return 1;
		}

		gpio_index = (uint)ret;
	}

	uboot_index = gpio_to_uboot_index(gpio_index);

#if defined(CONFIG_CCIMX51)
	value = imx_gpio_get_pin(uboot_index);
#endif // CONFIG_CCIMX51

	printf( "Value of the GPIO pin %d: %d\n", gpio_index, value);

	return 0;
}

static int do_testhw_gpio_w(cmd_tbl_t *cmdtp, int flag, int argc, char* argv[])
{
	uint gpio_index;
	uint uboot_index;
	uint value_int;
	uchar value;
	char *value_arg;

	if( (argc > 4) || (argc < 3) ) {
                eprintf( "Usage:\n%s\n", cmdtp->usage );
                return 1;
        }

	if ( argc == 3 )
	{
		gpio_index = simple_strtoul(argv[1], NULL, 10);
		if ( gpio_index >= NUM_OF_GPIOS )
		{
			eprintf( "Invalid GPIO pin: %d\n", gpio_index);
			return 1;
		}
		value_arg = argv[2];
	}
	else
	{
		long ret;

		// Get port and GPIO index
		ret = port_to_gpio(argv[1], argv[2]);
		if ( ret < 0 )
		{
			return 1;
		}

		gpio_index = (uint)ret;

		value_arg = argv[3];
	}

	value_int = simple_strtoul(value_arg, NULL, 10);
	if ( value_int > 1 )
	{
		eprintf( "Value should be 0 or 1\n");
		return 1;
	}
	value = (uchar)value_int;

	uboot_index = gpio_to_uboot_index(gpio_index);

#if defined(CONFIG_CCIMX51)
	imx_gpio_set_pin(uboot_index, value);
#endif // CONFIG_CCIMX51

	printf( "GPIO %d was set to value %d\n", gpio_index, value);

	return 0;
}

static int do_testhw_gpio_c(cmd_tbl_t *cmdtp, int flag, int argc, char* argv[])
{
	uint gpio_index;
	uint uboot_index;
	char *config_arg;

	if( (argc > 4) || (argc < 3) ) {
                eprintf( "Usage:\n%s\n", cmdtp->usage );
                return 1;
        }

	if ( argc == 3 )
	{
		gpio_index = simple_strtoul(argv[1], NULL, 10);
		if ( gpio_index >= NUM_OF_GPIOS )
		{
			eprintf( "Invalid GPIO pin: %d\n", gpio_index);
			return 1;
		}
		config_arg = argv[2];
	}
	else
	{
		long ret;

		// Get port and GPIO index
		ret = port_to_gpio(argv[1], argv[2]);
		if ( ret < 0 )
		{
			return 1;
		}

		gpio_index = (uint)ret;

		config_arg = argv[3];
	}

	uboot_index = gpio_to_uboot_index(gpio_index);

#if defined(CONFIG_CCIMX51)
	// Config iomux for gpio operation
	mxc_request_iomux(uboot_index, IOMUX_CONFIG_GPIO);
	// Some pins are multiplexed inputs
	switch ( uboot_index )
	{
		// Port 3, pin 1 is shared
		case MX51_PIN_EIM_LBA:
			mxc_iomux_set_input(MUX_IN_GPIO3_IPP_IND_G_IN_1_SELECT_INPUT, INPUT_CTL_PATH0);
			break;
		case MX51_PIN_DI1_PIN12:
			mxc_iomux_set_input(MUX_IN_GPIO3_IPP_IND_G_IN_1_SELECT_INPUT, INPUT_CTL_PATH1);
			break;

		// Port 3, pin 2 is shared
		case MX51_PIN_EIM_CRE:
			mxc_iomux_set_input(MUX_IN_GPIO3_IPP_IND_G_IN_2_SELECT_INPUT, INPUT_CTL_PATH0);
			break;
		case MX51_PIN_DI1_PIN13:
			mxc_iomux_set_input(MUX_IN_GPIO3_IPP_IND_G_IN_2_SELECT_INPUT, INPUT_CTL_PATH1);
			break;

		// Port 3, pin 3 is shared
		case MX51_PIN_NANDF_WE_B:
			mxc_iomux_set_input(MUX_IN_GPIO3_IPP_IND_G_IN_3_SELECT_INPUT, INPUT_CTL_PATH0);
			break;
		case MX51_PIN_DI1_D0_CS:
			mxc_iomux_set_input(MUX_IN_GPIO3_IPP_IND_G_IN_3_SELECT_INPUT, INPUT_CTL_PATH1);
			break;

		// Port 3, pin 4 is shared
		case MX51_PIN_NANDF_RE_B:
			mxc_iomux_set_input(MUX_IN_GPIO3_IPP_IND_G_IN_4_SELECT_INPUT, INPUT_CTL_PATH0);
			break;
		case MX51_PIN_DI1_D1_CS:
			mxc_iomux_set_input(MUX_IN_GPIO3_IPP_IND_G_IN_4_SELECT_INPUT, INPUT_CTL_PATH1);
			break;

		// Port 3, pin 5 is shared
		case MX51_PIN_NANDF_ALE:
			mxc_iomux_set_input(MUX_IN_GPIO3_IPP_IND_G_IN_5_SELECT_INPUT, INPUT_CTL_PATH0);
			break;
		case MX51_PIN_DISPB2_SER_DIN:
			mxc_iomux_set_input(MUX_IN_GPIO3_IPP_IND_G_IN_5_SELECT_INPUT, INPUT_CTL_PATH1);
			break;

		// Port 3, pin 6 is shared
		case MX51_PIN_NANDF_CLE:
			mxc_iomux_set_input(MUX_IN_GPIO3_IPP_IND_G_IN_6_SELECT_INPUT, INPUT_CTL_PATH0);
			break;
		case MX51_PIN_DISPB2_SER_DIO:
			mxc_iomux_set_input(MUX_IN_GPIO3_IPP_IND_G_IN_6_SELECT_INPUT, INPUT_CTL_PATH1);
			break;

		// Port 3, pin 7 is shared
		case MX51_PIN_NANDF_WP_B:
			mxc_iomux_set_input(MUX_IN_GPIO3_IPP_IND_G_IN_7_SELECT_INPUT, INPUT_CTL_PATH0);
			break;
		case MX51_PIN_DISPB2_SER_CLK:
			mxc_iomux_set_input(MUX_IN_GPIO3_IPP_IND_G_IN_7_SELECT_INPUT, INPUT_CTL_PATH1);
			break;

		// Port 3, pin 8 is shared
		case MX51_PIN_NANDF_RB0:
			mxc_iomux_set_input(MUX_IN_GPIO3_IPP_IND_G_IN_8_SELECT_INPUT, INPUT_CTL_PATH0);
			break;
		case MX51_PIN_DISPB2_SER_RS:
			mxc_iomux_set_input(MUX_IN_GPIO3_IPP_IND_G_IN_8_SELECT_INPUT, INPUT_CTL_PATH1);
			break;

			// Port 3, pin 12 is shared
		case MX51_PIN_GPIO_NAND:
			mxc_iomux_set_input(MUX_IN_GPIO3_IPP_IND_G_IN_12_SELECT_INPUT, INPUT_CTL_PATH0);
			break;
		case MX51_PIN_CSI1_D8:
			mxc_iomux_set_input(MUX_IN_GPIO3_IPP_IND_G_IN_12_SELECT_INPUT, INPUT_CTL_PATH1);
			break;

		default:
			break;
	}

	if ( strcmp(config_arg, "in") == 0 )
	{
		imx_gpio_pin_cfg_dir(uboot_index, 0);
	}
	else if ( strcmp(config_arg, "out") == 0 )
	{
		imx_gpio_pin_cfg_dir(uboot_index, 1);
	}
	else if ( strcmp(config_arg, "pu") == 0 )
	{
		uint pad_reg = mxc_iomux_get_pad(uboot_index);
		pad_reg = (pad_reg & ~0xF0) | PAD_CTL_PKE_ENABLE | PAD_CTL_PUE_PULL | PAD_CTL_100K_PU;
		mxc_iomux_set_pad(uboot_index, pad_reg);
	}
	else if ( strcmp(config_arg, "pd") == 0 )
	{
		uint pad_reg = mxc_iomux_get_pad(uboot_index);
		pad_reg &= ~PAD_CTL_PKE_ENABLE;
		mxc_iomux_set_pad(uboot_index, pad_reg);
	}
	else
	{
		eprintf( "Invalid config\n");
		return 1;
	}
#endif // CONFIG_CCIMX51

	printf( "GPIO %d config modified to %s\n", gpio_index, config_arg);

	return 0;
}

static int do_testhw_gpio_i(cmd_tbl_t *cmdtp, int flag, int argc, char* argv[])
{
	uint gpio_index;
	uint uboot_index;
	uchar arg_index = 1;
	uint nr_of_changes;
	uint delay;
	int state;

	if( (argc > 5) || (argc < 4) ) {
                eprintf( "Usage:\n%s\n", cmdtp->usage );
                return 1;
        }

	if ( argc == 4 )
	{
		gpio_index = simple_strtoul(argv[1], NULL, 10);
		if ( gpio_index >= NUM_OF_GPIOS )
		{
			eprintf( "Invalid GPIO pin: %d\n", gpio_index);
			return 1;
		}
		arg_index = 2;
	}
	else
	{
		long ret;

		// Get port and GPIO index
		ret = port_to_gpio(argv[1], argv[2]);
		if ( ret < 0 )
		{
			return 1;
		}

		gpio_index = (uint)ret;

		arg_index = 3;
	}

	// Get number of changes
	nr_of_changes = simple_strtoul(argv[arg_index++], NULL, 10);

	// Get delay time
	delay = simple_strtoul(argv[arg_index], NULL, 10);

	uboot_index = gpio_to_uboot_index(gpio_index);

	// Get actual state
#if defined(CONFIG_CCIMX51)
	state = imx_gpio_get_pin(uboot_index);
#endif // CONFIG_CCIMX51

	printf("GPIO %d invert is starting, number of change: %d, delay: %dus\n", gpio_index, nr_of_changes, delay);

	while ( nr_of_changes-- )
	{
		state ^= 0x1;

#if defined(CONFIG_CCIMX51)
		imx_gpio_set_pin(uboot_index, state);
#endif // CONFIG_CCIMX51

		udelay(delay);
	}

	printf("End\n");

	return 0;
}

static int do_testhw_gpio(cmd_tbl_t *cmdtp, int flag, int argc, char* argv[])
{
	int len;

	if( (argc > 5) || (argc < 2) || (strlen(argv[0]) != 6) || (argv[0][4] != '.') ) {
                eprintf( "Usage:\n%s\n", cmdtp->usage );
                return 1;
        }

	len = strlen(argv[0]);
	if ( (len > 2) && (argv[0][len-2] == '.') )
	{
		switch ( argv[0][len-1] )
		{
		case 'r':
			return do_testhw_gpio_r(cmdtp, flag, argc, argv);
			break;

		case 'w':
			return do_testhw_gpio_w(cmdtp, flag, argc, argv);
			break;

		case 'c':
			return do_testhw_gpio_c(cmdtp, flag, argc, argv);
			break;

		case 'i':
			return do_testhw_gpio_i(cmdtp, flag, argc, argv);
			break;

		default:
			goto err_inv_func;
		}
	}
	else
	{
		goto err_inv_func;
	}

err_inv_func:
	eprintf( "Invalid GPIO function\nUsage:\n%s\n", cmdtp->usage);
	return 1;
}

/* ********** Test command implemented ********** */

U_BOOT_CMD( gpio, 5, 1, do_testhw_gpio,
	    "gpio    - gpio command\n",
	    ".r [<gpio_port>] <gpio_number>   -  Read GPIO pin's state\n"
	    "     .w [<gpio_port>] <gpio_number> <value>   - Set or clear the GPIO pin\n"
	    "     .c [<gpio_port>] <gpio_number> <in/out/pu/pd>   - Set the GPIO pin's configuration: input / output / pull-up enabled / disabled\n"
	    "     .i [<gpio_port>] <gpio_number> <number_of_changes> <delay(us)>   - Invert GPIO pin <number_of_changes> times with <delay(us)> delay time\n"
	    "     <gpio_port> could be: " PORT_LIST
);

#endif


