#!/bin/sh

# Add policy route tables:
# echo '174     basic' >> /etc/iproute2/rt_tables
# echo '175     virtual' >> /etc/iproute2/rt_tables

# $1: The device name of VPN interface
VPN_DEVICE=$1
# $6: The OpenWrt alias of VPN interface
VPN_ALIAS=$6
LAN_DEVICE=`uci -P/var/state get network.lan.ifname`

rollback_on_failure()
{
	while iptables -t mangle -D OUTPUT -j VPN_ROUTING 2>/dev/null; do :; done
	while iptables -t mangle -D PREROUTING -i $LAN_DEVICE -j VPN_ROUTING 2>/dev/null; do :; done
	iptables -t mangle -F VPN_ROUTING 2>/dev/null
	iptables -t mangle -X VPN_ROUTING 2>/dev/null

	# We don't have to delete the default route in 'virtual', since
	# it will be brought down along with the interface.
	while ip rule del fwmark 199 table virtual 2>/dev/null; do :; done
}

start_vpn_routing()
{
	local vpn_server=`uci get network.$VPN_ALIAS.server`
	local default_rt=`uci get network.$VPN_ALIAS.defaultroute || echo 1` # NOTICE: global mode by default
	
	if ! grep '^175' /etc/iproute2/rt_tables >/dev/null; then
		( echo ""; echo '175     virtual' ) >> /etc/iproute2/rt_tables
	fi
	
	if [ $default_rt = 0 ]; then
		# 'defaultroute = 0' indicates 'intelligent mode', so only
		#  in this mode we use policy routing
		
		# Append user's local subnets to 'local' table
		local wan_subnet lan_subnet
		. /lib/functions/network.sh
		network_get_subnet lan_subnet lan
		network_get_subnet wan_subnet wan
		echo $lan_subnet > /proc/nf_salist/local
		echo $wan_subnet > /proc/nf_salist/local
		
		if [ -z "$VPN_DEVICE" -o -z "$LAN_DEVICE" -o -z "$vpn_server" ]; then
			# FIXME: Need to syslog here
			echo "*** Cannot get at least one of VPN_DEVICE, LAN_DEVICE, vpn_server." >&2
			exit 1
		fi
		
		# Route the marked traffic to VPN interface
		if ! ip route add default dev $VPN_DEVICE table virtual; then
			echo "*** Unexpected error while setting default route for table 'virtual'."
			exit 1
		fi
		ip rule add fwmark 199 table virtual
		
		iptables -t mangle -N VPN_ROUTING
		# Mark all 'to abroad' packets for policy routing
		iptables -t mangle -A VPN_ROUTING -d "$vpn_server" -j RETURN
		# NOTICE: If we failed to set --salist local, we must not start VPN
		if ! iptables -t mangle -A VPN_ROUTING -m salist --salist local --match-dip -j RETURN; then
			echo "*** Failed to use salist table 'local'."
			rollback_on_failure
			exit 1
		fi
		iptables -t mangle -A VPN_ROUTING -m salist --salist china --match-dip -j RETURN
		iptables -t mangle -A VPN_ROUTING -j MARK --set-mark 199
		
		iptables -t mangle -I PREROUTING -i $LAN_DEVICE -j VPN_ROUTING
		iptables -t mangle -I OUTPUT -j VPN_ROUTING
	else
		# FIXME: Route HiWiFi networks to the original default route.
		:
	fi
	
}

add_hostlist()
{
    [ -e /tmp/dnsmasq.d/vpn ] && rm /tmp/dnsmasq.d/vpn
    while read HOST ; do
        echo server=/$HOST/8.8.8.8  >> /tmp/dnsmasq.d/vpn
    done < /etc/vpn/hostlist && /etc/init.d/dnsmasq restart
}

if [ "$6" = "vpn" ]; then
	start_vpn_routing
	add_hostlist
	/etc/init.d/dnsmasq restart
fi

