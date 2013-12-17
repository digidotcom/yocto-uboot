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
include $(TOPDIR)/board/$(VENDOR)/config.mk

$(shell mkdir -p $(obj)cmd_nvram/lib/src)
$(if $(CONFIG_CMD_BOOTSTREAM), $(shell mkdir -p $(obj)cmd_bootstream))

LIB	= $(obj)lib$(VENDOR).o

COBJS-y	= cmd_bsp.o \
	  cmd_compat.o \
	  cmd_video.o \
	  cmd_chkimg.o \
	  cmd_nvram/mtd.o \
	  cmd_nvram/lib/src/nvram.o \
	  cmd_nvram/lib/src/nvram_cmdline.o \
	  cmd_nvram/cmd.o \
	  cmd_nvram/nvram_priv_uboot.o \
	  cmd_nvram/env.o \
	  cmd_nvram/partition.o \
	  vscanf.o \
	  atoi.o \
	  safe_strcat.o \
	  helper.o
COBJS-$(CONFIG_CMD_BOOTSTREAM) += cmd_bootstream/ncb.o \
				  cmd_bootstream/cmd_bootstream.o \
				  cmd_bootstream/mxsboot.o
COBJS-$(CONFIG_MXS_OTP) += cmd_mxsotp.o

COBJS	:= $(sort $(COBJS-y))
SRCS	:= $(AOBJS:.o=.S) $(COBJS:.o=.c)
OBJS	:= $(addprefix $(obj),$(AOBJS) $(COBJS))

$(LIB): $(obj).depend $(OBJS)
	$(call cmd_link_o_target, $(OBJS))

#########################################################################

# defines $(obj).depend target
include $(SRCTREE)/rules.mk

sinclude $(obj).depend

#########################################################################
