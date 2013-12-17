CPPFLAGS-y = \
	-DUBOOT \
	-I$(TOPDIR)/board/$(VENDOR)/common \
	-I$(TOPDIR)/board/$(VENDOR)/common/cmd_nvram \
	-I$(TOPDIR)/board/$(VENDOR)/common/cmd_nvram/lib/include

CPPFLAGS-$(CONFIG_CMD_BOOTSTREAM) += \
	-I$(TOPDIR)/board/$(VENDOR)/common/cmd_bootstream \
	-I$(TOPDIR)/drivers/mtd/nand

CPPFLAGS += $(CPPFLAGS-y)
CFLAGS += $(CPPFLAGS-y)
