#
# Copyright (C) 2013 lintel
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#
include $(TOPDIR)/rules.mk
include $(INCLUDE_DIR)/host.mk

ARCH:=arm
BOARD:=oxnas
BOARDNAME:=Oxford OXE8x0
FEATURES:=ext4 squashfs ubifs
MAINTAINER:=lintel<lintel.huang@gmail.com>

LINUX_VERSION:=3.1.10

include $(INCLUDE_DIR)/target.mk
DEFAULT_PACKAGES += hostapd kmod-usb2 kmod-usb-ohci kmod-usb-storage kmod-fs-ext4 kmod-fs-vfat kmod-nls-cp437 kmod-nls-utf8  kmod-nls-iso8859-1 \
uboot-envtools mtd-utils mtd-utils-flash-erase mtd-utils-flash-eraseall mtd-utils-nanddump mtd-utils-nandwrite  mtd-utils-ubiattach  \
mtd-utils-ubidetach mtd-utils-ubiformat mtd-utils-ubimkvol mtd-utils-ubimkvol

define Target/Description
	Build firmware image for NAS78xx.
endef

#CFLAGS:=-pipe -march=armv6 -mtune=arm1136jf-s -mfpu=vfp -mfloat-abi=softfp -marm
CFLAGS:=-Os -pipe -march=armv6 -mtune=arm1136j-s -fno-caller-saves

$(eval $(call BuildTarget))
