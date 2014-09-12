#
# Copyright (C) 2013-2014 www.hiwifi.com
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

# B-LINK MT7620A + MT7610E
define Profile/HB750ACH
	NAME:=HiWiFi Wireless HB750ACH Customer Board
	PACKAGES:=\
		kmod-usb-core kmod-usb2
endef

define Profile/HB750ACH/Description
	Default package set compatible with HiWiFi Wireless HB750ACH Customer Board.
endef
$(eval $(call Profile,HB750ACH))

# B-LINK MT7620N
define Profile/HB845H
	NAME:=Hiwifi Wireless HB845H Customer Board
endef

define Profile/HB845H/Description
	Default package set compatible with Hiwifi Wireless HB845H Customer Board.
endef
$(eval $(call Profile,HB845H))