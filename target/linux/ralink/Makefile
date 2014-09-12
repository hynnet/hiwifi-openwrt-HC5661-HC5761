#
# Copyright (C) 2012-2013 hiwifi.com
#
# This is free software. licensed under the GNU General Public License v2.
# See /LICENSE for more information
#
include $(TOPDIR)/rules.mk

ARCH:=mipsel
BOARD:=ralink
BOARDNAME:=Ralink Soc
SUBTARGETS:=mt7620 mt7620a-custom
CFLAGS:=-Os -pipe -mips32r2 -mtune=mips32r2 -fno-caller-saves
FEATURES:=squashfs

LINUX_VERSION:=3.3.8

include $(INCLUDE_DIR)/target.mk

DEFAULT_PACKAGES += wireless-tools \
		    kmod-leds-gpio kmod-ledtrig-gpio kmod-ledtrig-timer kmod-ledtrig-netdev \
		    kmode-ledtrig-default-on kmod-gpio-button-hotplug util-switch

define Target/Description
	Build firmware images for Ralink Soc(MT7620N/A) Boards
endef

$(eval $(call BuildTarget))
