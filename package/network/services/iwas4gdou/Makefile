# 
# Copyright (C) 2006-2008 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk

PKG_NAME:=iwas4gdou
PKG_VERSION:=1.0
PKG_RELEASE:=1.1
PKG_SOURCE_URL:=https://github.com/downloads/liuqun/iwas4gdou/
PKG_SOURCE:=$(PKG_NAME)-$(PKG_VERSION).tar.gz
PKG_MD5SUM:=73fbc09b4b3664d9356ee74b5b274b99

include $(INCLUDE_DIR)/package.mk

define Package/iwas4gdou
  SECTION:=net
  CATEGORY:=Network
  TITLE:=I was for GDOU 802.1X client program
  URL:=http://github.com/liuqun/iwas4gdou
  DEPENDS:=+libpcap
  SUBMENU:=CERNET
endef

define Package/iwas4gdou/description
 802.1X client from Guangdong Ocean University
endef

define Package/iwas4gdou/install
	$(INSTALL_DIR) $(1)/usr/sbin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/iwas4gdou  $(1)/usr/sbin/iwas4gdou

	$(INSTALL_DIR) $(1)/etc/init.d
	$(INSTALL_BIN) ./files/iwas4gdou.init $(1)/etc/init.d/iwas4gdou

	$(INSTALL_DIR) $(1)/etc/config
	$(INSTALL_CONF) ./files/iwas4gdou.cfg $(1)/etc/config/iwas4gdou

	$(INSTALL_DIR) $(1)/etc/hotplug.d/iface
	$(INSTALL_DATA) ./files/iwas4gdou.hotplug $(1)/etc/hotplug.d/iface/70-iwas4gdou
endef

$(eval $(call BuildPackage,iwas4gdou))
