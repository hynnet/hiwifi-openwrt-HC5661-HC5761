#
# Copyright (C) 2012-2013 hiwifi.com
#

BOOT_NAME=boot
PART_NAME=linux
RAMFS_COPY_DATA=/lib/bcm470x.sh

platform_do_upgrade_hiwifi() {
	sleep 2
	setled timer green system 500 3000
	sleep 1
	setled timer green internet 500 3000
	sleep 1
	setled timer green wlan2g 500 3000
	sleep 1
	setled timer green wlan5g 500 3000

	if [ "$SAVE_CONFIG" -eq 1 -a -z "$USE_REFRESH" ]; then
		if [ -f /tmp/img_has_boot ]; then
			echo "KEEP_CFG has bootloader"
			get_image "$1" | dd bs=2k count=128 conv=sync 2>/dev/null | mtd write - "${BOOT_NAME:-image}"
			get_image "$1" | dd bs=2k skip=128 conv=sync 2>/dev/null | mtd -j "$CONF_TAR" write - "${PART_NAME:-image}"
		else
			echo "KEEP_CFG no bootloader"
			get_image "$1" | mtd -j "$CONF_TAR" write - "${PART_NAME:-image}"
		fi
	else
		if [ -f /tmp/img_has_boot ]; then
			echo "DROP_CFG has bootloader"
			get_image "$1" | dd bs=2k count=128 conv=sync 2>/dev/null | mtd write - "${BOOT_NAME:-image}"
			get_image "$1" | dd bs=2k skip=128 conv=sync 2>/dev/null | mtd write - "${PART_NAME:-image}"
		else
			echo "DROP_CFG no bootloader"
			get_image "$1" | mtd write - "${PART_NAME:-image}"
		fi
	fi
}

platform_check_image() {
	local magic="$(get_magic_word "$1")"
	local magic_boot="$(get_magic_boot "$1")"
	[ "$ARGC" -gt 1 ] && return 1

	[ "$magic" != "4844" -a "$magic_boot" != "ff0400ea14f09fe514f09fe514f09fe5" ] && {
		echo "Ivalid image type."
		return 1;
	}

	[ "$magic_boot" == "ff0400ea14f09fe514f09fe514f09fe5" ] && {
		touch /tmp/img_has_boot
		echo "image has bootloader"
	}

	return 0;
}

platform_do_upgrade() {
	platform_do_upgrade_hiwifi "$ARGV"
}
