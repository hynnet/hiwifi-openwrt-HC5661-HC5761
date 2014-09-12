#
# Copyright (C) 2012-2012 Turbo Wireless
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#


define Profile/TW300V1
	NAME:=Turbo Wireless AR9341 Board
	PACKAGES:=kmod-hcwifi \
				kmod-usb-core kmod-usb2 kmod-usb-storage kmod-usb-storage-extras \
				ppp-mod-pppol2tp ppp-mod-pptp \
				kmod-crypto-deflate kmod-fs-ext4 kmod-fs-msdos kmod-fs-ntfs kmod-fs-vfat \
				kmod-ledtrig-gpio kmod-ledtrig-usbdev \
				kmod-nls-cp437 kmod-nls-iso8859-1 kmod-nls-utf8 \
				e2fsprogs
				
endef

define Profile/TW300V1/description
	Package set optimized for the Turbo Wireless AR9341 Board.
endef
$(eval $(call Profile,TW300V1))


define Profile/TW81V1
	NAME:=Turbo Wireless AR9331 Board
	PACKAGES:=kmod-hcwifi \
				kmod-usb-core kmod-usb2 kmod-usb-storage kmod-usb-storage-extras \
				ppp-mod-pppol2tp ppp-mod-pptp \
				kmod-crypto-deflate kmod-fs-ext4 kmod-fs-msdos kmod-fs-ntfs kmod-fs-vfat \
				kmod-ledtrig-gpio kmod-ledtrig-usbdev \
				kmod-nls-cp437 kmod-nls-iso8859-1 kmod-nls-utf8 \
				e2fsprogs
endef

define Profile/TW81V1/description
	Package set optimized for the Turbo Wireless AR9331 Board.
endef
$(eval $(call Profile,TW81V1))


define Profile/TW150V1
	NAME:=Turbo Wireless TW150V1 Board (AR9331)
	PACKAGES:=kmod-hcwifi \
				kmod-usb-core kmod-usb2 kmod-usb-storage kmod-usb-storage-extras \
				ppp-mod-pppol2tp ppp-mod-pptp \
				kmod-crypto-deflate kmod-fs-ext4 kmod-fs-msdos kmod-fs-ntfs kmod-fs-vfat \
				kmod-ledtrig-gpio kmod-ledtrig-usbdev \
				kmod-nls-cp437 kmod-nls-iso8859-1 kmod-nls-utf8 \
				e2fsprogs
endef

define Profile/TW150V1/description
	Package set optimized for the Turbo Wireless AR9331 New Board.
endef
$(eval $(call Profile,TW150V1))

define Profile/HC6341
	NAME:=Turbo Wireless HC6341 Board (AR9331)
	PACKAGES:=kmod-hcwifi \
				kmod-crypto-deflate kmod-ledtrig-gpio
endef

define Profile/HC6341/description
	Package set optimized for the Turbo Wireless AR9331 New Board.
endef
$(eval $(call Profile,HC6341))

define Profile/HC6342
	NAME:=Turbo Wireless HC6342 Board (AR9331 + NXP)
	PACKAGES:=kmod-hcwifi \
				kmod-crypto-deflate kmod-ledtrig-gpio
endef

define Profile/HC6342/description
	Package set optimized for the HiWiFi Wireless AR9331 New Board.
endef
$(eval $(call Profile,HC6342))

define Profile/HC6661
	NAME:=Hiwifi Wireless HC6661 Board (AR9341)
	PACKAGES:= kmod-hcwifi kmod-usb-core kmod-usb2 kmod-usb-storage kmod-usb-storage-extras \
				ppp-mod-pppol2tp ppp-mod-pptp \
				kmod-crypto-deflate kmod-fs-ext4 kmod-fs-msdos kmod-fs-ntfs kmod-fs-vfat \
				kmod-ledtrig-gpio kmod-ledtrig-usbdev \
				kmod-nls-cp437 kmod-nls-iso8859-1 kmod-nls-utf8 \
				e2fsprogs
endef

define Profile/HC6661/description
	Package set optimized for the Turbo Wireless AR9341 New Board.
endef
$(eval $(call Profile,HC6661))

define Profile/HB6881
	NAME:=Hiwifi Wireless HB6881 Board (AR9344)
	PACKAGES:= kmod-hcwifi kmod-usb-core kmod-usb2 kmod-usb-storage kmod-usb-storage-extras \
				ppp-mod-pppol2tp ppp-mod-pptp \
				kmod-crypto-deflate kmod-fs-ext4 kmod-fs-msdos kmod-fs-ntfs kmod-fs-vfat \
				kmod-ledtrig-gpio kmod-ledtrig-usbdev \
				kmod-nls-cp437 kmod-nls-iso8859-1 kmod-nls-utf8 \
				e2fsprogs
endef

define Profile/HB6881/description
	Package set optimized for the HiWiFi Wireless HB6881 Board(AR9344 + ASM1061).
endef
$(eval $(call Profile,HB6881))
