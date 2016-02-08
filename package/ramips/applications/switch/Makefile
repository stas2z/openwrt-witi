#
# Copyright (C) 2010-2013 hua.shao@mediatek.com
#
# MTK Property Software.
#

include $(TOPDIR)/rules.mk

PKG_NAME:=switch
PKG_RELEASE:=2

PKG_BUILD_DIR:=$(BUILD_DIR)/$(PKG_NAME)
PKG_KCONFIG:=RALINK_MT7621 CONFIG_MT7530_GSW
PKG_CONFIG_DEPENDS:=$(foreach c, $(PKG_KCONFIG),$(if $(CONFIG_$c),CONFIG_$(c)))


include $(INCLUDE_DIR)/package.mk
include $(INCLUDE_DIR)/kernel.mk

define Package/switch
  SECTION:=MTK Properties
  CATEGORY:=MTK Properties
  TITLE:=Command to config switch [Padavan]
  SUBMENU:=Applications
endef

define Package/switch/description
  An program to config MT7530 (MT7621) switch.
endef

define Build/Prepare
	mkdir -p $(PKG_BUILD_DIR)
	$(CP) ./src/* $(PKG_BUILD_DIR)/
	$(CP) ./patches/* $(PKG_BUILD_DIR)/
	find $(PKG_BUILD_DIR)/ -name "*.patch" -exec patch -p1 -N -i {} -d $(PKG_BUILD_DIR)/ \;
endef

TARGET_CFLAGS += \
	-I$(LINUX_DIR)/include \
	-I$(LINUX_DIR)/drivers/net/raeth \
	$(foreach c, $(PKG_KCONFIG),$(if $(CONFIG_$c),-DCONFIG_$(c)=$(CONFIG_$(c))))

MAKE_FLAGS += \
	$(foreach c, $(PKG_KCONFIG),$(if $(CONFIG_$c),CONFIG_$(c)=$(CONFIG_$(c))))

define Build/Configure
endef

define Package/switch/install
	$(INSTALL_DIR) $(1)/usr/bin
	$(INSTALL_DIR) $(1)/lib/network
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/switch $(1)/usr/bin
	$(INSTALL_BIN) ./files/switch.sh $(1)/lib/network
endef


$(eval $(call BuildPackage,switch))
