#!/bin/sh
# Copyright (C) 2013-2014 www.hiwifi.com

. /lib/platform.sh

STORAGE_PATH=/tmp/data
EXTRA_STORAGE_PATH=/tmp/bigdata
STORAGE_MOUNT_PREFIX=/tmp/storage
STORAGE_STATE_FILE=$STORAGE_MOUNT_PREFIX/storage_state
STORAGE_LOG_FILE=$STORAGE_MOUNT_PREFIX/storage_hotplug.log
STORAGE_INFO_FILE=$STORAGE_MOUNT_PREFIX/storage_info.txt
STORAGE_FORCE_FORMAT=$STORAGE_MOUNT_PREFIX/storage_forceformat
CRYPT_MOUNT_POINT=/opt

hiwifi_storage_log_init() {
	echo "$@" >$STORAGE_LOG_FILE
}

hiwifi_storage_log() {
	echo "$@" >>$STORAGE_LOG_FILE
}

hiwifi_hotplug_cb() {
	local action=$1

	for restart_file in /etc/hotplug.d/block/restart.d/* ; do
		$restart_file restart
	done

	for action_file in /etc/hotplug.d/block/action.d/* ; do
		$action_file $action
	done
}

hiwifi_storage_state_set() {
	echo $1 >$STORAGE_STATE_FILE
}

hiwifi_partition() {
	hiwifi_storage_log "$1 start auto partition"
	hiwifi_partition_cmd=/tmp/hiwifi-fdisk.cmd
cat <<EOF >$hiwifi_partition_cmd
p
o
n
p
1
1
+1024M
n
p
2


p
w
EOF
	fdisk $1 <$hiwifi_partition_cmd
	rm -rf $hiwifi_partition_cmd
	sync
}

# $1 dev_path
# $2 ID_FS_TYPE(filesystem type)
# $3 dev_mount_point 
hiwifi_mount_fs() {
	mkdir -p "$3" &>/dev/null
	if [ "$2" == "msdos" -o "$2" == "vfat" ] ; then
		mount -t vfat -o noatime,fmask=0000,dmask=0000,iocharset=utf8 "$1" "$3"
		if [ $? -eq "0" ]; then
			return 0
		fi
	elif [ "$2" == "ntfs" ] ; then
		ntfs-3g -o big_writes,noatime,nls=utf8 "$1" "$3"
		if [ $? -eq "0" ]; then				
			return 0
		fi
	elif [ "$2" == "exfat" ] ; then
		mount -t exfat -o noatime,fmask=0,dmask=0,iocharset=utf8 "$1" "$3"
		if [ $? -eq "0" ]; then			
			return 0
		fi			
	elif [ "$2" == "ext4" -o "$2" == "ext3" -o "$2" == "ext2" ] ; then
		e2fsck -y "$1"
		mount -t "$2" -o noatime "$1" "$3"
		if [ $? -eq "0" ]; then
			return 0
		fi
	fi
	
	rm -rf "$3" &>/dev/null

	return 1
}

# $1 storage size
# $2 filesystem type
# $3 status(ro or rw)
hiwifi_storage_info() {
cat <<EOF >$STORAGE_INFO_FILE
size=$1
fstype=$2
status=$3
EOF
}

# $1 dev_path
# $2 dev_mount_point
# $3 ID_FS_TYPE
# $4 FORCE
hiwifi_mount_with_ext4() {
	if [ "$3" != "ext4" ]; then
		if [ "$4" -eq "1" ]; then
			mkfs.ext4 "$1"
		else
			hiwifi_storage_state_set "not-formated"
			return 1
		fi
	else
		e2fsck -y "$1"
	fi

	mkdir -p "$2" &>/dev/null
	mount -t ext4 -o noatime "$1" "$2"
	if [ "$?" -eq "0" ]; then
		size=$(df -h | grep "$1" | awk '{print $2}')
		fstype="ext4"
		mount | grep "$1" | grep -qs "ro,"
		if [ $? -eq 0 ]; then
			status="ro"
		else
			status="rw"
		fi
		hiwifi_storage_info "$size" "$fstype" "$status"
		hiwifi_storage_state_set "mounted"
		return 0
	else
		hiwifi_storage_log "mount with ext4 failed: $?"
		hiwifi_storage_state_set "mount-failed"
		rm -rf "$2" &>/dev/null
	fi

	return 1
}

# $1 dev_path
# $2 ID_FS_TYPE
# $3 FORCE
hiwifi_mount_cryptdata() {
	. /lib/functions.sh
	local hwf_dm_passwd_file=/tmp/storage-dm-passwd

	mtdpart="$(find_mtd_part bdinfo)"

	hiwifi_storage_log "$1 filesystem is $2(mount to cryptdata)"

	[ -z "$mtdpart" ] && return 1

	# get the passwd
	echo $(/sbin/hi_crypto_key device-crypt-key) >$hwf_dm_passwd_file

	if [ "$3" -eq "1" ]; then
		# cleanup
		umount $CRYPT_MOUNT_POINT &>/dev/null
		cryptsetup luksClose cryptdata &>/dev/null

		cryptsetup luksOpen "$1" cryptdata -d $hwf_dm_passwd_file

		if [ "$?" -ne "0" ]; then
			hiwifi_storage_log "cryptsetup"
			# TODO: The size is not checked now.
			yes YES | cryptsetup luksFormat $1 -c aes-cbc-plain -d $hwf_dm_passwd_file
			cryptsetup luksOpen $1 cryptdata -d $hwf_dm_passwd_file
		fi

		mkdir -p $CRYPT_MOUNT_POINT &>/dev/null
		mount -t ext4 /dev/mapper/cryptdata $CRYPT_MOUNT_POINT
		[ $? -ne 0 ] && {
			hiwifi_storage_log "mkfs.ext4"
			mkfs.ext4 /dev/mapper/cryptdata
			mount -t ext4 /dev/mapper/cryptdata $CRYPT_MOUNT_POINT
		}
	else
		if [ "$2" != "crypto_LUKS" ]; then
			hiwifi_storage_state_set "not-formated"
		else
			hiwifi_storage_log "$1 filessytem is crypto_LUKS"
			# cleanup
			umount $CRYPT_MOUNT_POINT &>/dev/null
			cryptsetup luksClose cryptdata &>/dev/null

			cryptsetup luksOpen "$1" cryptdata -d $hwf_dm_passwd_file
			if [ "$?" -ne "0" ]; then
				hiwifi_storage_log "cryptsetup failed with $1"
				hiwifi_storage_log "restart cryptsetup"
				# TODO: The size is not checked now.
				yes YES | cryptsetup luksFormat $1 -c aes-cbc-plain -d $hwf_dm_passwd_file
				cryptsetup luksOpen $1 cryptdata -d $hwf_dm_passwd_file				
			fi

			mkdir -p $CRYPT_MOUNT_POINT &>/dev/null
			mount -t ext4 /dev/mapper/cryptdata $CRYPT_MOUNT_POINT
			[ "$?" -ne "0" ] && {
				hiwifi_storage_log "mount /dev/mapper/cryptdata to $CRYPT_MOUNT_POINT with ext4 failed"
				hiwifi_storage_log "mkfs.ext4"
				mkfs.ext4 /dev/mapper/cryptdata
				mount -t ext4 /dev/mapper/cryptdata $CRYPT_MOUNT_POINT
			}
		fi
	fi

	return 0
}

# $1 dev_path
# $2 dev_mount_point
hiwifi_start_storage() {
	mount | grep "$1" | grep -qs "ro,"
	if [ $? -eq 0 ]; then
		return
	fi

	[ ! -L $STORAGE_PATH ] && cp -rf $STORAGE_PATH/* $2
	rm -rf $STORAGE_PATH
	ln -ns $2 $STORAGE_PATH

	[ ! -e $STORAGE_PATH/var/lib ] && mkdir -p $STORAGE_PATH/var/lib

	hiwifi_hotplug_cb start
}

hiwifi_stop_storage() {
	hiwifi_storage_log "hiwifi_stop_storage $@"

	if [ -L $STORAGE_PATH ]; then
		hiwifi_storage_log "hiwifi_stop_storage remove $STORAGE_PATH"
		rm -rf $STORAGE_PATH
		mkdir -p $STORAGE_PATH
		hiwifi_hotplug_cb stop
	fi

	rm -rf $STORAGE_INFO_FILE

	hiwifi_storage_state_set "umounted"
}

hiwifi_storage_force_format() {
	local board=$(tw_board_name)
	local power=/sys/devices/platform//hiwifi:power/ata/value
	case "$board" in
		"HC5663")
			# power off
			echo  1 > $power
			sleep 20
			mkdir -p $STORAGE_MOUNT_PREFIX &>/dev/null
			touch $STORAGE_FORCE_FORMAT
			# power on to restart hotplug
			echo 0 > $power
			return 0
		;;
		*)
			return 1
		;;
	esac

	return 1
}

# $1 dev_path
# $2 ACTION
# $3 device
# $4 ID_FS_TYPE(filesytem type)
hiwifi_storage_handle() {
	local board=$(tw_board_name)
	local dev_mount_point="$STORAGE_MOUNT_PREFIX/$3"
	local part_num=$(echo "$1" | awk -F 'sd[a-z]' '{print $2}')
	mkdir -p $STORAGE_MOUNT_PREFIX &>/dev/null
	case "$2" in
		add)
			case "$board" in
				"HC5663")
					if [ -z $part_num ]; then
						hiwifi_storage_log_init "log start with $1 $2 $3 $4"
						fdisk -l "$1" | grep -qs "doesn't contain a valid partition table"
						# no partition table
						if [ "$?" -eq 0 -a -z $4 ]; then
							hiwifi_storage_log "$1 no valid partition table"
							# no partitions just auto formated
							hiwifi_partition "$1"
							hiwifi_storage_state_set "auto-formated"
						else
							if [ -f "$STORAGE_FORCE_FORMAT" ]; then
								hiwifi_storage_log "force format storage $1"
								hiwifi_partition "$1"
								hiwifi_storage_state_set "auto-formated"
								rm -rf "$STORAGE_FORCE_FORMAT"
								return 1
							fi

							# 3 or more partitions
							fdisk -l "$1" | grep -qs "$1"3
							if [ "$?" -eq 0 ]; then
								hiwifi_storage_log "$1 more than 2 partitions"
								hiwifi_storage_state_set "not-formated"
								return 1
							fi

							# only 1 partitions
							fdisk -l "$1" | grep -qs "$1"2
							if [ "$?" -ne 0 ]; then
								hiwifi_storage_log "$1 no the second partition"
								hiwifi_storage_state_set "not-formated"
								return 1
							fi

							hiwifi_storage_state_set "formated"
						fi
					else
						storage_state=$(cat $STORAGE_STATE_FILE)
						hiwifi_storage_log "$1 $2 $3 $4 $storage_state"
						case "$storage_state" in
							"auto-formated")
								# auto formated, need to create filesystem
								if [ "$part_num" -eq "1" ]; then
									hiwifi_storage_log "auto-formated mount with device $1 $4"
									hiwifi_mount_cryptdata "$1" "$4" "1"
								else
									[ -e /dev/mapper/cryptdata ] && {
										hiwifi_storage_log "auto-formated mount_ext4 with device $1 $4"
										hiwifi_mount_with_ext4 "$1" "$dev_mount_point" "$4" "1"
										[ $? -eq 0 ] && hiwifi_start_storage "$1" "$dev_mount_point"
									}
								fi
								;;
							"not-formated")
								return 1
							;;
							*)
								# only have two partitions, but the filesystem is unknown
								if [ "$part_num" -eq "1" ]; then
									# if not crypt partition, set to not-formated
									hiwifi_storage_log "formated mount with device $1 $4"
									hiwifi_mount_cryptdata "$1" "$4" "0"
								else
									[ -e /dev/mapper/cryptdata ] && {
										hiwifi_storage_log "formated mount_ext4 with device $1 $4"
										hiwifi_mount_with_ext4 "$1" "$dev_mount_point" "$4" "0"
										[ $? -eq 0 ] && hiwifi_start_storage "$1" "$dev_mount_point"
									}
								fi
							;;
						esac
					fi				
				;;	
				*)
					hiwifi_mount_fs "$1" "$4" "$dev_mount_point"
					rm -rf $EXTRA_STORAGE_PATH &>/dev/null
					ln -s $dev_mount_point $EXTRA_STORAGE_PATH
				;;
			esac
		;;		
		remove)
			case "$board" in
				"HC5663")
					hiwifi_storage_log "log end with $1 $2 $3"
					if [ "$part_num" -eq "1" ]; then
						umount -l $CRYPT_MOUNT_POINT &>/dev/null
						cryptsetup luksClose cryptdata &>/dev/null
					elif [ "$part_num" -eq "2" ]; then
						hiwifi_stop_storage
						# wait for process stop
						sleep 1
						umount -l $dev_mount_point &>/dev/null
						if [ "$?" -eq 0 ]; then
							rm -rf $dev_mount_point
						fi
						hiwifi_storage_state_set "removed"
					else
						umount -l $dev_mount_point &>/dev/null
						if [ "$?" -eq 0 ]; then
							rm -rf $dev_mount_point
						fi						
						hiwifi_storage_state_set "removed"
					fi
				;;
				*)
					rm -rf $EXTRA_STORAGE_PATH &>/dev/null
					mkdir -p $EXTRA_STORAGE_PATH
					umount $dev_mount_point &>/dev/null
				;;
			esac
		;;
	esac

	return 0	
}
