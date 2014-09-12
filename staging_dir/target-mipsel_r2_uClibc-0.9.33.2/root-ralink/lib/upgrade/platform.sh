#
# Copyright (C) 2012-2014 www.hiwifi.com
#

. /lib/ralink.sh

PART_NAME=firmware
BOOT_NAME=u-boot
RAMFS_COPY_DATA=/lib/ralink.sh

platform_check_image() {
	local name="HIWIFI"
	local machine=$(awk 'BEGIN{FS="[ \t]+:[ \t]"} /machine/ {print $2}' /proc/cpuinfo)
	local board=$(tw_board_name)
	local pci_devices=$(cat /proc/bus/pci/devices | wc -l)
	local magic="$(get_magic_word "$1")"
	local magic_long="$(get_magic_long "$1")"
	local magic_boot="$(get_magic_boot "$1")"
	local board_len="$(echo "$board" | wc -L)"
	local pdtname_up=$(get_board_name_up "$1" | cut -c 1-"$board_len")
	local pdtname_smt=$(get_board_name_smt "$1" | cut -c 1-"$board_len")

	[ "$ARGC" -gt 1 ] && return 1

	case "$board" in
	HC5661 | HC5761 | HB5981m | HB5981s | HC5641 | HC5663 | HC5861 |\
	HB5881 | BL-H750AC | BL-855R | HB5811)
		[ "$magic_long" != "27051956" -a "$magic" != "2705" -a "$magic_boot" != "ff00001000000000fd00001000000000" ] && {
			echo "Invalid image type."
			return 1
		}
		[ "$pdtname_up" != "$board" -a "$pdtname_smt" != "$board" ] && {
			[ "$pdtname_up" != "$name" -a "$pdtname_smt" != "$name" ] && {
				echo "Invalid image board."
				return 1
			}
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

platform_do_upgrade() {
	local board=$(tw_board_name)

	case "$board" in
	HC5661 | HC5761 | HB5981m | HB5981s | HC5641 | HC5663 | HC5861 |\
	HB5881 | BL-H750AC | BL-855R | HB5811)
		platform_do_upgrade_hiwifi "$ARGV"
		;;
	*)
		default_do_upgrade "$ARGV"
		;;
	esac
}

platform_firmware_save() {
	local hwf_firmware_path="/tmp/data/hiwifi"

	if [ -L /tmp/data ]; then
		v "Save firmware"
		mkdir -p "$hwf_firmware_path"
		cp -f $1 $hwf_firmware_path/rom.bin
		md5sum $hwf_firmware_path/rom.bin > $hwf_firmware_path/rom.md5
	fi
}

disable_watchdog() {
	killall watchdog
	( ps | grep -v 'grep' | grep '/dev/watchdog' ) && {
		echo 'Could not disable watchdog'
		return 1
	}
}

append sysupgrade_pre_upgrade disable_watchdog
