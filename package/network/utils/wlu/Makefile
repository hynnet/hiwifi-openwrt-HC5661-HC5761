#
# Copyright (C) 2012 Hardcore Communications, Inc.
# All rights reserved.
#

include $(TOPDIR)/rules.mk
include $(INCLUDE_DIR)/kernel.mk

PKG_NAME:=wlu
PKG_VERSION:=6.30.163.42
PKG_RELEASE:=1

PKG_BUILD_DIR:=$(BUILD_DIR)/$(PKG_NAME)

include $(INCLUDE_DIR)/package.mk

define Package/wlu
  SECTION:=net
  CATEGORY:=Network
  DEPENDS:=@TARGET_bcm470x +liblua
  TITLE:=Proprietary Broadcom wl driver config utility
endef

define Package/wlu/description
 This package contains the proprietary utility (wl) for configuring the 
 proprietary Broadcom wl driver.
endef

define Build/Prepare
	mkdir -p $(PKG_BUILD_DIR)
	$(CP) ./src/* $(PKG_BUILD_DIR)/
endef

TARGET_CFLAGS += -DBCMINTERNAL -DBCMDBG -DLINUX -DD11AC_IOTYPES -DPPR_API -DWLCNT
TARGET_CFLAGS += -D_GNU_SOURCE -s -I. -I$(LINUX_DIR)/brcm/include -Wall

MAKE_FLAGS += \
	FPIC="$(FPIC)" \
	CFLAGS="$(TARGET_CFLAGS)" \
	LDFLAGS="$(TARGET_LDFLAGS)"

define Package/wlu/install
	$(INSTALL_DIR) $(1)/usr/sbin $(1)/usr/lib/lua
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/wl $(1)/usr/sbin/
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/hcwifi.so $(1)/usr/lib/lua/
endef

$(eval $(call BuildPackage,wlu))