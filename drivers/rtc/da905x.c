/*
 * Copyright (C) 2011, Digi International Inc.
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
#include <config.h>

#if defined(CONFIG_CMD_DATE)

#include <rtc.h>
#include <i2c.h>

#undef DEBUG_RTC
#ifdef DEBUG_RTC
#define DBG(fmt,args...) printf(fmt ,##args)
#else
#define DBG(fmt,args...)
#endif

#define DEV_NAME			"ds905x-rtc"

/* Register offsets */
#define DA9052_COUNTS_REG		111
#define DA9052_COUNTMI_REG		112
#define DA9052_COUNTH_REG		113
#define DA9052_COUNTD_REG		114
#define DA9052_COUNTMO_REG		115
#define DA9052_COUNTY_REG		116

/* Masks */
#define DA9052_COUNTS_MONITOR		(1<<6)
#define DA9052_COUNTS_COUNTSEC		(63<<0)
#define DA9052_COUNTMI_COUNTMIN		(63<<0)
#define DA9052_COUNTH_COUNTHOUR		(31<<0)
#define DA9052_COUNTD_COUNTDAY		(31<<0)
#define DA9052_COUNTMO_COUNTMONTH	(15<<0)
#define DA9052_COUNTY_COUNTYEAR		(63<<0)

extern u8 da905x_i2c_addr;
extern int pmic_wr_reg(uchar reg, uchar data);
extern int pmic_rd_reg(uchar reg);

#define rtc_read			pmic_rd_reg
#define rtc_write			pmic_wr_reg

int rtc_get(struct rtc_time *rtc)
{
	uchar sec, min, hour, mday, month, year;

	/* Reading counts register latches the current RTC calendar count.
	 * This guranties the coherence of the read during aprox 500 ms */
	sec = rtc_read (DA9052_COUNTS_REG);
	if (!(sec & DA9052_COUNTS_MONITOR)) {
		puts("*** ERROR: " DEV_NAME " - incorrect date/time (Power was lost)\n");
	}
	min = rtc_read (DA9052_COUNTMI_REG);
	hour = rtc_read (DA9052_COUNTH_REG);
	mday = rtc_read (DA9052_COUNTD_REG);
	month = rtc_read (DA9052_COUNTMO_REG);
	year = rtc_read (DA9052_COUNTY_REG);

	DBG ("Get RTC year: %02d mon: %02d mday: %02d "
		"hr: %02d min: %02d sec: %02d\n",
		year, month, mday, hour, min, sec);

	rtc->tm_sec  = sec & DA9052_COUNTS_COUNTSEC;
	rtc->tm_min  = min & DA9052_COUNTMI_COUNTMIN;
	rtc->tm_hour = hour & DA9052_COUNTH_COUNTHOUR;
	rtc->tm_mday = mday & DA9052_COUNTD_COUNTDAY;
	rtc->tm_mon  = month & DA9052_COUNTMO_COUNTMONTH;
	rtc->tm_year = (year & DA9052_COUNTY_COUNTYEAR) + 2000;

	/* Compute the the day of week, not available in the RTC registers */
	GregorianDay(rtc);

	DBG ("Get DATE: %4d-%02d-%02d   TIME: %2d:%02d:%02d\n",
		rtc->tm_year, rtc->tm_mon, rtc->tm_mday,
		rtc->tm_hour, rtc->tm_min, rtc->tm_sec);

	return 0;
}

int rtc_set(struct rtc_time *rtc)
{
	DBG ("Set DATE: %4d-%02d-%02d   TIME: %2d:%02d:%02d\n",
		rtc->tm_year, rtc->tm_mon, rtc->tm_mday,
		rtc->tm_hour, rtc->tm_min, rtc->tm_sec);

	/* Set seconds and activate power fail monitor bit */
	rtc_write (DA9052_COUNTS_REG,
		   (rtc->tm_sec & DA9052_COUNTS_COUNTSEC) | DA9052_COUNTS_MONITOR);
	rtc_write (DA9052_COUNTMI_REG, rtc->tm_min & DA9052_COUNTMI_COUNTMIN);
	rtc_write (DA9052_COUNTH_REG, rtc->tm_hour & DA9052_COUNTH_COUNTHOUR);
	rtc_write (DA9052_COUNTD_REG, rtc->tm_mday & DA9052_COUNTD_COUNTDAY);
	rtc_write (DA9052_COUNTMO_REG, rtc->tm_mon & DA9052_COUNTMO_COUNTMONTH);
	rtc_write (DA9052_COUNTY_REG, (rtc->tm_year - 2000) & DA9052_COUNTY_COUNTYEAR);

	/* The write in the year register latches the registers
	 * DA9052_COUNTS_COUNTSEC to DA9052_COUNTY_COUNTYEAR into the
	 * RTC calendar count */

	return 0;
}

void rtc_reset(void)
{
}

#endif /* defined(CONFIG_CMD_DATE) */
