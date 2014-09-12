#!/bin/sh
#
# Copyright (C) 2012-2013 hiwifi.com
# All rights reserved.
# 

BCM470X_BOARD_NAME=

bcm470x_board_detect() {
	local machine
	local name

	machine=$(awk 'BEGIN{FS="[ \t]+:[ \t]"} /machine/ {print $2}' /proc/cpuinfo)

	case "$machine" in
	*"HC8961")
		name="HC8961"
		;;	
	esac

	[ -z "$name" ] && name="unknown"

	[ -z "$BCM470X_BOARD_NAME" ] && BCM470X_BOARD_NAME="$name"

	[ -e "/tmp/sysinfo/" ] || mkdir -p "/tmp/sysinfo/"

	echo "$BCM470X_BOARD_NAME" > /tmp/sysinfo/board_name
}

bcm470x_board_name() {
	local name

	[ -f /tmp/sysinfo/board_name ] && name=$(cat /tmp/sysinfo/board_name)
	[ -z "$name" ] && name="unknown"

	echo "$name"
}

tw_board_name() {
	local name

	[ -f /tmp/sysinfo/board_name ] && name=$(cat /tmp/sysinfo/board_name)
	[ -z "$name" ] && name="unknown"

	echo "$name"
}

tw_get_mac() {
	ifconfig eth0 | grep HWaddr | awk '{ print $5 }' | awk -F: '{print $1$2$3$4$5$6}'
}
