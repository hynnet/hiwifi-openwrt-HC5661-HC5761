#
# Copyright (C) 2007-2013 OpenWrt.org
# Copyright (C) 2014 NowRush Studio <hackpascal@gmail.com>
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk
include $(INCLUDE_DIR)/version.mk

PKG_NAME:=goagent
PKG_VERSION:=3.1.22
PKG_RELEASE:=1

PKG_BUILD_DIR := $(BUILD_DIR)/$(PKG_NAME)-$(PKG_VERSION)

include $(INCLUDE_DIR)/package.mk

define Package/goagent
  SECTION:=net
  CATEGORY:=Network
  DEPENDS:=+python +pyopenssl +gevent
  TITLE:=GoAgent Proxy
  VERSION:=$(PKG_VERSION)
endef

define Package/goagent/description
 GoAgent is a proxy tool to break the GFW.
endef

define Build/Prepare
endef

define Build/Configure
endef

define Build/Compile
endef

define Package/goagent/install
	$(INSTALL_DIR) $(1)/etc/config
	$(INSTALL_DATA) ./files/etc/config/goagent $(1)/etc/config/
	$(INSTALL_DIR) $(1)/tmp/storage/mmcblk0p2/goagent
	$(INSTALL_DATA) ./files/etc/goagent/CA.crt $(1)/tmp/storage/mmcblk0p2/goagent/
#	$(INSTALL_DATA) ./files/etc/goagent/packages.egg $(1)/tmp/storage/mmcblk0p2/goagent/
	$(INSTALL_DATA) ./files/etc/goagent/proxy.ini $(1)/tmp/storage/mmcblk0p2/goagent/
	$(INSTALL_DATA) ./files/etc/goagent/proxy.pac $(1)/tmp/storage/mmcblk0p2/goagent/
	$(INSTALL_BIN) ./files/etc/goagent/goagent.sh $(1)/tmp/storage/mmcblk0p2/goagent/
	$(INSTALL_BIN) ./files/etc/goagent/proxy.py $(1)/tmp/storage/mmcblk0p2/goagent/
	$(INSTALL_BIN) ./files/etc/goagent/proxylib.py $(1)/tmp/storage/mmcblk0p2/goagent/
	$(INSTALL_BIN) ./files/etc/goagent/dnsproxy.py $(1)/tmp/storage/mmcblk0p2/goagent/
	$(INSTALL_BIN) ./files/etc/goagent/goagent.sh $(1)/tmp/storage/mmcblk0p2/goagent/
	$(INSTALL_DIR) $(1)/etc/init.d
	$(INSTALL_BIN) ./files/etc/init.d/goagent $(1)/etc/init.d/
endef

$(eval $(call BuildPackage,goagent))
