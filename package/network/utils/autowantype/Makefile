# 
# Copyright (C) 2012 hiwifi.com
# Copyright (C) eric<jin.zhong@hiwifi.tw>
#
#

include $(TOPDIR)/rules.mk

PKG_NAME:=autowantype
PKG_VERSION:=1
PKG_RELEASE:=20140306

PKG_BUILD_DIR := $(BUILD_DIR)/$(PKG_NAME)
include $(INCLUDE_DIR)/package.mk

define Package/autowantype
  SECTION:=net
  CATEGORY:=Network
  TITLE:=HiWiFi WAN type check utility
  URL:=http://www.hiwifi.com
endef

define Package/autowantype/description
 autowantype: 1  means PPPOE
 autowantype: 2  means DHCP
 autowantype: 3  means Manual
 autowantype: 99 means Cable problem
endef


define Build/Prepare
	mkdir -p $(PKG_BUILD_DIR)
	$(CP) ./src/* $(PKG_BUILD_DIR)/
endef


define Package/autowantype/install
	$(INSTALL_DIR) $(1)/usr/bin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/autowantype  $(1)/usr/bin/autowantype
endef

$(eval $(call BuildPackage,autowantype))
