#
# Copyright (C) 2009 OpenWrt.org
#

SUBTARGET:=mt7621
BOARDNAME:=MT7621 based boards
ARCH_PACKAGES:=ramips_1004kc
FEATURES+=usb
CPU_TYPE:=1004kc
CPU_SUBTYPE:=dsp

DEFAULT_PACKAGES +=

define Target/Description
	Build firmware images for Ralink MT7621 based boards.
endef

