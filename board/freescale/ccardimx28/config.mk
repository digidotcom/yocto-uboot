#
# image should be loaded at 0x41008000
#
LDSCRIPT := $(SRCTREE)/board/$(VENDOR)/$(BOARD)/u-boot.lds

TEXT_BASE = 0x47800000
DIGI_BOARD = y