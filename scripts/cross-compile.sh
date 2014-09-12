#!/bin/bash

#
# Script for "cheating" Bash with $PATH set to a directory which
# contains  "fake" compiling utilities (gcc, g++, ld, cc, ...)
# symbolic-linked to OpenWrt cross-compilers.
# In this way developers can simply run "gcc", "g++" commands to
# cross-compile C/C++ files, or just run "make" to build simple
# Makefile-based C/C++ projects.
#
# Please run './cross-compile.sh' for usage.
#
# Author: Jianying Liu <jianying.liu@hiwifi.tw>
# Copyright: 2014 Beijing HiWiFi Co., Ltd
#

# By default we cheat local gcc, g++ commands by adding directory
# containing cross compilers to the head of $PATH.
GCC_CHEATING=Y

# Remote duplicate lines from file
uniq_file_lines()
{
	local file="$1"
	local tmpfile=__tmp.$$
	sort -u "$file"  > $tmpfile
	mv -f $tmpfile "$file" || rm -f $tmpfile
}

get_staging_target()
{
	local __dir
	for __dir in "$1"/staging_dir/target-*; do
		[ -d "$__dir"/usr ] && break
	done
	[ -d "$__dir" ] && echo "$__dir"
}

get_staging_toolchain()
{
	local __dir
	for __dir in "$1"/staging_dir/toolchain-*; do
		[ -f "$__dir"/include/stdio.h ] && break
	done
	[ -d "$__dir" ] && echo "$__dir"
}

do_start()
{
	local openwrt_root=`readlink -f "$1"`; shift 1
	local staging_dir="$openwrt_root"/staging_dir
	local staging_target=`get_staging_target "$openwrt_root"`
	local staging_toolchain=`get_staging_toolchain "$openwrt_root"`
	local kernsrc="X"

	if [ ! -d "$staging_toolchain" ]; then
		echo "*** Cannot get a possible \"staging_dir/toolchain-*\" directory. Please make sure you are under OpenWrt source root:"
		exit 1
	fi
	if [ ! -d "$staging_target" ]; then
		echo "Warning: Directory \"staging_dir/target-*\" does not exist. Using a toolchain?"
	fi

	eval `grep '^CONFIG_ARCH=' "$openwrt_root/.config" | head -n1`
	local arch_type="$CONFIG_ARCH"
	[ -z "$arch_type" ] && arch_type=`basename "$openwrt_root"`

	local fake_bin="$openwrt_root"/.fakebin
	local link_log="$fake_bin"/links.log

	# Find the kernel source tree directory */
	local i
	for i in "$openwrt_root"/build_dir/linux-*/linux-* \
	  "$openwrt_root"/build_dir/linux-*/linux-*  \
	  "$openwrt_root"/build_dir/target-*/linux-*/linux-*; do
		if [ -d "$i"/arch/mips ]; then
			kernsrc="$i"
			break
		fi
	done
	
	mkdir -p "$fake_bin"
	touch "$link_log"

	# Link: gcc -> mips-openwrt-linux-gcc
	for i in addr2line ar as c++ c++filt cpp elfedit g++ gcc gcc-4.6.4 gcov gdb \
	  gprof ld ld.bfd nm objcopy objdump ranlib readelf size strings strip; do
		[ -e "$fake_bin/$i" ] && continue
		local dstbin=`echo "$staging_toolchain"/bin/*-openwrt-linux-$i`
		[ -e "$dstbin" ] || continue
		if file -Lbi "$dstbin" | grep -i 'text\/.*script' >/dev/null; then
			rm -f "$fake_bin/$i"
			cat > "$fake_bin/$i" <<EOF
#!/bin/sh
exec "$dstbin" "\$@"
EOF
			chmod +x "$fake_bin/$i"
		else
			ln -sf "$dstbin" "$fake_bin/$i"
		fi
	done
	[ -e $fake_bin/cc ] || ln -sf gcc $fake_bin/cc
	
	# Place a fake 'uname' command their to cheat Linux kernel's Makefile
	if [ ! -e $fake_bin/uname ]; then
		# Get the kernel version
		local kernver=3.3.8
		[ -d "$kernsrc" ] && kernver=`basename "$kernsrc" | sed  's/^linux-//'`

		cat > $fake_bin/uname <<EOF
#!/bin/sh
case "\$1" in
	-m) echo mips;;
	-r) echo $kernver;;
	-n) hostname;;
	-s|"") echo Linux;;
	-a) echo "Linux \`hostname\` $kernver #1 \`date\` mips GNU/Linux";;
	*)
		echo "Fake uname for cross-compiling kernel modules."
		echo "Usage: uname [-m|-r|-n|-s|-a]"
		;;
esac
EOF
		chmod +x $fake_bin/uname
	fi

	if [ -d $staging_target/usr ]; then
		# Link: toochain/.../lib/libevent.so -> target/.../lib/libevent.so
		for i in $staging_target/usr/lib/*; do
			local file=`basename "$i"`
			if [ -e $staging_toolchain/lib/$file ]; then
				continue
			else
				ln -s $staging_target/usr/lib/$file $staging_toolchain/lib/$file
				echo $staging_toolchain/lib/$file >> $link_log
			fi
		done

		# Link extra header files
		for i in $staging_target/usr/include/*; do
			local file=`basename "$i"`
			if [ -e $staging_toolchain/include/$file ]; then
				continue
			else
				ln -s $staging_target/usr/include/$file $staging_toolchain/include/$file
				echo $staging_toolchain/include/$file >> $link_log
			fi
		done
	fi

	# Remove duplicate items
	uniq_file_lines $link_log

	PATH="$staging_toolchain/bin:$PATH"
	[ "$GCC_CHEATING" = Y ] && PATH="$fake_bin:$PATH"

	export PATH
	export STAGING_DIR="$staging_dir"
	export KERNSRC="$kernsrc"
	export KERNELPATH="$kernsrc"

	if [ $# -eq 0 ]; then
		exec bash --rcfile <( [ -f ~/.bashrc ] && cat ~/.bashrc; echo "PS1='[\[\e[1;33m\]cross-compile@$arch_type:\[\e[m\]\w]\$ '" )
	else
		exec "$@"
	fi
}

do_clean()
{
	local openwrt_root=`readlink -f "$1"`
	local fake_bin="$openwrt_root"/.fakebin
	local link_log="$fake_bin"/links.log

	if [ -d $fake_bin ]; then
		if [ -f $link_log ]; then
			local file
			while read file; do
				rm -vf $file
			done < $link_log
		fi
		rm -vrf $fake_bin
		echo ">>> Done! Removed all attached symbolic links. <<<"
	fi
}

print_help()
{
cat <<EOF
Script for "cheating" Bash with \$PATH set to a directory which contains  "fake" 
compiling utilities (gcc, g++, ld, cc, ...) symbolic-linked to OpenWrt cross-compilers.
In this way developers can simply run "gcc", "g++" commands to cross-compile C/C++
files, or just run "make" to build simple Makefile-based C/C++ projects.

Usage:
 $0 <openwrt_root>
         ... setup cross compiling with specified OpenWrt source tree,
             cross compilers can either be invoked by 'xxxx-openwrt-linux-gcc'
             or 'gcc'
 $0 <openwrt_root> -n|--nogcc
         ... similar as above but do not cheat local compilers, but
             cross compilers can only be invoked by 'xxxx-openwrt-linux-gcc'
 $0 <openwrt_root> -c|--clean
         ... remove all temporary symbolic links and directories

Instructions for different cross-compiling scenarios:
 1. "GNU configure" based projects:
   * Download and unpack the code to be compiled, change into the unpacked directory;
   * Run "./cross-compile.sh <openwrt_root> -n", entering a child shell with directory
     of "xxxx-openwrt-linux-gcc" set to PATH;
   * "./configure" with option "--host=xxxx-openwrt-linux", e.g.,
     ./configure --host=mips-openwrt-linux
   * make
 2. "GNU make" based projects:
   * Run "./cross-compile.sh <openwrt_root>", entering a child shell with directories
     of both "xxxx-openwrt-linux-gcc" and FAKE "gcc" set to PATH;
   * Change into the project directory;
   * make
EOF
}

# Start here
openwrt_root="$1"; shift 1
if [ -d "$openwrt_root" ]; then
	case "$1" in
		-h) print_help;;
		-c|--clean) do_clean "$openwrt_root";;
		-n|--nogcc) GCC_CHEATING=N; shift 1; do_start "$openwrt_root" "$@";;
		*) do_start "$openwrt_root" "$@";;
	esac
else
	print_help
fi

