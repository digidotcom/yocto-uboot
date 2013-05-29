#include <common.h>
#include <usb.h>
#include <asm/io.h>
#include <usb/ehci-fsl.h>
#include <asm/arch/mx51.h>

#include "ehci.h"
#include "ehci-core.h"

#include "asm/arch/arc_otg.h"

void isp1504_set_vbus_power(int on);

void usb_iomux_setup(void)
{
	mxc_request_iomux(MX51_PIN_USBH1_STP, IOMUX_CONFIG_ALT0);
	mxc_iomux_set_pad(MX51_PIN_USBH1_STP, (PAD_CTL_SRE_FAST | PAD_CTL_DRV_HIGH | PAD_CTL_PUE_KEEPER |
		PAD_CTL_PKE_ENABLE | PAD_CTL_HYS_ENABLE));

	mxc_request_iomux(MX51_PIN_USBH1_CLK, IOMUX_CONFIG_ALT0);
	mxc_iomux_set_pad(MX51_PIN_USBH1_CLK, (PAD_CTL_SRE_FAST | PAD_CTL_DRV_HIGH | PAD_CTL_PUE_KEEPER |
		PAD_CTL_PKE_ENABLE | PAD_CTL_HYS_ENABLE | PAD_CTL_DDR_INPUT_CMOS));

	mxc_request_iomux(MX51_PIN_USBH1_DIR, IOMUX_CONFIG_ALT0);
	mxc_iomux_set_pad(MX51_PIN_USBH1_DIR, (PAD_CTL_SRE_FAST | PAD_CTL_DRV_HIGH | PAD_CTL_PUE_KEEPER |
		PAD_CTL_PKE_ENABLE | PAD_CTL_HYS_ENABLE | PAD_CTL_DDR_INPUT_CMOS));

	mxc_request_iomux(MX51_PIN_USBH1_NXT, IOMUX_CONFIG_ALT0);
	mxc_iomux_set_pad(MX51_PIN_USBH1_NXT, (PAD_CTL_SRE_FAST | PAD_CTL_DRV_HIGH | PAD_CTL_PUE_KEEPER |
		PAD_CTL_PKE_ENABLE | PAD_CTL_HYS_ENABLE | PAD_CTL_DDR_INPUT_CMOS));

	mxc_request_iomux(MX51_PIN_USBH1_DATA0, IOMUX_CONFIG_ALT0);
	mxc_iomux_set_pad(MX51_PIN_USBH1_DATA0, (PAD_CTL_SRE_FAST | PAD_CTL_DRV_HIGH | PAD_CTL_100K_PU |
		PAD_CTL_PUE_KEEPER | PAD_CTL_PKE_ENABLE | PAD_CTL_HYS_ENABLE));

	mxc_request_iomux(MX51_PIN_USBH1_DATA1, IOMUX_CONFIG_ALT0);
	mxc_iomux_set_pad(MX51_PIN_USBH1_DATA1, (PAD_CTL_SRE_FAST | PAD_CTL_DRV_HIGH | PAD_CTL_100K_PU |
		PAD_CTL_PUE_KEEPER | PAD_CTL_PKE_ENABLE | PAD_CTL_HYS_ENABLE));

	mxc_request_iomux(MX51_PIN_USBH1_DATA2, IOMUX_CONFIG_ALT0);
	mxc_iomux_set_pad(MX51_PIN_USBH1_DATA2, (PAD_CTL_SRE_FAST | PAD_CTL_DRV_HIGH | PAD_CTL_100K_PU |
		PAD_CTL_PUE_KEEPER | PAD_CTL_PKE_ENABLE | PAD_CTL_HYS_ENABLE));

	mxc_request_iomux(MX51_PIN_USBH1_DATA3, IOMUX_CONFIG_ALT0);
	mxc_iomux_set_pad(MX51_PIN_USBH1_DATA3, (PAD_CTL_SRE_FAST | PAD_CTL_DRV_HIGH | PAD_CTL_100K_PU |
		PAD_CTL_PUE_KEEPER | PAD_CTL_PKE_ENABLE | PAD_CTL_HYS_ENABLE));

	mxc_request_iomux(MX51_PIN_USBH1_DATA4, IOMUX_CONFIG_ALT0);
	mxc_iomux_set_pad(MX51_PIN_USBH1_DATA4, (PAD_CTL_SRE_FAST | PAD_CTL_DRV_HIGH | PAD_CTL_100K_PU |
		PAD_CTL_PUE_KEEPER | PAD_CTL_PKE_ENABLE | PAD_CTL_HYS_ENABLE));

	mxc_request_iomux(MX51_PIN_USBH1_DATA5, IOMUX_CONFIG_ALT0);
	mxc_iomux_set_pad(MX51_PIN_USBH1_DATA5, (PAD_CTL_SRE_FAST | PAD_CTL_DRV_HIGH | PAD_CTL_100K_PU |
		PAD_CTL_PUE_KEEPER | PAD_CTL_PKE_ENABLE | PAD_CTL_HYS_ENABLE));

	mxc_request_iomux(MX51_PIN_USBH1_DATA6, IOMUX_CONFIG_ALT0);
	mxc_iomux_set_pad(MX51_PIN_USBH1_DATA6, (PAD_CTL_SRE_FAST | PAD_CTL_DRV_HIGH | PAD_CTL_100K_PU |
		PAD_CTL_PUE_KEEPER | PAD_CTL_PKE_ENABLE | PAD_CTL_HYS_ENABLE));

	mxc_request_iomux(MX51_PIN_USBH1_DATA7, IOMUX_CONFIG_ALT0);
	mxc_iomux_set_pad(MX51_PIN_USBH1_DATA7, (PAD_CTL_SRE_FAST | PAD_CTL_DRV_HIGH | PAD_CTL_100K_PU |
		PAD_CTL_PUE_KEEPER | PAD_CTL_PKE_ENABLE | PAD_CTL_HYS_ENABLE));

	mxc_request_iomux(MX51_PIN_DISPB2_SER_RS, IOMUX_CONFIG_GPIO);
	mxc_iomux_set_pad(MX51_PIN_DISPB2_SER_RS, (PAD_CTL_SRE_FAST | PAD_CTL_DRV_HIGH | PAD_CTL_47K_PU |
		PAD_CTL_PUE_KEEPER | PAD_CTL_PKE_ENABLE | PAD_CTL_HYS_ENABLE));

	imx_gpio_set_pin(MX51_PIN_DISPB2_SER_RS, 0);
	imx_gpio_pin_cfg_dir(MX51_PIN_DISPB2_SER_RS, 1);
	udelay(1000);
	imx_gpio_set_pin(MX51_PIN_DISPB2_SER_RS, 1);
}

int ehci_hcd_init(void)
{
	usb_iomux_setup();

	/* Enable USB AHB clock (CG13) */
	__REG(CCM_BASE_ADDR + CLKCTL_CCGR2) |= 0x3 << 26;

	/* Set USBH1_STOP to GPIO and toggle it */
	mxc_request_iomux(MX51_PIN_USBH1_STP, IOMUX_CONFIG_GPIO);
	mxc_iomux_set_pad(MX51_PIN_USBH1_STP, 0x000000C5);
	/* ??? PAD config ??? */
	imx_gpio_pin_cfg_dir(MX51_PIN_USBH1_STP, 1);
	imx_gpio_set_pin(MX51_PIN_USBH1_STP, 1);

	udelay(100000);

	/* Enable USBOH3_CLK */
	__REG(CCM_BASE_ADDR + CLKCTL_CCGR2) |= 0x3 << 28;

	/* Stop then Reset */
	UH1_USBCMD &= ~UCMD_RUN_STOP;
	while (UH1_USBCMD & UCMD_RUN_STOP) ;

	UH1_USBCMD |= UCMD_RESET;
	while (UH1_USBCMD & UCMD_RESET) ;

	/* Select the clock from external PHY */
	USB_CTRL_1 |= USB_CTRL_UH1_EXT_CLK_EN;

	/* select ULPI PHY PTS=2 */
	UH1_PORTSC1 = (UH1_PORTSC1 & ~PORTSC_PTS_MASK) | PORTSC_PTS_ULPI;

	USBCTRL &= ~UCTRL_H1WIE; /* HOST1 wakeup intr disable */
	USBCTRL &= ~UCTRL_H1UIE; /* Host1 ULPI interrupt disable */
	USBCTRL |= UCTRL_H1PM; /* HOST1 power mask */
	USB_PHY_CTR_FUNC |= USB_UH1_OC_DIS; /* OC is not used */

	/* Interrupt Threshold Control:Immediate (no threshold) */
	UH1_USBCMD &= UCMD_ITC_NO_THRESHOLD;

	UH1_USBCMD |= UCMD_RESET;       /* reset the controller */

	/* allow controller to reset, and leave time for
	* the ULPI transceiver to reset too.
	*/
	udelay(100000);

	/* setback USBH1_STP to be function */
	mxc_request_iomux(MX51_PIN_USBH1_STP, IOMUX_CONFIG_ALT0);
	mxc_iomux_set_pad(MX51_PIN_USBH1_STP, PAD_CTL_SRE_FAST |
			  PAD_CTL_DRV_HIGH | PAD_CTL_ODE_OPENDRAIN_NONE |
			  PAD_CTL_PUE_KEEPER | PAD_CTL_PKE_ENABLE |
			  PAD_CTL_HYS_ENABLE | PAD_CTL_DDR_INPUT_CMOS |
			  PAD_CTL_DRV_VOT_LOW);

	/* disable remote wakeup irq */
	USBCTRL &= ~UCTRL_H1WIE;

	UH1_USBMODE |= USBMODE_CM_HOST;

	if ( UH1_HCSPARAMS & HCSPARAMS_PPC )
	{
		UH1_PORTSC1 |= PORTSC_PORT_POWER;
	}

	isp1504_set_vbus_power(1);

	hccr = (struct ehci_hccr *)(CONFIG_SYS_USB_EHCI_REGS_BASE);
	hcor = (struct ehci_hcor *)((uint32_t) hccr
			+ HC_LENGTH(ehci_readl(&hccr->cr_capbase)));

	return 0;
}

int ehci_hcd_stop(void)
{
	return 0;
}


/* ISP 1504 register addresses */
#define ISP1504_VID_LOW		0x00	/* Vendor ID low */
#define ISP1504_VID_HIGH	0x01	/* Vendor ID high */
#define ISP1504_PID_LOW		0x02	/* Product ID low */
#define ISP1504_PID_HIGH	0x03	/* Product ID high */
#define ISP1504_FUNC		0x04	/* Function Control */
#define ISP1504_ITFCTL		0x07	/* Interface Control */
#define ISP1504_OTGCTL		0x0A	/* OTG Control */

/* add to above register address to access Set/Clear functions */
#define ISP1504_REG_SET		0x01
#define ISP1504_REG_CLEAR	0x02

/* 1504 OTG Control Register bits */
#define USE_EXT_VBUS_IND	(1 << 7)	/* Use ext. Vbus indicator */
#define DRV_VBUS_EXT		(1 << 6)	/* Drive Vbus external */
#define DRV_VBUS		(1 << 5)	/* Drive Vbus */
#define CHRG_VBUS		(1 << 4)	/* Charge Vbus */
#define DISCHRG_VBUS		(1 << 3)	/* Discharge Vbus */
#define DM_PULL_DOWN		(1 << 2)	/* enable DM Pull Down */
#define DP_PULL_DOWN		(1 << 1)	/* enable DP Pull Down */
#define ID_PULL_UP		(1 << 0)	/* enable ID Pull Up */

/* 1504 OTG Function Control Register bits */
#define SUSPENDM		(1 << 6)	/* places the PHY into
						   low-power mode      */
#define DRV_RESET		(1 << 5)	/* Active HIGH transceiver
						   reset                  */

/*!
 * read ULPI register 'reg' thru VIEWPORT register 'view'
 *
 * @param       reg   register to read
 * @param       view  the ULPI VIEWPORT register address
 * @return	return isp1504 register value
 */
#if 0
static u8 isp1504_read(int reg, volatile u32 *view)
{
	u32 data;

	/* make sure interface is running */
	if (!(__raw_readl(view) && ULPIVW_SS)) {
		__raw_writel(ULPIVW_WU, view);
		do {		/* wait for wakeup */
			data = __raw_readl(view);
		} while (data & ULPIVW_WU);
	}

	/* read the register */
	__raw_writel((ULPIVW_RUN | (reg << ULPIVW_ADDR_SHIFT)), view);

	do {			/* wait for completion */
		data = __raw_readl(view);
	} while (data & ULPIVW_RUN);

	return (u8) (data >> ULPIVW_RDATA_SHIFT) & ULPIVW_RDATA_MASK;
}
#endif

/*!
 * set bits into OTG ISP1504 register 'reg' thru VIEWPORT register 'view'
 *
 * @param       bits  set value
 * @param	reg   which register
 * @param       view  the ULPI VIEWPORT register address
 */
static void isp1504_set(u8 bits, int reg, volatile u32 *view)
{
	u32 data;

	/* make sure interface is running */
	if (!(__raw_readl(view) && ULPIVW_SS)) {
		__raw_writel(ULPIVW_WU, view);
		do {		/* wait for wakeup */
			data = __raw_readl(view);
		} while (data & ULPIVW_WU);
	}

	__raw_writel((ULPIVW_RUN | ULPIVW_WRITE |
		      ((reg + ISP1504_REG_SET) << ULPIVW_ADDR_SHIFT) |
		      ((bits & ULPIVW_WDATA_MASK) << ULPIVW_WDATA_SHIFT)),
		     view);

	while (__raw_readl(view) & ULPIVW_RUN)	/* wait for completion */
		continue;
}

/*!
 * clear bits in OTG ISP1504 register 'reg' thru VIEWPORT register 'view'
 *
 * @param       bits  bits to clear
 * @param	reg   in this register
 * @param       view  the ULPI VIEWPORT register address
 */
static void isp1504_clear(u8 bits, int reg, volatile u32 *view)
{
	__raw_writel((ULPIVW_RUN | ULPIVW_WRITE |
		      ((reg + ISP1504_REG_CLEAR) << ULPIVW_ADDR_SHIFT) |
		      ((bits & ULPIVW_WDATA_MASK) << ULPIVW_WDATA_SHIFT)),
		     view);

	while (__raw_readl(view) & ULPIVW_RUN)	/* wait for completion */
		continue;
}

//extern int gpio_usbotg_hs_active(void);

static void isp1508_fix(volatile u32 *view)
{
	/*if (!machine_is_mx31_3ds())
		gpio_usbotg_hs_active();*/

	/* Set bits IND_PASS_THRU and IND_COMPL */
	isp1504_set(0x60, ISP1504_ITFCTL, view);

	/* Set bit USE_EXT_VBUS_IND */
	isp1504_set(USE_EXT_VBUS_IND, ISP1504_OTGCTL, view);
}

void isp1504_set_vbus_power(int on)
{
	// Set VBUS power
	if (on) {
		isp1504_set(DRV_VBUS_EXT |	/* enable external Vbus */
			    DRV_VBUS |	/* enable internal Vbus */
			    USE_EXT_VBUS_IND |	/* use external indicator */
			    CHRG_VBUS,	/* charge Vbus */
			    ISP1504_OTGCTL, &UH1_ULPIVIEW);

	} else {
		isp1508_fix(&UH1_ULPIVIEW);

		isp1504_clear(DRV_VBUS_EXT |	/* disable external Vbus */
			      DRV_VBUS,	/* disable internal Vbus */
			      ISP1504_OTGCTL, &UH1_ULPIVIEW);

		isp1504_set(USE_EXT_VBUS_IND |	/* use external indicator */
			    DISCHRG_VBUS,	/* discharge Vbus */
			    ISP1504_OTGCTL, &UH1_ULPIVIEW);
	}

}
