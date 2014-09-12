# 
# Copyright (C) 2012-2013 hiwifi.com
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk
include $(INCLUDE_DIR)/kernel.mk

PKG_NAME:=kmod-exfat
PKG_RELEASE:=5

include $(INCLUDE_DIR)/package.mk

define KernelPackage/exfat
  SUBMENU:=Filesystems
  TITLE:=exFat Filesystem drivers
  DEPENDS:=+kmod-nls-utf8
  FILES:= \
	$(PKG_BUILD_DIR)/exfat_core.ko \
	$(PKG_BUILD_DIR)/exfat_fs.ko
  AUTOLOAD:=$(call AutoLoad,35,exfat_core exfat_fs)
endef

define KernelPackage/exfat/description
 This package contains exfat filesystem drivers.
endef

define Build/Prepare
	mkdir -p $(PKG_BUILD_DIR)
	$(CP) ./src/* $(PKG_BUILD_DIR)/
endef

define Build/Compile
	$(MAKE) -C "$(LINUX_DIR)" \
		CROSS_COMPILE="$(TARGET_CROSS)" \
		ARCH="$(LINUX_KARCH)" \
		SUBDIRS="$(PKG_BUILD_DIR)" \
		EXTRA_CFLAGS="$(BUILDFLAGS)" \
		modules
endef

$(eval $(call KernelPackage,exfat))
