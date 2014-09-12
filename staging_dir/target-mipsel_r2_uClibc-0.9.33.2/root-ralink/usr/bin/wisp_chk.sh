#!/bin/sh

ifname=$(uci get wireless.@wifi-iface[1].ifname)

while [ $(uci get network.wan.ifname) = "$ifname" ];
do
	sleep 3
	[ -n "$(ifconfig $ifname | grep UP)" ] || {
		sleep 9
		continue
	}
	status=$(lua -e "require 'hcwifi';print(hcwifi.get(\"$ifname\",'status'))")
	[ $status -eq 0 ] || continue

	lua -e "require 'hcwifi';hcwifi.ctl(\"$ifname\",'scan')"
	sleep 4
	cnt=$(lua -e "require 'hcwifi';print(#hcwifi.get(\"$ifname\",'aplist'))")
	for idx in $(seq $cnt); do
		i=-1
		while [ $status -eq 0 ]; do
			i=$((${i}+1))
			ssid=$(uci get wireless.@wifi-iface[1].ssid${i} 2>/dev/null)
			[ -n "$ssid" ] || break
			bssid=$(uci get wireless.@wifi-iface[1].bssid${i} 2>/dev/null)
			encryption=$(uci get wireless.@wifi-iface[1].encryption${i})
			tbssid=$(lua -e "require 'hcwifi';table=hcwifi.get(\"$ifname\",'aplist');print(table[$idx].bssid)")
			tssid=$(lua -e "require 'hcwifi';table=hcwifi.get(\"$ifname\",'aplist');print(table[$idx].ssid)")
			tencryption=$(lua -e "require 'hcwifi';table=hcwifi.get(\"$ifname\",'aplist');print(table[$idx].auth)")
			[ -z "$bssid" ] || [ "$bssid" = "$tbssid" ] || continue
			[ "$ssid" = "$tssid" ] || continue
			[ "$encryption" = "$tencryption" ] || continue

			channel=$(lua -e "require 'hcwifi';table=hcwifi.get(\"$ifname\",'aplist');print(table[$idx].channel)")
			key=$(uci get wireless.@wifi-iface[1].key${i} 2>/dev/null)

			uci set wireless.radio0.channel="$channel"
			uci set wireless.@wifi-iface[1].bssid="$tbssid"
			uci set wireless.@wifi-iface[1].ssid="$ssid"
			uci set wireless.@wifi-iface[1].encryption="$encryption"
			uci set wireless.@wifi-iface[1].key="$key"

			ifup wan
			sleep 12
			status=$(lua -e "require 'hcwifi';print(hcwifi.get(\"$ifname\",'status'))")
		done
		[ $status -eq 0 ] || break
	done
	status=$(lua -e "require 'hcwifi';print(hcwifi.get(\"$ifname\",'status'))")
	[ $status -eq 0 ] && sleep 30
done
