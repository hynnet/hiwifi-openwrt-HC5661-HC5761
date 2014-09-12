#!/bin/sh
#
# Copyright (C) 2012-2013 hiwifi.com
#

ralink_uboot_version() {
	dd if=/dev/mtdblock0 ibs=1 skip=196544 2>/dev/null | dd of=/tmp/sysinfo/boot_version bs=20 skip=0 count=1 2>/dev/null
	cat /tmp/sysinfo/boot_version | grep -qs "HIWIFI_BOOT"
	if [ $? -ne 0 ]; then
		echo "HIWIFI_BOOT 00000000" >/tmp/sysinfo/boot_version
	fi
}

ralink_board_detect() {
	local machine
	local name

	machine=$(awk 'BEGIN{FS="[ \t]+:[ \t]"} /machine/ {print $2}' /proc/cpuinfo)

	case "$machine" in
	*"Hiwifi Wireless HC5661 Board")
		name="HC5661"
		;;
	*"Hiwifi Wireless HC5761 Board")
		name="HC5761"
		;;
	*"Hiwifi Wireless HB5981m Board")
		name="HB5981m"
		;;
	*"Hiwifi Wireless HB5981s Board")
		name="HB5981s"
		;;
	*"HiWiFi Wireless HC5641 Board")
		name="HC5641"
		;;
	*"HiWiFi Wireless HC5663 Board")
		name="HC5663"
		;;
	*"HiWiFi Wireless HC5861 Board")
		name="HC5861"
		;;
	*"HiWiFi Wireless HB5881 Board")
		name="HB5881"
		;;
	*"HiWiFi Wireless HB5811 Board")
		name="HB5811"
		;;
	*"HiWiFi-HB-750ACH Board")
		name="BL-H750AC"
		;;
	*"HiWiFi-HB-845H Board")
		name="BL-855R"
		;;
	esac

	[ -z "$name" ] && name="unkown"

	[ -e "/tmp/sysinfo/" ] || mkdir -p "/tmp/sysinfo/"

	ralink_uboot_version

	echo "$name" > /tmp/sysinfo/board_name
	echo "$machine" > /tmp/sysinfo/model
}

get_board_name_up() {
	get_image "$@" | dd bs=32 skip=1 count=1 2>/dev/null
}

get_board_name_smt() {
	get_image "$@" | dd bs=2k skip=160 count=1 2>/dev/null | dd bs=32 skip=1 count=1 2>/dev/null
}

get_boot_version() {
	get_image "$@" | dd ibs=1 skip=196544 2>/dev/null | dd bs=20 skip=0 count=1 2>/dev/null | awk '{print $2}'
}

tw_board_name() {
	local name

	[ -f /tmp/sysinfo/board_name ] && name=$(cat /tmp/sysinfo/board_name)
	[ -z "$name" ] && name="unknown"

	echo "$name"
}

tw_boot_version() {
	cat /tmp/sysinfo/boot_version | awk '{print $2}'
}

tw_get_mac() {
	board=$(tw_board_name)
	case "$board" in
		*)
			ifconfig eth2 | grep HWaddr | awk '{ print $5 }' | awk -F: '{printf $1$2$3$4$5$6}'
		;;
	esac
}

tw_set_bridge_port() {
	local board=$(tw_board_name)
	case "$board" in
	HC5661 | HC5761 | HC5663 | HC5861 | HB5881)
		if [ $1 -eq 0 ]; then
			# port 1-4 in VLAN 1(LAN), port 0 in VLAN 2(WAN)
			switch reg w 2114 10001
			switch vlan set 0 1 01111111
			switch vlan set 1 2 10000011
		else
			# port 0-1 in VLAN 2(WAN), port 2-4 in VLAN 1(LAN)
			switch reg w 2114 10002
			switch vlan set 0 1 00111111
			switch vlan set 1 2 11000011
		fi

		# clear mac table if vlan configuration chaged
		switch clear
		;;
	esac
}

hiwifi_6LoWPAN_fixup() {
	uci set 6LoWPAN.tun0.tty=/dev/ttyS1
	uci commit
	sync
}
