#!/bin/sh

PART_NAME=firmware

find_mtd_part() {
	local PART="$(grep "\"$1\"" /proc/mtd | awk -F: '{print $1}')"
	local PREFIX=/dev/mtdblock

	PART="${PART##mtd}"
	[ -d /dev/mtdblock ] && PREFIX=/dev/mtdblock/
	echo "${PART:+$PREFIX$PART}"
}

create_link() {
	rm -rf /tmp/data
	ln -s $1 /tmp/data
}

check_storage() {
	dev_path="/dev/mmcblk0p2"
	dev_mount="/tmp/mmcblk0p2"
	mkdir -p $dev_mount
	mount -t ext4 -o noatime "$dev_path" "$dev_mount"
	if [ "$?" -eq "0" ]; then
		create_link "$dev_mount"
		return 0
	fi

	return 1
}

mount_storage() {
	COUNTER=0
	while [ $COUNTER -lt 10 ]; do
		sleep 1
		check_storage
		if [ "$?" -eq "0" ]; then
			break
		else
			COUNTER=`expr $COUNTER + 1`
		fi
	done
}

hiwifi_board_name() {
	local name

	[ -f /tmp/sysinfo/board_name ] && name=$(cat /tmp/sysinfo/board_name)
	[ -z "$name" ] && name="unknown"

	echo "$name"
}

hiwifi_board_detect() {
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
	esac

	[ -z "$name" ] && name="unkown"

	[ -e "/tmp/sysinfo/" ] || mkdir -p "/tmp/sysinfo/"

	echo "$name" > /tmp/sysinfo/board_name
	echo "$machine" > /tmp/sysinfo/model
}

get_image() { # <source> [ <command> ]
	local from="$1"
	local conc="$2"
	local cmd

	case "$from" in
		*) cmd="cat";;
	esac
	if [ -z "$conc" ]; then
		local magic="$(eval $cmd $from 2>/dev/null | dd bs=2 count=1 2>/dev/null | hexdump -n 2 -e '1/1 "%02x"')"
		case "$magic" in
			1f8b) conc="zcat";;
			425a) conc="bzcat";;
		esac
	fi

	eval "$cmd $from 2>/dev/null ${conc:+| $conc}"
}

get_magic_word() {
	get_image "$@" | dd bs=2 count=1 2>/dev/null | hexdump -v -n 2 -e '1/1 "%02x"'
}

get_magic_long() {
	get_image "$@" | dd bs=4 count=1 2>/dev/null | hexdump -v -n 4 -e '1/1 "%02x"'
}

get_magic_boot() {
	get_image "$@" | dd bs=16 count=1 2>/dev/null | hexdump -v -n 16 -e '1/1 "%02x"'
}

get_board_name_up() {
	get_image "$@" | dd bs=32 skip=1 count=1 2>/dev/null
}

get_board_name_smt() {
	get_image "$@" | dd bs=2k skip=160 count=1 2>/dev/null | dd bs=32 skip=1 count=1 2>/dev/null
}

hiwifi_check_image() {
	local board=$(hiwifi_board_name)
	local magic="$(get_magic_word "$1")"
	local magic_long="$(get_magic_long "$1")"
	local magic_boot="$(get_magic_boot "$1")"
	local board_len="$(echo "$board" | wc -L)"
	local pdtname_up=$(get_board_name_up "$1" | cut -c 1-"$board_len")
	local pdtname_smt=$(get_board_name_smt "$1" | cut -c 1-"$board_len")

	case "$board" in
	HC5661 | HC5761)
		[ "$magic_long" != "27051956" -a "$magic" != "2705" -a "$magic_boot" != "ff00001000000000fd00001000000000" ] && {
			echo "Invalid image type."
			return 1
		}
		[ "$pdtname_up" != "$board" -a "$pdtname_smt" != "$board" ] && {
			echo "Invalid image board."
			return 1
		}
		[ "$magic_boot" == "ff00001000000000fd00001000000000" ] && {
			touch /tmp/img_has_boot
			echo "Image has uboot."
		}
		return 0
		;;
	esac

	echo "Sysupgrade is not yet supported on $board."
	return 1
}

do_conf_backup() {
	mtdpart="$(find_mtd_part hwf_config)"
	[ -z "$mtdpart" ] && return 1
	mtd -q erase hwf_config
	mtd -q write "$1" hwf_config
}

hiwifi_get_conf() {
	mtdpart="$(find_mtd_part hwf_config)"
	[ -z "$mtdpart" ] && return 1

	magic="$(dd if=$mtdpart bs=1 skip=0 2>/dev/null | hexdump -n 2 -e '2/1 "%02x"')"
	[ "$magic" != "1f8b" ] && return 1

	rm -rf "$CONF_TAR"
	cat $mtdpart >"$CONF_TAR"

	return 0
}

blink_led_with_num() {
	case $1 in
	1)
		setled timer green system 100 100
		;;
	2)
		setled timer green system 500 1500
		sleep 1
		setled timer green wlan-2p4 500 1500
		;;
	3)
		setled timer green system 500 2500
		sleep 1
		setled timer green internet 500 2500
		sleep 1
		setled timer green wlan-2p4 500 2500
		;;
	4)
		setled timer green system 500 3000
		sleep 1
		setled timer green internet 500 3000
		sleep 1
		setled timer green wlan-5p 500 3000
		sleep 1
		setled timer green wlan-2p4 500 3000
		;;
	esac
}

hiwifi_led_off() {
	setled on green system
	setled off green internet
	setled off green wlan-5p
	setled off green wlan-2p4
}

hiwifi_do_upgrade() {
	local upgrade_ret=1
	hiwifi_board_detect
	hiwifi_check_image "$1"
	if [ "$?" -eq 1 ]; then
		return 1
	fi

	case "$board" in
	HC5761)
		blink_led_with_num 4
		;;
	HC5661)
		blink_led_with_num 3
		;;
	esac

	if [ -f /tmp/img_has_boot ]; then
		hiwifi_get_conf
		if [ -f "$CONF_TAR" ]; then
			get_image "$1" | dd bs=2k skip=160 conv=sync 2>/dev/null | mtd -q -j "$CONF_TAR" write - "${PART_NAME:-image}"
		else
			get_image "$1" | dd bs=2k skip=160 conv=sync 2>/dev/null | mtd -q write - "${PART_NAME:-image}"
		fi
		upgrade_ret=$?
	fi

	hiwifi_led_off

	return $upgrade_ret
}
