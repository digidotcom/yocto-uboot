DIGI_BOARD = y

ifneq ($(CONFIG_SPL_BUILD),y)
ALL-y += $(obj)u-boot.sb
endif
