#
# Copyright (C) 2012-2013 hiwifi.com
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#
include $(TOPDIR)/rules.mk
include $(INCLUDE_DIR)/image.mk

# lzma comrepess image name
VMLINUZ_NAME:=vmlinuz

BIN_DATE_DIR:=$(shell date '+%y%m%d-%H%M%S')-$(REVISION)

# cfe size 256K
BCM_CFE_SIZE:= 262144

KDIR_TMP:=$(KDIR)/tmp

define Image/Prepare
	-mkdir $(KDIR_TMP)
	cp -f tools/lzma_4k $(STAGING_DIR_HOST)/bin
	chmod +x $(STAGING_DIR_HOST)/bin/lzma_4k
	cp $(LINUX_DIR)/arch/arm/boot/Image $(KDIR)/Image
	$(STAGING_DIR_HOST)/bin/lzma_4k e $(KDIR)/Image $(KDIR)/$(VMLINUZ_NAME)
	rm -rf $(KDIR)/fs_mark
	echo -ne '\xde\xad\xc0\xde' > $(KDIR)/fs_mark
	$(call prepare_generic_squashfs, $(KDIR)/fs_mark)
endef

define trxalign/jffs2-128k
-a 0x20000 -f $(KDIR)/root.$(1)
endef

define trxalign/jffs2-64k
-a 0x10000 -f $(KDIR)/root.$(1)
endef

define trxalign/squashfs
-a 0x10000 -f $(KDIR)/root.$(1) $(if $(2), -f $(2)) -a 0x10000 -A $(KDIR)/fs_mark
endef

define Image/Build/Profile
	$(call twseriesStore,$(1),$(2))
endef

define CatFiles
	if [ `stat -c%s "$(1)"` -gt $(2) ]; then \
		echo "Warning: $(1) is too big"; \
	else if [ `stat -c%s $(3)` -gt $(4) ]; then \
		echo "Warning: $(3) is too big"; \
	else \
		( dd if=$(1) bs=$(2) conv=sync; dd if=$(3) ) > $(5); \
	fi; fi
endef

define twseriesStore
	@mkdir -p $(BIN_DIR)/$(BIN_DATE_DIR)
	cp -rf $(BIN_DIR)/$(IMG_PREFIX)-$(2)-cfez.bin $(BIN_DIR)/$(BIN_DATE_DIR)
	cp -rf $(BIN_DIR)/$(IMG_PREFIX)-$(1)-$(2).trx $(BIN_DIR)/$(BIN_DATE_DIR)/$(IMG_PREFIX)-$(1)-$(2)-sysupgrade.bin
	$(call CatFiles,$(BIN_DIR)/$(IMG_PREFIX)-$(2)-cfez.bin,$(BCM_CFE_SIZE),$(BIN_DIR)/$(IMG_PREFIX)-$(1)-$(2).trx,20000000,$(BIN_DIR)/$(BIN_DATE_DIR)/$(IMG_PREFIX)-$(1)-$(2)-flashsmt.bin)
endef

HC8961_CMDLINE:=board=HC8961 console=ttyS0,115200 root=/dev/mtdblock rootfstype=squashfs,jffs2 noinitrd

define Image/Build/HC8961
	cp $(KDIR)/Image $(KDIR_TMP)/Image
	$(STAGING_DIR_HOST)/bin/patch-cmdline $(KDIR_TMP)/Image "$(strip $(HC8961_CMDLINE))"
	$(STAGING_DIR_HOST)/bin/lzma_4k e $(KDIR_TMP)/Image $(KDIR_TMP)/$(VMLINUZ_NAME)
endef

define Image/Build
	echo $(1)
	echo $(PROFILE)
	$(call Image/Build/$(1),$(1))
	$(call Image/Build/$(PROFILE),$(PROFILE))
	$(STAGING_DIR_HOST)/bin/trx -o $(BIN_DIR)/$(IMG_PREFIX)-$(1)-$(PROFILE).trx \
		-f $(KDIR_TMP)/$(VMLINUZ_NAME) $(call trxalign/$(1),$(1))
	$(call Image/Build/Profile,$(1),$(PROFILE))	
endef

$(eval $(call BuildImage))
