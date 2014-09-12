#
# Copyright (C) 2012 Hardcore Communications, Inc.
# All rights reserved.
#

include $(TOPDIR)/rules.mk
include $(INCLUDE_DIR)/kernel.mk

PKG_NAME:=wlconf
PKG_VERSION:=6.30.163.42
PKG_RELEASE:=1

PKG_BUILD_DIR:=$(BUILD_DIR)/$(PKG_NAME)

include $(INCLUDE_DIR)/package.mk

define Package/wlconf
  SECTION:=net
  CATEGORY:=Network
  DEPENDS:=@TARGET_bcm470x
  TITLE:=wl driver setup utility
endef

define Package/wlconf/description
 This package contains an utility for initializing the proprietary Broadcom 
 wl driver.
endef

define Build/Prepare
	mkdir -p $(PKG_BUILD_DIR)
	$(CP) ./src/* $(PKG_BUILD_DIR)/
endef

TARGET_CFLAGS += -D_GNU_SOURCE -s -I. -I$(LINUX_DIR)/brcm/include -Wall

TARGET_CFLAGS += -DNAS_WKSP_BUILD_NAS_AUTH -DNAS_WKSP_BUILD_NAS_SUPPL -DNAS_WKSP_ON_DEMAND -DBCMSUPPL

MAKE_FLAGS += \
	FPIC="$(FPIC)" \
	CFLAGS="$(TARGET_CFLAGS)" \
	LDFLAGS="$(TARGET_LDFLAGS)"

define Package/wlconf/install
	$(INSTALL_DIR) $(1)/usr/lib $(1)/usr/sbin $(1)/etc/init.d
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/libshared.so $(1)/usr/lib/libshared.so
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/libbcmcrypto.so $(1)/usr/lib/libbcmcrypto.so
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/wlconf $(1)/usr/sbin/
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/nvram $(1)/usr/sbin/
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/eapd $(1)/usr/sbin/
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/nas $(1)/usr/sbin/
	$(INSTALL_BIN) ./files/wlconf.init $(1)/etc/init.d/wlconf
endef

$(eval $(call BuildPackage,wlconf))