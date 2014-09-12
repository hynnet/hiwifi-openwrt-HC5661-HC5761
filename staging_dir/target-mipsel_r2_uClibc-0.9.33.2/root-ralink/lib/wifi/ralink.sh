#!/bin/sh
# ralink.sh
# controlling script of ralink wireless driver for openwrt system
# (c)2013 Beijing Geek-Geek Technology Co., Ltd.

append DRIVERS "ralink"

. /lib/platform.sh

set_rt2860() {
	sed -i "/^$1=/d" /etc/wlan/RT2860.dat
	echo "$1=""$2" >> /etc/wlan/RT2860.dat
}

set_mt7610() {
	sed -i "/^$1=/d" /etc/wlan/iNIC_ap.dat
	echo "$1=""$2" >> /etc/wlan/iNIC_ap.dat
}

add_rt2860() {
	local name="$1"
	local vars="$2"
	sed -i 's/^'"$name"'=\(.*\)/&;'"$vars"'/g' /etc/wlan/RT2860.dat
}

add_mt7610() {
	local name="$1"
	local vars="$2"
	sed -i 's/^'"$name"'=\(.*\)/&;'"$vars"'/g' /etc/wlan/iNIC_ap.dat
}

start_ralink_vif() {
	local vif="$1"
	local ifname="$2"

	local net_cfg
	net_cfg="$(find_net_config "$vif")"
	[ -z "$net_cfg" ] || start_net "$ifname" "$net_cfg"
}

find_ralink_phy() {
	local device="$1"

	config_get vifs "$device" vifs
	for vif in $vifs; do
		config_get_bool disabled "$vif" disabled 0
		[ $disabled = 0 ] || continue
		return 0
	done

	return 1
}

scan_ralink() {
	[ -d /etc/wifi.d ] && {
		for SCRIPT in /etc/wifi.d/*
		do
			[ -x "$SCRIPT" ] && "$SCRIPT" "$@"
		done
	}
	return 0
}

enable_ralink() {
	local device="$1"
	config_get channel "$device" channel
	config_get txpwr "$device" txpwr
	config_get vifs "$device" vifs
	config_get hwmode "$device" hwmode
	config_get htbw "$device" htbw
	config_get txstream "$device" txstream
	config_get rxstream "$device" rxstream
	config_get_bool repeater "$device" repeater 0
	config_get_bool configured "$device" configured 0

	find_ralink_phy "$device" || return 0

	set_rt2860 Channel $channel
	if [ $channel -eq 0 ]; then
		set_rt2860 AutoChannelSelect 1
	else
		set_rt2860 AutoChannelSelect 0
	fi
	if [ $channel -ge 6 ]; then
		set_rt2860 HT_EXTCHA 0
	else
		set_rt2860 HT_EXTCHA 1
	fi

	[ "${hwmode:-11n}" = "11g" ] && set_rt2860 WirelessMode 0
	[ -n "$htbw" ] && set_rt2860 HT_BW $htbw
	[ -n "$txstream" ] && set_rt2860 HT_TxStream $txstream
	[ -n "$rxstream" ] && set_rt2860 HT_RxStream $rxstream
	[ -n "$repeater" ] && set_rt2860 ApCliRepeater $repeater

	case "$txpwr" in
		max*)
			set_rt2860 TxPower 100
			set_mt7610 TxPower 100
			;;
		mid*) 
			set_rt2860 TxPower 50
			set_mt7610 TxPower 50
			;;
		min*)
			set_rt2860 TxPower 10
			set_mt7610 TxPower 10
			;;
		   *)
			set_rt2860 TxPower "$txpwr"
			set_mt7610 TxPower "$txpwr"
			;;
	esac

	set_rt2860 ApCliEnable 0
	set_mt7610 ApCliEnable 0
	enwpad=0
	for vif in $vifs; do
		config_get_bool disabled "$vif" disabled 0
		[ $disabled = 0 ] || continue

		config_get ifname "$vif" ifname
		config_get mode "$vif" mode
		config_get ssid "$vif" ssid
		config_get bssid "$vif" bssid
		config_get macaddr "$vif" macaddr
		config_get hidden "$vif" hidden
		config_get txrate "$vif" txrate
		config_get macfilter "$vif" macfilter
		config_get maclist "$vif" maclist
		config_get encryption "$vif" encryption
		config_get key "$vif" key
		config_get rekey "$vif" rekey
		config_get ssidprefix "$vif" ssidprefix

		config_get_bool isolate "$vif" isolate 0

		[ "$ifname" = "apcli0" ] && {
			set_rt2860 ApCliEnable 1
			set_rt2860 ApCliSsid "$ssid"
			set_rt2860 ApCliBssid "$bssid"
			case "$encryption" in
			psk2)
				set_rt2860 ApCliAuthMode "WPA2PSK"
				set_rt2860 ApCliEncrypType "AES"
			;;
			psk)
				set_rt2860 ApCliAuthMode "WPAPSK"
				set_rt2860 ApCliEncrypType "AES"
			;;
			wpa2)
				set_rt2860 ApCliAuthMode "WPA2"
				set_rt2860 ApCliEncrypType "AES"
				enwpad=1
			;;
			wpa)
				set_rt2860 ApCliAuthMode "WPA"
				set_rt2860 ApCliEncrypType "AES"
				enwpad=1
			;;
			*)
				set_rt2860 ApCliAuthMode "OPEN"
				set_rt2860 ApCliEncrypType "NONE"
			;;
			esac
			set_rt2860 ApCliWPAPSK "$key"

			clone_mac=$(uci get network.wan.macaddr)
			[ -n "$clone_mac" ] && set_rt2860 ApCliMacCloneAddr "$clone_mac"

			continue
		}

		[ "$ifname" = "ra1" ] && {
			set_rt2860 SSID2 "$ssidprefix""$ssid"
			set_mt7610 SSID2 "$ssidprefix""$ssid""_5G"

			add_rt2860 NoForwarding $isolate
			add_mt7610 NoForwarding $isolate

			case "$encryption" in
			auto|mixed-psk*)
				add_rt2860 AuthMode "WPAPSKWPA2PSK"
				add_rt2860 EncrypType "AES"
				add_mt7610 AuthMode "WPAPSKWPA2PSK"
				add_mt7610 EncrypType "AES"
			;;
			mixed-wpa)
				add_rt2860 AuthMode "WPA1WPA2"
				add_rt2860 EncrypType "AES"
				add_mt7610 AuthMode "WPA1WPA2"
				add_mt7610 EncrypType "AES"
				enwpad=1
			;;
			*)
				add_rt2860 AuthMode "OPEN"
				add_rt2860 EncrypType "NONE"
				add_mt7610 AuthMode "OPEN"
				add_mt7610 EncrypType "NONE"
			;;
			esac

			[ -n "$key" ] && {
				set_rt2860 WPAPSK2 "$key"
				set_mt7610 WPAPSK2 "$key"
			}

			continue
		}

		[ -n "$ssid" ] && {
			set_rt2860 SSID1 "$ssidprefix""$ssid"
			set_mt7610 SSID1 "$ssidprefix""$ssid""_5G"
		}
		if [ "$hidden" = "enable" ]; then
			set_rt2860 HideSSID 1
			set_mt7610 HideSSID 1
		else
			set_rt2860 HideSSID 0
			set_mt7610 HideSSID 0
		fi
		if [ "$macfilter" = "allow" ]; then
			set_rt2860 AccessPolicy0 1
			set_rt2860 AccessPolicy1 1
			set_mt7610 AccessPolicy0 1
		elif [ "$macfilter" = "deny" ]; then
			set_rt2860 AccessPolicy0 2
			set_rt2860 AccessPolicy1 2
			set_mt7610 AccessPolicy0 2
		else
			set_rt2860 AccessPolicy0 0
			set_rt2860 AccessPolicy1 0
			set_mt7610 AccessPolicy0 0
		fi
		[ -n "$maclist" ] && {
			i=0
			for aclmac in $maclist; do
				if [ $i -eq 0 ]; then
					i=1
					set_rt2860 AccessControlList0 "$aclmac"
					set_rt2860 AccessControlList1 "$aclmac"
					set_mt7610 AccessControlList0 "$aclmac"
				else
					add_rt2860 AccessControlList0 "$aclmac"
					add_rt2860 AccessControlList1 "$aclmac"
					add_mt7610 AccessControlList0 "$aclmac"
				fi
			done
		}

		set_rt2860 NoForwarding $isolate
		set_mt7610 NoForwarding $isolate

		case "$encryption" in
			auto|mixed-psk*)
				set_rt2860 AuthMode "WPAPSKWPA2PSK"
				set_rt2860 EncrypType "AES"
				set_mt7610 AuthMode "WPAPSKWPA2PSK"
				set_mt7610 EncrypType "AES"
			;;
			mixed-wpa)
				set_rt2860 AuthMode "WPA1WPA2"
				set_rt2860 EncrypType "AES"
				set_mt7610 AuthMode "WPA1WPA2"
				set_mt7610 EncrypType "AES"
				enwpad=1
			;;
			*)
				set_rt2860 AuthMode "OPEN"
				set_rt2860 EncrypType "NONE"
				set_mt7610 AuthMode "OPEN"
				set_mt7610 EncrypType "NONE"
			;;
		esac

		[ -n "$key" ] && {
			set_rt2860 WPAPSK1 "$key"
			set_mt7610 WPAPSK1 "$key"
		}
		[ -n "$rekey" ] && {
			set_rt2860 RekeyInterval "$rekey"
			set_mt7610 RekeyInterval "$rekey"
		}
	done

	for vif in $vifs; do
		config_get_bool disabled "$vif" disabled 0
		[ $disabled = 0 ] || continue

		config_get ifname "$vif" ifname
		config_get ifname2 "$vif" ifname2
		config_get network "$vif" network

		[ "$network" = "wan" ] || {
			start_ralink_vif "$vif" "$ifname"
			start_ralink_vif "$vif" "$ifname2"
			set_wifi_up "$vif" "$ifname"
			ifconfig $ifname up
			ifconfig $ifname2 up 2>/dev/null
		}
	done

	if [ ! -e "/etc/config/led_disable" ]; then
		iwpriv ra0 set led=1
		iwpriv rai0 set led=1 2>/dev/null
	fi

	[ $enwpad -eq 1 ] && wpad & 
	 [ $repeater -eq 1 ] && [ $configured -eq 0 ] && {
	 	iwpriv apcli0 set WscConfMode=1
	 	iwpriv apcli0 set WscMode=2
	 	iwpriv apcli0 set WscGetConf=1
	 }

	return 0
}

disable_ralink() {
	local device="$1"
	config_get vifs "$device" vifs

	killall wpad 2>/dev/null
	set_wifi_down "$device"

	iwpriv ra0 set led=0 2>/dev/null
	iwpriv rai0 set led=0 2>/dev/null

	include /lib/network
	for vif in $vifs; do
		config_get ifname "$vif" ifname
		config_get ifname2 "$vif" ifname2
		config_get network "$vif" network

		[ "$network" = "wan" ] || {
			ifconfig $ifname down 2>/dev/null
			ifconfig $ifname2 down 2>/dev/null

			brctl show | grep -qs "$ifname"
			if [ $? -eq 0 ]; then
				unbridge1 $ifname $network 2>/dev/null
			fi
			brctl show | grep -qs "$ifname2"
			if [ $? -eq 0 ]; then
				unbridge1 $ifname2 $network 2>/dev/null
			fi
		}
	done

	return 0
}

detect_ralink() {
	devidx=0
	config_load wireless
	while :; do
		config_get type "radio$devidx" type
		[ -n "$type" ] || break
		devidx=$((devidx + 1))
		[ -n "$type" ] && [ "$type" != "ralink" ] && continue
		[ -n "$type" ] && [ "$type" == "ralink" ] && return
	done

	mac_suffix=$(tw_get_mac)
	mac_suffix=${mac_suffix:6:6}

	cat <<EOF
config wifi-device radio$devidx
	option type	ralink
	option channel	0
	option txpwr	max

config wifi-iface
	option device	radio$devidx
	option ifname	ra0
	option ifname2	rai0
	option network	lan
	option mode	ap
	option ssid	HiWiFi_${mac_suffix}
	option encryption none

config wifi-iface
	option device	radio$devidx
	option ifname	apcli0
	option network	lan
	option mode	sta
	option ssid	HiWiFi_${mac_suffix}
	option encryption none
	option disabled 1

config wifi-iface
	option device	radio$devidx
	option ifname	ra1
	option ifname2	rai1
	option network	lan
	option mode	ap
	option ssid	HiWiFi_guest
	option encryption none
	option isolate 	1
	option disabled 1
EOF
}
