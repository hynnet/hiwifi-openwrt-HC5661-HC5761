#
# Copyright (C) 2013-2014 www.hiwifi.com
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk
include $(INCLUDE_DIR)/image.mk

BIN_DATE_DIR:=$(shell date '+%y%m%d-%H%M%S')-$(REVISION)

define imgname
$(BIN_DIR)/$(IMG_PREFIX)-$(2)-$(1)
endef

define imgstore
$(BIN_DIR)/$(BIN_DATE_DIR)/$(IMG_PREFIX)-$(2)-$(patsubst jffs2-%,jffs2,$(patsubst squashfs-%,squashfs,$(1)))
endef

define sysupname
$(call imgname,$(1),$(2))-sysupgrade.bin
endef

define signname
$(call imgname,$(1),$(2))-sign.bin
endef

define signtmpname
$(call imgname,$(1),$(2))-sign-tmp.bin
endef

define sysupstore
$(call imgstore,$(1),$(2))-sysupgrade.bin
endef

define flashsmtname
$(call imgname,$(1),$(2))-flashsmt.bin
endef

define flashsmtstore
$(call imgstore,$(1),$(2))-flashsmt.bin
endef

define HiwifiSignImg
	$(STAGING_DIR_HOST)/bin/hwf-img-sign -i $(1) -o $(2) -k $(TOPDIR)/hwf-keys/hiwifi.key -c $(TOPDIR)/hwf-keys/hiwifi.crt
endef

define FlashSMT
	$(call HiwifiSignImg,$(call sysupname,$(1),$(2)), $(call signname,$(1),$(2)))
	$(call CatFiles,$(BIN_DIR)/$(IMG_PREFIX)-$(2)-uboot.bin,196608,$(call signname,$(1),$(2)),65536,$(call signtmpname,$(1),$(2)))
	$(call CatFiles,$(call signtmpname,$(1),$(2)),327680,$(call sysupname,$(1),$(2)),20000000,$(call flashsmtname,$(1),$(2)))
endef

define mkcmdline
board=$(1) console=$(2),$(3)
endef

ifeq ($(CONFIG_TARGET_ROOTFS_INITRAMFS),y)
	VMLINUX:=$(IMG_PREFIX)-vmlinux-initramfs
	UIMAGE:=$(IMG_PREFIX)-uImage-initramfs
endif

define CompressLzma
	$(STAGING_DIR_HOST)/bin/lzma e $(1) -lc1 -lp2 -pb2 $(2)
endef

define PatchKernelLzma
	cp $(KDIR)/vmlinux $(KDIR)/vmlinux-$(1)
	$(STAGING_DIR_HOST)/bin/patch-cmdline $(KDIR)/vmlinux-$(1) '$(strip $(2))'
	$(call CompressLzma,$(KDIR)/vmlinux-$(1),$(KDIR)/vmlinux-$(1).bin.lzma)
endef

define MkImage
	$(eval imagename=$(if $(4),$(4),$(4) Linux-$(LINUX_VERSION)))
	mkimage -A mips -O linux -T kernel -C $(1) -a 0x80000000 -e 0x80000000\
		-n "$(imagename)" \
		-d $(2) $(3)
endef

define MkImageLzma
	$(call PatchKernelLzma,$(1),$(2))
	$(call MkImage,lzma,$(KDIR)/vmlinux-$(1).bin.lzma,$(KDIR)/vmlinux-$(1).uImage,$(3))
endef

define MkImageLzma/initramfs
	$(call PatchKernelLzma,$(1)-initramfs,$(2))
	$(call MkImage,lzma,$(KDIR)/vmlinux-$(1)-initramfs.bin.lzma,$(KDIR)/vmlinux-$(1)-initramfs.uImage.bin,$(3))
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

define AppendFiles
	( \
		dd if=$(1) bs=64k conv=sync; \
		dd if=$(2); \
	) > $(4); \
	if [ `stat -c%s $(4)` -gt $(3) ]; then \
		echo "Warning: sysup img is too big"; \
		rm -rf $(4); \
	fi
endef

define Sysupgrade/KRuImage
	$(call AppendFiles,$(KDIR)/vmlinux-$(2).uImage,$(KDIR)/root.$(1),$(3),$(call sysupname,$(1),$(2)))
endef

define Image/BuildKernel
	cp $(KDIR)/vmlinux.elf $(BIN_DIR)/$(VMLINUX).elf
	cp $(KDIR)/vmlinux $(BIN_DIR)/$(VMLINUX).bin
	$(call CompressLzma,$(KDIR)/vmlinux,$(KDIR)/vmlinux.bin.lzma)
	$(call MkImage,lzma,$(KDIR)/vmlinux.bin.lzma,$(KDIR)/uImage.lzma)
	cp $(KDIR)/uImage.lzma $(BIN_DIR)/$(UIMAGE).bin
ifeq ($(CONFIG_TARGET_ROOTFS_INITRAMFS),y)	
	$(call Image/Build/Initramfs)
endif	
endef

#$(1)squashfs $(2)HC5761 $(3)board=HC5761 console=ttyS1, 115200 $(4)16318464 $(5)HC5761
define BuildFirmware/hiwifi
	$(call MkImageLzma,$(2),$(3),$(5))
	$(call Sysupgrade/KRuImage,$(1),$(2),$(4))
	$(call FlashSMT,$(1),$(2))
	@mkdir -p $(BIN_DIR)/$(BIN_DATE_DIR)
	@mv $(call sysupname,$(1),$(2)) $(call sysupstore,$(1),$(2))
	@mv $(call flashsmtname,$(1),$(2)) $(call flashsmtstore,$(1),$(2))
	@cp -f $(BIN_DIR)/$(IMG_PREFIX)-$(2)-uboot.bin $(BIN_DIR)/$(BIN_DATE_DIR)/$(IMG_PREFIX)-$(2)-uboot.bin
	(cd $(DEBUG_TARGET_DIR); tar -czf $(BIN_DIR)/$(BIN_DATE_DIR)/debug-root-$(REVISION)-$(BOARD)-$(2).tgz *)
	(mkdir -p $(BIN_DIR)/debug-root)
	(cp -rf $(BIN_DIR)/$(BIN_DATE_DIR)/debug-root-$(REVISION)-$(BOARD)-$(2).tgz $(BIN_DIR)/debug-root/debug-root-$(REVISION)-$(BOARD)-$(2).tgz)
endef

define BuildFirmware/hiwifi/initramfs
	$(call MkImageLzma/initramfs,$(2),$(3),$(5))
	@cp -f $(KDIR)/vmlinux-$(2)-initramfs.uImage.bin $(BIN_DIR)/$(IMG_PREFIX)-$(2)-initramfs.uImage.bin
endef

define BuildFirmware/hiwifi_8M
	$(call BuildFirmware/hiwifi,$(1),$(2),$(call mkcmdline,$(3),$(4),$(5)),7929856,$(6))
endef

define BuildFirmware/hiwifi_16M
	$(call BuildFirmware/hiwifi,$(1),$(2),$(call mkcmdline,$(3),$(4),$(5)),16318464,$(6))
endef

define BuildFirmware/hiwifi_8M/initramfs
	$(call BuildFirmware/hiwifi/initramfs,$(1),$(2),$(call mkcmdline,$(3),$(4),$(5)),7929856,$(6))
endef

define BuildFirmware/hiwifi_16M/initramfs
	$(call BuildFirmware/hiwifi/initramfs,$(1),$(2),$(call mkcmdline,$(3),$(4),$(5)),16318464,$(6))
endef

define Image/Build/squashfs
	# Align and append a JFFS marker
	$(call prepare_generic_squashfs,$(KDIR)/root.$(1))
endef

define Image/Build/Profile/HC5661
	$(call BuildFirmware/hiwifi_16M,$(1),HC5661,HC5661,ttyS1,115200,HC5661)
endef

define Image/Build/Profile/HC5661/initramfs
	$(call BuildFirmware/hiwifi_16M/initramfs,$(1),HC5661,HC5661,ttyS1,115200,HC5661)
endef

define Image/Build/Profile/HB5811
	$(call BuildFirmware/hiwifi_8M,$(1),HB5811,HB5811,ttyS1,115200,HB5811)
endef

define Image/Build/Profile/HB5811/initramfs
	$(call BuildFirmware/hiwifi_8M/initramfs,$(1),HB5811,HB5811,ttyS1,115200,HB5811)
endef

define Image/Build/Profile/HC5761
	$(call BuildFirmware/hiwifi_16M,$(1),HC5761,HC5761,ttyS1,115200,HC5761)
endef

define Image/Build/Profile/HC5761/initramfs
	$(call BuildFirmware/hiwifi_16M/initramfs,$(1),HC5761,HC5761,ttyS1,115200,HC5761)
endef

define Image/Build/Profile/HB750ACH
	$(call BuildFirmware/hiwifi_16M,$(1),BL-H750AC,BL-H750AC,ttyS1,115200,BL-H750AC)
endef

define Image/Build/Profile/HB750ACH/initramfs
	$(call BuildFirmware/hiwifi_16M/initramfs,$(1),BL-H750AC,BL-H750AC,ttyS1,115200,BL-H750AC)
endef

define Image/Build/Profile/HB845H
	$(call BuildFirmware/hiwifi_16M,$(1),BL-855R,BL-855R,ttyS1,115200,BL-855R)
endef

define Image/Build/Profile/HB845H/initramfs
	$(call BuildFirmware/hiwifi_16M/initramfs,$(1),BL-855R,BL-855R,ttyS1,115200,BL-855R)
endef

define Image/Build/Profile/HC5861
	$(call BuildFirmware/hiwifi_16M,$(1),HC5861,HC5861,ttyS1,115200,HC5861)
endef

define Image/Build/Profile/HC5861/initramfs
	$(call BuildFirmware/hiwifi_16M/initramfs,$(1),HC5861,HC5861,ttyS1,115200,HC5861)
endef

define Image/Build/Profile/HB5881
	$(call BuildFirmware/hiwifi_16M,$(1),HB5881,HB5881,ttyS1,115200,HB5881)
endef

define Image/Build/Profile/HB5881/initramfs
	$(call BuildFirmware/hiwifi_16M/initramfs,$(1),HB5881,HB5881,ttyS1,115200,HB5881)
endef

define Image/Build/Profile/HB5981m
	$(call BuildFirmware/hiwifi_16M,$(1),HB5981m,HB5981m,ttyS1,115200,HB5981m)
endef

define Image/Build/Profile/HB5981m/initramfs
	$(call BuildFirmware/hiwifi_16M/initramfs,$(1),HB5981m,HB5981m,ttyS1,115200,HB5981m)
endef

define Image/Build/Profile/HB5981s
	$(call BuildFirmware/hiwifi_16M,$(1),HB5981s,HB5981s,ttyS1,115200,HB5981s)
endef

define Image/Build/Profile/HB5981s/initramfs
	$(call BuildFirmware/hiwifi_16M/initramfs,$(1),HB5981s,HB5981s,ttyS1,115200,HB5981s)
endef

define Image/Build/Profile/HC5641
	$(call BuildFirmware/hiwifi_8M,$(1),HC5641,HC5641,ttyS0,115200,HC5641)
endef

define Image/Build/Profile/HC5641/initramfs
	$(call BuildFirmware/hiwifi_8M/initramfs,$(1),HC5641,HC5641,ttyS1,115200,HC5641)
endef

define Image/Build/Profile/HC5663
	$(call BuildFirmware/hiwifi_16M,$(1),HC5663,HC5663,ttyS1,115200,HC5663)
endef

define Image/Build/Profile/HC5663/initramfs
	$(call BuildFirmware/hiwifi_16M/initramfs,$(1),HC5663,HC5663,ttyS1,115200,HC5663)
endef

define Image/Build/Initramfs
	$(call Image/Build/Profile/$(PROFILE)/initramfs,$(1))
endef

define Image/Build
	$(call Image/Build/$(1),$(1))
	dd if=$(KDIR)/root.$(1) of=$(BIN_DIR)/$(IMG_PREFIX)-root.$(1) bs=64k conv=sync
	$(call Image/Build/Profile/$(PROFILE),$(1))
endef

$(eval $(call BuildImage))
