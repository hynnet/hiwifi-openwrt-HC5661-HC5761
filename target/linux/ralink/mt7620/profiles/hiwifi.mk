#
# Copyright (C) 2013-2014 www.hiwifi.com
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

define Profile/HC5661
	NAME:=Hiwifi Wireless HC5661 Board
	PACKAGES:=\
		kmod-usb-core kmod-usb2 \
		kmod-ledtrig-usbdev
endef

define Profile/HC5661/Description
	Default package set compatible with Hiwifi Wireless HC5661 Board.
endef
$(eval $(call Profile,HC5661))

define Profile/HC5761
	NAME:=Hiwifi Wireless HC5761 Board
	PACKAGES:=\
		kmod-usb-core kmod-usb2 \
		kmod-ledtrig-usbdev
endef

define Profile/HC5761/Description
	Default package set compatible with Hiwifi Wireless HC5761 Board.
endef
$(eval $(call Profile,HC5761))

# MT7620A + MT7610E + PoE
define Profile/HB5881
	NAME:=HiWiFi Wireless HB5881 Board
	PACKAGES:=\
		kmod-usb-core kmod-usb2
endef

define Profile/HB5881/Description
	Default package set compatible with HiWiFi Wireless HB5881(MT7620A + MT7610E + PoE) Board.
endef
$(eval $(call Profile,HB5881))

# MT7620A + MT7612E
define Profile/HC5861
	NAME:=HiWiFi Wireless HC5861 Board
	PACKAGES:=\
		kmod-usb-core kmod-usb2
endef

define Profile/HC5861/Description
	Default package set compatible with HiWiFi Wireless HC5861(MT7620A + MT7612E) Board.
endef
$(eval $(call Profile,HC5861))

define Profile/HC5663
	NAME:=Hiwifi Wireless HC5663 Board
	PACKAGES:=\
		kmod-usb-core kmod-usb2 \
		kmod-ledtrig-usbdev kmod-ata-core kmod-ata-ahci
endef

define Profile/HC5663/Description
	Default package set compatible with Hiwifi Wireless HC5663 Board.
endef
$(eval $(call Profile,HC5663))

define Profile/HB5981m
	NAME:=Hiwifi Wireless HB5981 master Board
	PACKAGES:=\
		kmod-usb-core kmod-usb2 \
		kmod-ledtrig-usbdev kmod-ata-core kmod-ata-ahci \
		icpuc
endef

define Profile/HB5981m/Description
	Default package set compatible with Hiwifi Wireless HB5981 master Board.
endef
$(eval $(call Profile,HB5981m))

define Profile/HB5981s
	NAME:=Hiwifi Wireless HB5981 slave Board
	PACKAGES:=\
		kmod-usb-core kmod-usb2 \
		kmod-ledtrig-usbdev icpuc
endef

define Profile/HB5981s/Description
	Default package set compatible with Hiwifi Wireless HB5981 Slave Board.
endef
$(eval $(call Profile,HB5981s))

define Profile/HC5641
	NAME:=HiWiFi Wireless HC5641 Board
	PACKAGES:=\
		kmod-usb-core kmod-usb2 \
		kmod-ledtrig-usbdev
endef

define Profile/HC5641/Description
	Default package set compatible with HiWiFi Wireless HC5641 Board.
endef
$(eval $(call Profile,HC5641))

define Profile/HB5811
	NAME:=HiWiFi Wireless HB5811 AP Board
endef

define Profile/HB5811/Description
	Default package set compatible with HiWiFi Wireless HB5811 AP Board(with HB5881).
endef
$(eval $(call Profile,HB5811))