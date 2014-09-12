#
# Copyright (C) 2012-2013 hiwifi.com
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#
include $(TOPDIR)/rules.mk

ARCH:=arm
BOARD:=bcm470x
BOARDNAME:=Broadcom BCM4707/BCM47081/BCM4708
FEATURES:=squashfs
SUBTARGETS:=generic smp

LINUX_VERSION:=3.3.8

CFLAGS:=-Os -pipe -march=armv7-a -mcpu=cortex-a9 -mtune=cortex-a9 -msoft-float -mfloat-abi=soft -Wno-error=unused-but-set-variable -Wno-error=unused-but-set-parameter

include $(INCLUDE_DIR)/target.mk

DEAFULT_PACKAGES += kmod-diag nvram kmod-leds-gpio kmod-gpio-button-hotplug \
					kmod-ledtrig-default-on kmod-ledtrig-timer kmod-ledtrig-netdev util-switch

define Target/Description
	Build firmware images for Broadcom BCM4707/47081/4708 based boards
endef

$(eval $(call BuildTarget))
