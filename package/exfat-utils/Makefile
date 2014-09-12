#
# Copyright (C) 2012-2013 hiwifi.com
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk

PKG_NAME:=exfat-utils
PKG_RELEASE:=1

PKG_VERSION:=1.0.1

PKG_SOURCE:=$(PKG_NAME)-$(PKG_VERSION).tar.gz
PKG_SOURCE_URL:=http://exfat.googlecode.com/files

include $(INCLUDE_DIR)/package.mk

define Package/mkexfatfs
  SECTION:=utils
  CATEGORY:=Utilities
  URL:=http://code.google.com/p/exfat/
  SUBMENU:=Filesystem
  TITLE:=create exfat fs
endef

define Package/mkexfatfs/description
  Utilities to create exfat filesystem.
endef

define Package/fsckexfat
  SECTION:=utils
  CATEGORY:=Utilities
  URL:=http://code.google.com/p/exfat/
  SUBMENU:=Filesystem
  TITLE:=check exfat fs
endef

define Package/fsckexfat/description
  Utilities to check exfat filesystem.
endef

define Build/Prepare
	$(call Build/Prepare/Default)
	$(CP) ./src/Makefile $(PKG_BUILD_DIR)/
endef

TARGET_CFLAGS += -O2 -D_FILE_OFFSET_BITS=64 -std=c99 -D_GNU_SOURCE -I./libexfat

define Build/Compile
	$(MAKE) -C $(PKG_BUILD_DIR) \
		CC="$(TARGET_CC)" \
		RANLIB="$(TARGET_RANLIB)" \
		CFLAGS="$(TARGET_CFLAGS)" \
		LDFLAGS="$(TARGET_LDFLAGS)"
endef

define Package/mkexfatfs/install
	$(INSTALL_DIR) $(1)/sbin
	$(CP) $(PKG_BUILD_DIR)/mkfs.exfat $(1)/sbin/
endef

define Package/fsckexfat/install
	$(INSTALL_DIR) $(1)/sbin
	$(CP) $(PKG_BUILD_DIR)/fsck.exfat $(1)/sbin/
endef

$(eval $(call BuildPackage,mkexfatfs))
$(eval $(call BuildPackage,fsckexfat))
