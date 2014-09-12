#!/bin/sh
#
# Copyright (C) 2012-2013 hiwifi.com
#

do_ralink() {
	. /lib/ralink.sh

	ralink_board_detect
}

boot_hook_add preinit_main do_ralink
