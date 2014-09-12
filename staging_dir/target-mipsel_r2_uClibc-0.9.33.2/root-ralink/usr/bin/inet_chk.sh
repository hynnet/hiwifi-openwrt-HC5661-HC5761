#!/bin/sh

. /lib/functions/network.sh
. /lib/functions/net_update_actions.sh

static_domains="www.qq.com t.cn"
needup=""
inetchk_state_log_file="/tmp/data/hwf_inetchk_state_log"
log_cnt=""

valid_gateway()
{
	local arp_entry
	local wan_type
	wan_type="$(uci get network.wan.proto)"
	if [[ "$wan_type" = "pppoe" ]];then
		return 0
	fi

	###:TODO
	network_get_gateway wan_gw wan
	if [[ "$?" -eq 0 ]];then
		arp_entry=$(cat /proc/net/arp | grep "^$wan_gw " 2> /dev/null)
		if [[ "$?" -eq "0" ]]; then
			echo $arp_entry | grep "00:00:00:00:00:00" &> /dev/null
			if [[ "$?" -ne "0" ]];then
				return 0
			fi
		fi
	fi

	return 1
}

get_wlan_relay_status()
{
	###FIXME:
	### 1, These codes what test wlan status should be move to /etc/hotplug.d.
	### Unfortunately, our system does't support reply wlan-relay-mode status in hotplug!
	### 2, In case of the unstable of wireless-rely mode, we do not use ping to test the network,
	### just let it go.

	wlan_st=$(uci get wireless.@wifi-iface[1].network 2>/dev/null)
	if [[ "${wlan_st:-?}" = "wan" ]]; then
		local wlan_iface=$(uci get network.wan.ifname)
		wan_link=$(lua -e "require 'hcwifi';print(hcwifi.get(\"$wlan_iface\",'status'))")
		return $wan_link
	fi

	return 0
}

chk_inet_state_by_wget()
{
	local rv=""

	wget -O /dev/null http://m.taobao.com/ &> /dev/null
	rv=$?

	if [ "$rv" -ne 0 ]; then
		wget -O /dev/null http://www.qq.com/ &> /dev/null
		rv=$?
	fi

	return $rv
}

chk_inet_state_by_curl()
{
	local rv=""

	curl -o /dev/null http://www.baidu.com/ &> /dev/null
	rv=$?

	if [ "$rv" -ne 0 ]; then
		curl -o /dev/null http://www.so.com/ &> /dev/null
		rv = $?
	fi

	return $rv
}

chk_inet_state_by_nslookup()
{
	nslookup baidu.com &> /dev/null
}

collect_inetchk_state_log()
{
	[[ "$1" != "$2"	]] && {
		[[ "$log_cnt" -gt "10000" ]] && {
			: > $inetchk_state_log_file
		}

		echo "$(date +%Y-%m-%d-%H-%M-%S):$2:$3" >> $inetchk_state_log_file
		log_cnt=$(( ++log_cnt ))
	}
}

chk_inet_state_by_sqos()
{
	local speed=$(awk '{if(match($0,/^=*$/)){target= NR +1} if(NR == target) print $5}' /proc/net/smartqos/stat 2>&-)

	[[ $? -eq 0 ]] && {
		if [[ "$speed" -gt 50 ]]; then
			return 0
		fi
	}

	return 1
}

inet_chk_update_action()
{
	local net_state=$1

	get_inet_chk_switch &> /dev/null
	if [ $? -ne 0 ];then
		if [[ "$net_state" = "inet_up" ]]; then
			netup_action
			needup="needup"
		else
			netdown_action	"$needup"
			needup=""
		fi
	else
		netup_action
		if [[ "$net_state" = "inet_up" ]]; then
			needup_tmp="needup"
			net_internet_led_update "inet_up"
		else
			net_internet_led_update "inet_down"

			if [[ "$needup_tmp" = "needup" ]];then
				net_autostart_wan
			fi
			needup_tmp=""
		fi
	fi
}

#Init inet state
netdown_action
netup_action

net_get_agreement
if [[ $? -eq 0 ]];then
	while [[ true ]];
	do
		netdown_action
		sleep 1
		net_get_agreement
		if [[ "$?" -ne 0 ]];then
			break
		fi
	done
	netup_action
fi

do_while_func()
{
	local net_state="inet_down"

	is_relay="$(lua -e 'local net=require "luci.controller.api.network"; print(net.is_bridge())')"

	old_state=$1

	chk_inet_state_by_sqos
	if [[ $? -eq 0 ]];then
		net_state="inet_up"
		collect_inetchk_state_log "$old_state" "$net_state" "sqos"
	elif [[ "$is_relay" = "true" ]]; then
		get_wlan_relay_status
		if [[ $? -eq 1 ]]; then
			net_state="inet_up"
		fi
		collect_inetchk_state_log "$old_state" "$net_state" "relay link"
	else
		valid_gateway
		if [[ "$?" -eq 0 ]];then
			for cur in $static_domains
			do
				ping $cur -c 1 -W 2 &>/dev/null
				if [[ $? -eq 0 ]]; then
					net_state="inet_up"
					break
				fi
			done
			collect_inetchk_state_log "$old_state" "$net_state" "ping"

			if [[ "$net_state" = "inet_down" ]]; then
				chk_inet_state_by_nslookup
				if [[ "$?" -eq 0 ]]; then
					net_state="inet_up"
				fi
			fi
			collect_inetchk_state_log "$old_state" "$net_state" "nslookup"

			if [[ "$net_state" = "inet_down" ]]; then
				chk_inet_state_by_curl
				if [[ "$?" -eq 0 ]]; then
					net_state="inet_up"
				fi
			fi
			collect_inetchk_state_log "$old_state" "$net_state" "curl"

			if [[ "$net_state" = "inet_down" ]]; then
				chk_inet_state_by_wget
				if [[ "$?" -eq 0 ]];then
					net_state="inet_up"
				fi
			fi

			collect_inetchk_state_log "$old_state" "$net_state" "wget"
		else
			collect_inetchk_state_log "$old_state" "$net_state" "gateway"
		fi
	fi

	collect_inetchk_state_log "$old_state" "$net_state" "final"

	old_state="$net_state"

	if [ "$old_state" = "inet_up" ];then
		return 0
	else
		return 1
	fi
}

: >  $inetchk_state_log_file
log_cnt=$(cat "$inetchk_state_log_file" 2> /dev/null | wc -l)

old_state="$(cat /tmp/state/inet_stat 2> /dev/null)"
needup_tmp="needup"
state=$old_state

while [[ true ]];
do
	( do_while_func $state )

	if [ $? -eq 0 ]; then
		state="inet_up"
	else
		state="inet_down"
	fi

	inet_chk_update_action $state

	sleep 5
done
