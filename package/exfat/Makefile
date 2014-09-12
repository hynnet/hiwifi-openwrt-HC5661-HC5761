#
# Copyright (C) 2011 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk

PKG_NAME:=fuse-exfat
PKG_RELEASE:=1

PKG_VERSION:=1.0.1

PKG_SOURCE:=$(PKG_NAME)-$(PKG_VERSION).tar.gz
PKG_SOURCE_URL:=http://exfat.googlecode.com/files

include $(INCLUDE_DIR)/package.mk

define Package/exfat
  SECTION:=utils
  CATEGORY:=Utilities
  URL:=http://code.google.com/p/exfat/
  SUBMENU:=Filesystem
  TITLE:=exFAT Driver
  DEPENDS:=@!PACKAGE_kmod-exfat +kmod-fuse +libfuse
endef

define Package/exfat/description
  Full-featured exFAT file system implementation for GNU/Linux other Unix-like systems as a FUSE module.
endef

define Build/Prepare
	$(call Build/Prepare/Default)
	$(CP) ./src/Makefile $(PKG_BUILD_DIR)/
endef

define Package/exfat/install
	$(INSTALL_DIR) $(1)/sbin
	$(CP) $(PKG_BUILD_DIR)/mount.exfat $(1)/sbin/
endef

define Package/exfat/postinst
#!/bin/sh

if [ -z "$$(grep ^\\exfat: $${IPKG_INSTROOT}/etc/filesystems)" ]; then
	echo "exfat" >> $${IPKG_INSTROOT}/etc/filesystems
fi
endef

$(eval $(call BuildPackage,exfat))
