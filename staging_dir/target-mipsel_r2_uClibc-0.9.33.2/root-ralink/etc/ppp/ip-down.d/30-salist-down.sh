#!/bin/sh

LAN_DEVICE=`uci -P/var/state get network.lan.ifname`

stop_vpn_routing()
{
	while iptables -t mangle -D OUTPUT -j VPN_ROUTING 2>/dev/null; do :; done
	while iptables -t mangle -D PREROUTING -i $LAN_DEVICE -j VPN_ROUTING 2>/dev/null; do :; done
	iptables -t mangle -F VPN_ROUTING 2>/dev/null
	iptables -t mangle -X VPN_ROUTING 2>/dev/null

	# We don't have to delete the default route in 'virtual', since
	# it will be brought down along with the interface.
	while ip rule del fwmark 199 table virtual 2>/dev/null; do :; done
}

del_hostlist()
{
	rm -f /tmp/dnsmasq.d/vpn
}

if [ "$6" = "vpn" ]; then
	stop_vpn_routing
	del_hostlist
	/etc/init.d/dnsmasq restart
fi

