#
# Copyright (C) 2012-2013 hiwifi.com
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

define Profile/HC8961
	NAME:=Hiwifi Wireless HC8961 Board
	PACKAGES:=e2fsprogs			
endef

define Profile/HC8961/description
	Package set optimized for the Hiwifi Wireless HC8961 based on bcm47081 Board
endef
$(eval $(call Profile,HC8961))

