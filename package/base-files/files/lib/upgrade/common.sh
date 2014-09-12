#!/bin/sh

RAM_ROOT=/tmp/root

ldd() { LD_TRACE_LOADED_OBJECTS=1 $*; }
libs() { ldd $* | awk '{print $3}'; }

install_file() { # <file> [ <file> ... ]
	for file in "$@"; do
		dest="$RAM_ROOT/$file"
		[ -f $file -a ! -f $dest ] && {
			dir="$(dirname $dest)"
			mkdir -p "$dir"
			cp $file $dest
		}
	done
}

install_bin() { # <file> [ <symlink> ... ]
	src=$1
	files=$1
	[ -x "$src" ] && files="$src $(libs $src)"
	install_file $files
	[ -e /lib/ld-linux.so.3 ] && {
		install_file /lib/ld-linux.so.3
	}
	shift
	for link in "$@"; do {
		dest="$RAM_ROOT/$link"
		dir="$(dirname $dest)"
		mkdir -p "$dir"
		[ -f "$dest" ] || ln -s $src $dest
	}; done
}

pivot() { # <new_root> <old_root>
	/bin/mount | grep "on $1 type" 2>&- 1>&- || /bin/mount -o bind $1 $1
	mkdir -p $1$2 $1/proc $1/sys $1/dev $1/tmp $1/overlay && \
	/bin/mount -o noatime,move /proc $1/proc && \
	pivot_root $1 $1$2 || {
       /bin/umount -l $1 $1
		return 1
	}

	/bin/mount -o noatime,move $2/sys /sys
	/bin/mount -o noatime,move $2/dev /dev
	/bin/mount -o noatime,move $2/tmp /tmp
	/bin/mount -o noatime,move $2/overlay /overlay 2>&-
	return 0
}

run_ramfs() { # <command> [...]
	install_bin /bin/busybox /bin/ash /bin/sh /bin/mount /bin/umount        \
		/sbin/pivot_root /usr/bin/wget /sbin/reboot /bin/sync /bin/dd   \
		/bin/grep /bin/cp /bin/mv /bin/tar /usr/bin/md5sum "/usr/bin/[" \
		/bin/vi /bin/ls /bin/cat /usr/bin/awk /usr/bin/hexdump          \
		/bin/sleep /bin/zcat /usr/bin/bzcat /usr/bin/printf /usr/bin/wc

	install_bin /sbin/mtd
	install_bin /sbin/setled
	install_bin /bin/ps
	install_bin /bin/dmesg
	for file in $RAMFS_COPY_BIN; do
		install_bin $file
	done
	install_file /etc/resolv.conf /lib/functions.sh /lib/functions.sh /lib/upgrade/*.sh $RAMFS_COPY_DATA

	pivot $RAM_ROOT /mnt || {
		echo "Failed to switch over to ramfs. Please reboot."
		echo "Failed to switch over to ramfs. Please reboot." >>$SYSUPGRADE_LOG
		exit 1
	}

	/bin/mount -o remount,ro /mnt
	/bin/umount -l /mnt

	grep /overlay /proc/mounts > /dev/null && {
		/bin/mount -o noatime,remount,ro /overlay
		/bin/umount -l /overlay
	}
	# wait for jffs2 kernel thread dead
	sleep 1

	# spawn a new shell from ramdisk to reduce the probability of cache issues
	exec /bin/busybox ash -c "$*"
}

kill_remaining() { # [ <signal> ]
	local sig="${1:-TERM}"
	echo -n "Sending $sig to remaining processes ... "

	local stat
	for stat in /proc/[0-9]*/stat; do
		[ -f "$stat" ] || continue

		local pid name state ppid rest
		read pid name state ppid rest < $stat
		name="${name#(}"; name="${name%)}"

		local cmdline
		read cmdline < /proc/$pid/cmdline

		# Skip kernel threads 
		[ -n "$cmdline" ] || continue

		case "$name" in
			# Skip essential services
			*ash*|*init*|*ssh*|*dropbear*|*login*) : ;;

			# Killable process
			*)
				if [ $pid -ne $$ ] && [ $ppid -ne $$ ]; then
					echo -n "$name "
					kill -$sig $pid 2>/dev/null
				fi
			;;
		esac
	done
	echo ""
}

run_hooks() {
	local arg="$1"; shift
	for func in "$@"; do
		eval "$func $arg"
	done
}

ask_bool() {
	local default="$1"; shift;
	local answer="$default"

	[ "$INTERACTIVE" -eq 1 ] && {
		case "$default" in
			0) echo -n "$* (y/N): ";;
			*) echo -n "$* (Y/n): ";;
		esac
		read answer
		case "$answer" in
			y*) answer=1;;
			n*) answer=0;;
			*) answer="$default";;
		esac
	}
	[ "$answer" -gt 0 ]
}

v() {
	[ "$VERBOSE" -ge 1 ] && echo "$@"
}

rootfs_type() {
	/bin/mount | awk '($3 ~ /^\/$/) && ($5 !~ /rootfs/) { print $5 }'
}

get_image() { # <source> [ <command> ]
	local from="$1"
	local conc="$2"
	local cmd

	case "$from" in
		http://*|ftp://*) cmd="wget -O- -q";;
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

get_pdtname_up() {
	get_image "$@" | dd bs=2k skip=3328 count=1 2>/dev/null | dd bs=32 skip=1 count=1 2>/dev/null
}

get_pdtname_smt() {
	get_image "$@" | dd bs=2k skip=3392 count=1 2>/dev/null | dd bs=32 skip=1 count=1 2>/dev/null
}

get_pdtname_KR_up() {
	get_image "$@" | dd bs=32 skip=1 count=1 2>/dev/null
}

get_pdtname_KR_smt() {
	get_image "$@" | dd bs=2k skip=64 count=1 2>/dev/null | dd bs=32 skip=1 count=1 2>/dev/null
}

refresh_mtd_partitions() {
	mtd refresh rootfs
}

jffs2_copy_config() {
	if grep rootfs_data /proc/mtd >/dev/null; then
		# squashfs+jffs2
		mtd -e rootfs_data jffs2write "$CONF_TAR" rootfs_data
	else
		# jffs2
		mtd jffs2write "$CONF_TAR" rootfs
	fi
}

default_do_upgrade() {
	sync
	if [ "$SAVE_CONFIG" -eq 1 -a -z "$USE_REFRESH" ]; then
		get_image "$1" | mtd -j "$CONF_TAR" write - "${PART_NAME:-image}"
	else
		get_image "$1" | mtd write - "${PART_NAME:-image}"
	fi
}

do_upgrade() {
	v "Performing system upgrade..."
	if type 'platform_do_upgrade' >/dev/null 2>/dev/null; then
		platform_do_upgrade "$ARGV"
	else
		default_do_upgrade "$ARGV"
	fi

	[ "$SAVE_CONFIG" -eq 1 -a -n "$USE_REFRESH" ] && {
		v "Refreshing partitions"
		if type 'platform_refresh_partitions' >/dev/null 2>/dev/null; then
			platform_refresh_partitions
		else
			refresh_mtd_partitions
		fi
		if type 'platform_copy_config' >/dev/null 2>/dev/null; then
			platform_copy_config
		else
			jffs2_copy_config
		fi
	}
	echo "Upgrade completed" >>$SYSUPGRADE_LOG
	v "Upgrade completed"
	echo "++++++TASK++++++" >>$SYSUPGRADE_LOG
	ps >>$SYSUPGRADE_LOG
	echo "++++++MEMINFO++++++" >>$SYSUPGRADE_LOG
	cat /proc/meminfo >>$SYSUPGRADE_LOG
	echo "++++++DMESG++++++" >>$SYSUPGRADE_LOG
	dmesg >>$SYSUPGRADE_LOG

	[ -n "$DELAY" ] && sleep "$DELAY"
	ask_bool 1 "Reboot" && {
		v "Rebooting system..."
		echo "Rebooting system..." >>$SYSUPGRADE_LOG
		sync
		reboot -f
		sleep 5
		echo b 2>/dev/null >/proc/sysrq-trigger
	}
}
