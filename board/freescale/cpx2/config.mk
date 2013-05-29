#
# image should be loaded at 0x43800000
#
LDSCRIPT := $(SRCTREE)/board/$(VENDOR)/$(BOARD)/u-boot.lds

TEXT_BASE = 0x43800000
DIGI_BOARD = y