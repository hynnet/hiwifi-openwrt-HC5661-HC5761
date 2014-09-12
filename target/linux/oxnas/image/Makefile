# 
# Copyright (C) 2005-2013 lintel<lintel.huang@gmail.com>
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#
include $(TOPDIR)/rules.mk
include $(INCLUDE_DIR)/image.mk

JFFS2_BLOCKSIZE=128k
JFFS2OPTS += --little-endian --pagesize=0x800 --no-cleanmarkers --pad

UBIFS_OPTS += -m 2048 -e 126KiB -c 8192
UBINIZE_OPTS += -m 2048 -p 128KiB -O 512


define Image/BuildKernel
	$(TARGET_CROSS)objcopy -O binary -R .note -R .comment -S $(LINUX_DIR)/arch/arm/boot/compressed/vmlinux $(LINUX_DIR)/linux.bin
	mkimage -A arm -O linux -T kernel -C none -a 60008000 -e 60008000 -n "PandoraBox-OX820-$(LINUX_VERSION)" -d $(LINUX_DIR)/linux.bin $(LINUX_DIR)/uImage

	cp $(LINUX_DIR)/uImage $(BIN_DIR)/$(IMG_PREFIX)-$(KERNEL)-uImage
endef

define Image/Build/squashfs
    $(call prepare_generic_squashfs,$(BIN_DIR)/$(IMG_PREFIX)-root.$(1))
endef

define Image/Build
	$(CP) $(KDIR)/root.$(1) $(BIN_DIR)/$(IMG_PREFIX)-root.$(1)
	$(call Image/Build/$(1),$(1))
endef


$(eval $(call BuildImage))
