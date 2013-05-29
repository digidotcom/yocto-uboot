/*common/digi/cmd_testhw/cpu/cc9p9215_lcd.c
 * Init part taken from
 * http://www.trulydisplays.com/tft/specs/TFT IC Spec for -148W Himax HX8347.pdf
 */

#include <common.h>

#if (CONFIG_COMMANDS & CFG_CMD_BSP && \
		defined(CONFIG_UBOOT_CMD_BSP_TESTHW) &&    \
		(defined(CONFIG_CC9P9215) || defined(CONFIG_CCW9P9215)))

#include <asm-arm/io.h>
#include <asm/arch-ns9xxx/ns921x_gpio.h>
#include <asm/arch-ns9xxx/io.h>
#include <cmd_testhw/testhw.h>

#define LOADADDR  0x200000 /*same like loadaddr */
#define LCD_CS_OFFSET	0x40000000
#define	LCD_POINTER	LCD_CS_OFFSET
#define LCD_DATA	(LCD_CS_OFFSET + 2)
#define RESET_GPIO	86
static void write_lcd_reg(uint addr, uint data)
{
	writew(addr, LCD_POINTER);
	writew(data, LCD_DATA);
}

static void mdelay(int count)
{
	for(;count != 0;count--)
		udelay(1000);
}

static int do_testhw_lcd( int argc, char* argv[] )
{
const char szCmd [60];
int ret, x, y;

if( argc != 1 ) {
	eprintf( "Usage: lcd image_ name \n" );
	goto error;
}

sprintf((char *)szCmd, "tftp 0x%x %s", LOADADDR, argv[ 0 ] );
ret = (run_command( szCmd, 0 ) >=  0);
if(!ret)
	goto error;

	/* setup chipselect */
	writel(0x3,   0xa0700080);
	writel(0x181, 0xa0700200);
	writel(0x4,   0xa0700204);
	writel(0x0,   0xa0700208);
	writel(0x0,   0xa070020c);
	writel(0x0,   0xa0700210);
	writel(0x0,   0xa0700214);
	writel(0x0,   0xa0700218);

	/* RESET */
	gpio_cfg_set(RESET_GPIO, GPIO_CFG_FUNC_GPIO | GPIO_CFG_OUTPUT);
	gpio_ctrl_set(RESET_GPIO, 0);
	mdelay(150);   /* After Inter-MicroP Program (load OTP) */
	gpio_ctrl_set(RESET_GPIO, 1);

	mdelay(50);   /* After Inter-MicroP Program (load OTP) */
	writew(0x22, LCD_POINTER);

	/* Gamma for CMO 2.8 */
	write_lcd_reg(0x46,0x94);
	write_lcd_reg(0x47,0x41);
	write_lcd_reg(0x48,0x00);
	write_lcd_reg(0x49,0x33);
	write_lcd_reg(0x4A,0x23);
	write_lcd_reg(0x4B,0x45);
	write_lcd_reg(0x4C,0x44);
	write_lcd_reg(0x4D,0x77);
	write_lcd_reg(0x4E,0x12);
	write_lcd_reg(0x4F,0xCC);
	write_lcd_reg(0x50,0x46);
	write_lcd_reg(0x51,0x82);

	/* 240x320 window setting */
	write_lcd_reg(0x02,0x00); /* Column address start2 */
	write_lcd_reg(0x03,0x00); /* Column address start1 */
	write_lcd_reg(0x04,0x01); /* Column address end2 */
	write_lcd_reg(0x05,0x3F); /* Column address end1 */
	write_lcd_reg(0x06,0x00); /* Row address start2 */
	write_lcd_reg(0x07,0x00); /* Row address start1 */
	write_lcd_reg(0x08,0x00); /* Row address end2 */
	write_lcd_reg(0x09,0xEF); /* Row address end1 */

	/* Display Setting */
	write_lcd_reg(0x01,0x06); /* IDMON=0, INVON=1, NORON=1, PTLON=0 */
	write_lcd_reg(0x16,0x68); /* MY=0, MX=0, MV=0, ML=1, BGR=0, TEON=0 */

	write_lcd_reg(0x23,0x95); /* N_DC=1001 0101 */
	write_lcd_reg(0x24,0x95); /* P_DC=1001 0101 */
	write_lcd_reg(0x25,0xFF); /* I_DC=1111 1111 */

	write_lcd_reg(0x27,0x02); /* N_BP=0000 0110 */
	write_lcd_reg(0x28,0x02); /* N_FP=0000 0110 */
	write_lcd_reg(0x29,0x02); /* P_BP=0000 0110 */
	write_lcd_reg(0x2A,0x02); /* P_FP=0000 0110 */
	write_lcd_reg(0x2C,0x02); /* I_BP=0000 0110 */
	write_lcd_reg(0x2D,0x02); /* I_FP=0000 0110 */
	write_lcd_reg(0x3A,0x01); /* N_RTN=0000, N_NW=001 */
	write_lcd_reg(0x3B,0x01); /* P_RTN=0000, P_NW=000 */
	write_lcd_reg(0x3C,0xF0); /* I_RTN=1111, I_NW=000 */
	write_lcd_reg(0x3D,0x00); /* DIV=00 */
	mdelay(20);
	write_lcd_reg(0x35,0x38); /* EQS=38h */
	write_lcd_reg(0x36,0x78); /* EQP=78h */
	write_lcd_reg(0x3E,0x38); /* SON=38h */
	write_lcd_reg(0x40,0x0F); /* GDON=0Fh */
	write_lcd_reg(0x41,0xF0); /* GDOFF */

	/* Power Supply Setting */
	write_lcd_reg(0x19,0x49); /* OSCADJ=10 0000, OSD_EN=1 60Hz */
	write_lcd_reg(0x93,0x0F); /* RADJ=1100 */
	mdelay(10);
	write_lcd_reg(0x20,0x40); /* BT=0100 */
	write_lcd_reg(0x1D,0x07); /* VC1=111 */
	write_lcd_reg(0x1E,0x00); /* VC3=000 */
	write_lcd_reg(0x1F,0x04); /* VRH=0100 */

	/* VCOM Setting for CMO 2.8” Panel */
	write_lcd_reg(0x44,0x40);     /* VCM=101 0000 */
	write_lcd_reg(0x45,0x12);     /* VDV=1 0001 */
	mdelay(10);
	write_lcd_reg(0x1C,0x04);     /* AP=100 */
	mdelay(20);
	write_lcd_reg(0x43,0x80);     /* Set VCOMG=1 */
	mdelay(5);
	write_lcd_reg(0x1B,0x08);     /* GASENB=0, PON=1, DK=1, XDK=0,
					 DDVDH_TRI=0, STB=0 */
	mdelay(40);
	write_lcd_reg(0x1B,0x10);     /* GASENB=0, PON=1, DK=0, XDK=0,
					 DDVDH_TRI=0, STB=0*/
	mdelay(40);
	/* Display ON Setting */
	write_lcd_reg(0x90,0x7F);     /* SAP=0111 1111 */
	write_lcd_reg(0x26,0x04);     /* GON=0, DTE=0, D=01 */
	mdelay(40);
	write_lcd_reg(0x26,0x24);     /* GON=1, DTE=0, D=01 */
	write_lcd_reg(0x26,0x2C);     /* GON=1, DTE=0, D=11 */
	mdelay(40);
	write_lcd_reg(0x26,0x3C);     /* GON=1, DTE=1, D=11 */
	write_lcd_reg(0x57,0x02);     /* GON=1, DTE=1, D=11 */
	write_lcd_reg(0x55,0x00);     /* GON=1, DTE=1, D=11 */
	write_lcd_reg(0x57,0x00);     /* GON=1, DTE=1, D=11 */

	writew(0x22, LCD_POINTER);
	u16 data;
	u16 *pdata = (u16 *)LOADADDR;
	for (y = 239; y >= 0; y--) {
		for (x = 319; x >= 0; x--) {

			data = *(pdata + (y * 320) + x);
			writew(data, LCD_DATA);
		}
	}

	return 1;
error:
return 0;
}
/* ********** Test command implemented ********** */

TESTHW_CMD( lcd, "loads given image via tftp into framebuffer.");
#endif
