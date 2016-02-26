#
# Copyright (C) 2008-2011 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#
include $(TOPDIR)/rules.mk

ARCH:=mipsel
BOARD:=ramips
BOARDNAME:=MTK/Ralink APSoC
SUBTARGETS:=mt7621 mt7628 mt7620
FEATURES:=squashfs gpio

LINUX_COMMIT:=r45
LINUX_VER:=3.10.60
LINUX_VERSION:=$(LINUX_VER)-$(LINUX_COMMIT)

include $(INCLUDE_DIR)/target.mk
DEFAULT_PACKAGES += 

#ifeq ($(call qstrip,$(CONFIG_EXTERNAL_KERNEL_TREE))$(call qstrip,$(CONFIG_KERNEL_GIT_CLONE_URI)),)
LINUX_SITE:=https://github.com/stas2z/linux-3.10-witi/archive/
#endif
LINUX_SOURCE=$(LINUX_VERSION).tar.gz

define Target/Description
	Build firmware images for MTK/Ralink APSoC based boards.
endef

$(eval $(call BuildTarget))
