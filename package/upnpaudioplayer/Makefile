#
# Copyright (C) 2013 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk

PKG_NAME:=upnpaudioplayer
PKG_VERSION:=0.1-git
PKG_RELEASE=$(PKG_SOURCE_VERSION)

PKG_SOURCE_PROTO:=git
PKG_SOURCE_URL:=git://github.com/phillimar/UPNPAudioPlayer.git
PKG_SOURCE_SUBDIR:=$(PKG_NAME)-$(PKG_VERSION)
PKG_SOURCE_VERSION:=582b81f5e35a2fc6b03158fe37c5913113aa2cd6
PKG_SOURCE:=$(PKG_NAME)-$(PKG_VERSION)-$(PKG_SOURCE_VERSION).tar.gz

include $(INCLUDE_DIR)/package.mk
include $(INCLUDE_DIR)/nls.mk

define Package/upnpaudioplayer
  SECTION:=multimedia
  CATEGORY:=Multimedia
  DEPENDS:=+gstreamer +libgupnp
  TITLE:=A Headless UPnP Renderer
endef

define Package/upnpaudioplayer/description
  It's an application that plays audio that is controlled via UPnP ("Universal Plug and Play")
endef

define Build/Configure
	(cd $(PKG_BUILD_DIR)/src; make spotless );
	$(call Build/Configure/Default)
endef

define Build/Compile
	$(MAKE) -C $(PKG_BUILD_DIR)/src \
		DESTDIR="$(PKG_INSTALL_DIR)" \
		all install
endef

define Package/upnpaudioplayer/install
	$(INSTALL_DIR) $(1)/usr/bin
	$(INSTALL_BIN) $(PKG_INSTALL_DIR)/usr/bin/$(PKG_NAME) $(1)/usr/bin/
	$(INSTALL_DIR) $(1)/usr/data
	$(CP) $(PKG_BUILD_DIR)/data/*.xml $(1)/usr/data/
	$(INSTALL_DIR) $(1)/etc/config
	$(INSTALL_DATA) files/etc/config/upnpaudioplayer $(1)/etc/config/
	$(INSTALL_DIR) $(1)/etc/init.d
	$(INSTALL_BIN) files/etc/init.d/upnpaudioplayer $(1)/etc/init.d/
endef

$(eval $(call BuildPackage,upnpaudioplayer))
