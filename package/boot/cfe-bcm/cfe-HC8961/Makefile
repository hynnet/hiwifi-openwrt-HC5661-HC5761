#
# Copyright (C) 2013 hiwifi.tw
#

include $(TOPDIR)/rules.mk
include $(INCLUDE_DIR)/kernel.mk

PKG_NAME:=cfe-bcm
PKG_VERSION:=1.0.37
PKG_RELEASE:=1

PKG_BUILD_DIR:=$(KERNEL_BUILD_DIR)/$(PKG_NAME)-$(BUILD_VARIANT)/$(PKG_NAME)-$(PKG_VERSION)
PKG_TARGETS:=bin

include $(INCLUDE_DIR)/package.mk

CFE_TARGET_NAME:=HC8961

# override CFLAGS, cfe Makefile will create CFLAGS to build cfe
# if use the default CFLAGS(include TARGET_CFLAGS), it will comile failed
CFLAGS:= -Wno-error=unused-but-set-variable -Wno-error=unused-but-set-parameter

define cfe/Default
  TITLE:=
  CONFIG:=
  IMAGE:=
endef

define cfe/$(CFE_TARGET_NAME)
  TITLE:=cfe for HC8961 based on bcm47081 + bcm4352 + bcm43217 board
endef

CFES:=$(CFE_TARGET_NAME)

define Package/cfe/template
define Package/cfe-bcm470x-$(1)
  SECTION:=boot
  CATEGORY:=Boot Loaders
  TITLE:=$(2)
  DEPENDS:=@TARGET_bcm470x_generic
  DEFAULT:=y if (TARGET_bcm470x_generic_HC8961)
  VARIANT:=$(1)
endef
endef

define BuildCfePackage
	$(eval $(cfe/Default))
	$(eval $(cfe/$(1)))
	$(call Package/cfe/template,$(1),$(TITLE))
endef

CFE_BINARY:=cfez.bin

ifdef BUILD_VARIANT
$(eval $(call cfe/$(BUILD_VARIANT)))
CFE_IMAGE:=$(if $(IMAGE),$(IMAGE),openwrt-$(BOARD)-$(BUILD_VARIANT)-$(CFE_BINARY))
endif

define Build/Prepare
	mkdir -p $(PKG_BUILD_DIR)
	$(CP) ../cfe_common/* $(PKG_BUILD_DIR)
	$(CP) nvram $(PKG_BUILD_DIR)
	chmod -R +x $(PKG_BUILD_DIR)/tools
	find $(PKG_BUILD_DIR) -name .svn | $(XARGS) rm -rf
endef

define Build/Configure

endef

define Build/Compile
	$(MAKE) -C $(PKG_BUILD_DIR)/src/cfe \
		TOOLPREFIX=$(TARGET_CROSS)
endef

define Package/cfe/install/template
define Package/cfe-bcm470x-$(1)/install
	$(INSTALL_DIR) $$(1)
	$(PKG_BUILD_DIR)/tools/nvserial -i $(PKG_BUILD_DIR)/$(CFE_BINARY) -o $(BIN_DIR)/$(2) -s SN $(PKG_BUILD_DIR)/nvram/HC8961.txt
endef
endef

$(foreach u,$(CFES), \
	$(eval $(call Package/cfe/install/template,$(u),openwrt-$(BOARD)-$(SUBTARGET)-$(u)-$(CFE_BINARY))) \
)

$(foreach u,$(CFES), \
	$(eval $(call BuildCfePackage,$(u))) \
	$(eval $(call BuildPackage,cfe-bcm470x-$(u))) \
)
