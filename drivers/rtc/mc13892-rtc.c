/*
 * Copyright (C) 2009, Digi International Inc.
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

#if defined(CONFIG_CMD_DATE)

#include <rtc.h>
#include <spi.h>
#include <imx_spi.h>
#include <asm/arch/imx_spi_pmic.h>

/* Register offsets */
#define MC13982_RTC_TIME	20
#define MC13982_RTC_ALARM	21
#define MC13982_RTC_DAY		22
#define MC13982_RTC_DAY_AL	23

/* Masks */
#define RTC_TOD_COUNTER_MSK	0x1ffff
#define RTC_CAL_COUNT_MSK	(0x1f << 17)
#define RTC_CAL_MODE		(0x3 << 22)
#define RTC_TOD_ALARM_MSK	0x1ffff
#define RTC_DISABLE_MSK		(1 << 23)
#define RTC_DAY_COUNTER_MSK	0x7fff
#define RTC_DAY_ALARM		0x7fff	

#define READ_RETRIES		5
#define WRITE_RETRIES		5

int rtc_get(struct rtc_time *rtc)
{
	u32 sec1, sec2, day;
	int i = 0;
	
	if (!pmicslv) {
		printf("*** ERROR: PMIC device not initialized yet\n");
		return -1;
	}

	do {
		/**
		 * Read the seconds twice, to avoid counter rollovers and also read the
		 * number of days between that reads to ensure that the values are correct. 
		 */
		sec1 = pmic_reg(pmicslv, MC13982_RTC_TIME, 0, 0) & RTC_TOD_COUNTER_MSK;
		day = pmic_reg(pmicslv, MC13982_RTC_DAY, 0, 0) & RTC_DAY_COUNTER_MSK;
		sec2 = pmic_reg(pmicslv, MC13982_RTC_TIME, 0, 0) & RTC_TOD_COUNTER_MSK;
	} while (sec1 != sec2 && i++ < READ_RETRIES);

	if (i == READ_RETRIES)
		return -1;

	to_tm(day * 86400 + sec1, rtc);

	rtc->tm_yday = 0;
	rtc->tm_isdst = 0;

	return 0;
}

int rtc_set(struct rtc_time *rtc)
{
	u32 sec, sec1, calib, day;
	int i = 0;
	
	if (!pmicslv) {
		printf("*** ERROR: PMIC device not initialized yet\n");
		return -1;
	}

	/* Store calibration mode and count to write it back */
	calib = pmic_reg(pmicslv, MC13982_RTC_TIME, 0, 0);
	calib &= (RTC_CAL_COUNT_MSK | RTC_CAL_MODE);

	sec = mktime(rtc->tm_year, rtc->tm_mon, rtc->tm_mday,
		     rtc->tm_hour, rtc->tm_min, rtc->tm_sec);
	day = sec / 86400;
	sec %= 86400;

	do {
		/**
		 * Write the number of seconds, then the number of days and verify that 
		 * the seconds have not changed, what could cause a day change...
		 */
		pmic_reg(pmicslv, MC13982_RTC_TIME, sec | calib, 1);
		pmic_reg(pmicslv, MC13982_RTC_DAY, day, 1);
		sec1 = pmic_reg(pmicslv, MC13982_RTC_TIME, 0, 0) & RTC_TOD_COUNTER_MSK;
	} while (sec != sec1 && i++ < WRITE_RETRIES);	    

	return i == WRITE_RETRIES ? -1 : 0;
}

void rtc_reset(void)
{
}

#endif /* defined(CONFIG_CMD_DATE) */
