#
# (C) Copyright 2004
# Wolfgang Denk, DENX Software Engineering, wd@denx.de.
#
# See file CREDITS for list of people who contributed to this
# project.
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License as
# published by the Free Software Foundation; either version 2 of
# the License, or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place, Suite 330, Boston,
# MA 02111-1307 USA
#

include $(TOPDIR)/config.mk

ifneq ($(OBJTREE),$(SRCTREE))
$(shell mkdir -p $(obj)common $(obj)cpu $(obj)wireless)
endif

DIGI_DIR = $(TOPDIR)/common/digi

LIB	= $(obj)libtesthw.a

AOBJS	= \
	cpu/ns9360_asm.o

COBJS = cmd.o \
	common/dvt.o  \
	common/gpio.o \
	common/i2c.o  \
	common/nand.o \
	common/nor.o \
	common/time.o \
	wireless/vBPiper.o \
	wireless/wireless.o \
	wireless/gui_tst_wifi.o \
	wireless/adc081C021.o \
	wireless/calibration.o \
	wireless/commandswifi.o \
	cpu/ns921x.o  \
	cpu/ns921x_powersave.o \
	cpu/ns921x_rtc.o \
	cpu/ns921x_sysclock.o \
	cpu/ns9215_adc.o \
	cpu/ns9360.o \
	cpu/cc9m2443_lcd.o\
	cpu/cc9p9215_lcd.o\
	cpu/cc9m2443_adc.o\
	cpu/cc9m2443_serial.o

SRCS	:= $(AOBJS:.o=.S) $(COBJS:.o=.c)
OBJS	:= $(addprefix $(obj),$(AOBJS) $(COBJS))

CFLAGS += -I $(DIGI_DIR) -I $(DIGI_DIR)/cmd_nvram/lib/include \
	-I $(DIGI_DIR)/cmd_nvram -DUBOOT
CPPFLAGS += -I $(DIGI_DIR) -I $(DIGI_DIR)/cmd_nvram/lib/include \
	-I $(DIGI_DIR)/cmd_nvram -DUBOOT

all:	$(LIB)

$(LIB): $(obj).depend $(OBJS)
	$(AR) $(ARFLAGS) $@ $(OBJS)

#########################################################################

# defines $(obj).depend target
include $(SRCTREE)/rules.mk

sinclude $(obj).depend

#########################################################################
