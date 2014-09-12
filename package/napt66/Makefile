#
# Copyright (C) 2012 tsl0922@sina.com
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk
include $(INCLUDE_DIR)/kernel.mk

PKG_NAME:=napt66
PKG_VERSION:=0.2
PKG_RELEASE:=20130903.1

PKG_SOURCE:=$(PKG_NAME)-$(PKG_VERSION).tar.gz
PKG_SOURCE_URL:=http://napt66.googlecode.com/files/
PKG_MD5SUM:=b2b717b3cde4c214f7172b8bdae2b49d

include $(INCLUDE_DIR)/package.mk

define KernelPackage/napt66
	TITLE:=kmod-$(PKG_NAME)
  	URL:=http://code.google.com/p/napt66/
	SUBMENU:=Netfilter Extensions
	DEPENDS:=+ip6tables +radvd
	FILES:=$(PKG_BUILD_DIR)/napt66.$(LINUX_KMOD_SUFFIX)
endef

define KernelPackage/napt66/description
 Stateful IPv6-to-IPv6 Network Address Port Translation.
endef

EXTRA_KCONFIG:= \
	CONFIG_NAPT66=m

EXTRA_CFLAGS:= \
	$(patsubst CONFIG_%, -DCONFIG_%=1, $(patsubst %=m,%,$(filter %=m,$(EXTRA_KCONFIG)))) \
	$(patsubst CONFIG_%, -DCONFIG_%=1, $(patsubst %=y,%,$(filter %=y,$(EXTRA_KCONFIG)))) \

MAKE_OPTS:= \
	ARCH="$(LINUX_KARCH)" \
	CROSS_COMPILE="$(TARGET_CROSS)" \
	SUBDIRS="$(PKG_BUILD_DIR)" \
	EXTRA_CFLAGS="$(EXTRA_CFLAGS)" \
	$(EXTRA_KCONFIG)

define Build/Compile
	$(MAKE) -C "$(LINUX_DIR)" \
		$(MAKE_OPTS) \
		modules
endef

define KernelPackage/napt66/install
	$(INSTALL_DIR) $(1)/etc/init.d/
	$(INSTALL_BIN) ./files/napt66.init $(1)/etc/init.d/napt66
endef

$(eval $(call KernelPackage,napt66))
