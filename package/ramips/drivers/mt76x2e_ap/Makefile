# All rights reserved.
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.

include $(TOPDIR)/rules.mk
include $(INCLUDE_DIR)/kernel.mk

PKG_NAME:=mt76x2e_ap
PKG_VERSION:=3.0.4.0P1

include $(INCLUDE_DIR)/package.mk

define KernelPackage/mt76x2e_ap
  CATEGORY:=MTK Properties
  TITLE:=MTK MT76x2e 3.0.4.0P1 AP driver
  FILES:=$(LINUX_DIR)/drivers/net/wireless/ralink/mt76x2_ap/mt76x2e.ko
  DEPENDS:=+kmod-cfg80211
  AUTOLOAD:=$(call AutoLoad,91,mt76x2e)
  SUBMENU:=Drivers
endef

define KernelPackage/mt76x2e_ap/description
 Kernel modules for Mediatek MT76x2E AP driver support
endef

define Build/Prepare
	mkdir -p $(PKG_BUILD_DIR)
endef

define Build/Configure

endef

define Build/Compile

endef

define KernelPackage/mt76x2e_ap/install
    $(INSTALL_DIR) $(1)/lib/wifi/ ; \
    $(INSTALL_DIR) $(1)/etc/wireless/mt7602e/ ; \
    $(INSTALL_DIR) $(1)/usr/share/ ; \
    $(INSTALL_BIN) ./src/mt7602e.sh $(1)/lib/wifi/ ; \
    $(INSTALL_DATA) ./src/mt7602e.dat $(1)/etc/wireless/mt7602e/ ; \
    $(INSTALL_DATA) ./src/mt7602e*.bin $(1)/etc/wireless/mt7602e/ ; \
    $(INSTALL_DATA) ./src/SingleSKU_CE.dat $(1)/etc/wireless/mt7602e/SingleSKU.dat ; \
    $(INSTALL_DATA) ./src/RT2860STA.dat $(1)/etc/wireless/mt7602e/RT2860STA.dat ; \
    $(INSTALL_DIR) $(1)/etc/wireless/mt7612e/ ; \
    $(INSTALL_BIN) ./src/mt7612e.sh $(1)/lib/wifi/ ; \
    $(INSTALL_DATA) ./src/mt7612e.dat $(1)/etc/wireless/mt7612e/ ; \
    $(INSTALL_DATA) ./src/mt7612e*.bin $(1)/etc/wireless/mt7612e/ ; \
    $(INSTALL_DATA) ./src/SingleSKU_5G_CE.dat $(1)/etc/wireless/mt7612e/SingleSKU.dat ; \
    $(INSTALL_DATA) ./src/RT2860STA.dat $(1)/etc/wireless/mt7612e/RT2860STA.dat
endef

$(eval $(call KernelPackage,mt76x2e_ap))
