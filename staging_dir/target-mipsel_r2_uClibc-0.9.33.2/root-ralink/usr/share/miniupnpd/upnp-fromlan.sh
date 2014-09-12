#!/bin/sh

# Copyright (c) 2013 Beijing HiWiFi Co., Ltd.
# Author: Jianying Liu <jianying.liu@hiwifi.tw>
# Description: This script is to fix UPNP port mapping from
#  WAN to LAN but accessed from LAN.
#

add()
{
	local proto="$1"
	local ext_port="$2"
	local int_addr="$3"
	local int_port="$4"

	. /lib/functions/network.sh
	network_get_subnet lan_subnet lan
	network_get_ipaddr wan_ipaddr wan

	iptables -t nat -A miniupnpd_fromlan -s $lan_subnet -d $wan_ipaddr -p $proto --dport $ext_port -j DNAT --to-destination $int_addr:$int_port
	iptables -t nat -A miniupnpd_tolan -s $lan_subnet -d $int_addr -p $proto --dport $int_port -j MASQUERADE
	iptables -A miniupnpd_fwd -d $int_addr -p $proto --dport $int_port -j ACCEPT
}

remove()
{
	local proto="$1"
	local ext_port="$2"
	local int_addr="$3"
	local int_port="$4"

	. /lib/functions/network.sh
	network_get_subnet lan_subnet lan
	network_get_ipaddr wan_ipaddr wan

	iptables -t nat -D miniupnpd_fromlan -s $lan_subnet -d $wan_ipaddr -p $proto --dport $ext_port -j DNAT --to-destination $int_addr:$int_port
	iptables -t nat -D miniupnpd_tolan -s $lan_subnet -d $int_addr -p $proto --dport $int_port -j MASQUERADE
	iptables -D miniupnpd_fwd -d $int_addr -p $proto --dport $int_port -j ACCEPT
}

case "$1" in
	add) shift 1; add "$@";;
	remove) shift 1; remove "$@";;
	*) echo "Usage: $1 add|remove <proto> <ext_port> <int_addr> <int_port>";;
esac

