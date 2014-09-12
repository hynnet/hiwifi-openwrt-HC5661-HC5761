
NANDBD_UPGRADE_DIR := $(BASE_DIR)/package/apps/nandbd_upgrade

nandbd_upgrade: $(ROOTFS_DIR)/usr/sbin/nandbd_upgrade

$(ROOTFS_DIR)/usr/sbin/nandbd_upgrade $(NANDBD_UPGRADE_DIR)/nandbd_upgrade
	@install -d $(@D)
	@install -m 755 $< $@

$(NANDBD_UPGRADE_DIR)/nandbd_upgrade:
	$(MAKE) -C $(NANDBD_UPGRADE_DIR) CROSS_COMPILE=$(CROSS_COMPILE) CFLAGS="$(TARGET_CFLAGS)"

nandbd_upgrade-clean nandbd_upgrade-dirclean:
	-$(MAKE) -C $(NANDBD_UPGRADE_DIR) CROSS_COMPILE=$(CROSS_COMPILE) clean
	@rm -rf $(ROOTFS_DIR)/usr/sbin/nandbd_upgrade

# Kernel must support MTD drivers first, or it's useless
ifeq ($(strip $(SDK_ROOTFS_APPS_NANDBD)),y)
ifeq ($(strip $(SDK_BUILD_NAND_BOOT)),y)
SDK_ROOTFS_PACKAGES += nandbd_upgrade
endif
endif

