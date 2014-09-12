init_guider_port=81
netup_port=81
netdown_port=83
lan_ipaddr=$(uci get network.lan.ipaddr)

net_get_agreement()
{
	need="$(awk -F ':' 'BEGIN{ need_init = 1 } {if(NR <= 2 && $2 == 0) { need_init = 0 } } END{print need_init }' /etc/agreement)"

	return $need
}

get_inet_chk_switch()
{
    local switch=$(uci get inet_chk.switch 2> /dev/null)
    
    if [ $? -ne 0 ]; then
        return 1
    else
        case "$switch" in
        "-1")
            return  1
            ;;  
        "0") 
            return 0
            ;;  
        *)  
            if [ $(date +%s) -lt $switch ];then
                return 0
            else
                /usr/bin/inet_chk_switch "on"
                return 1
            fi  
            ;;  
        esac    
    fi  
}

net_kproxy_update()
{
	net_get_agreement
	AGREEMENT=$?
	local netstate=$1
    
	if [[ "$AGREEMENT" -eq 0 ]]; then
	        kproxy -A -A ${lan_ipaddr}:$init_guider_port &>/dev/null
	        kproxy -A -F 1 &> /dev/null
	elif [[ "inet_up" = "$netstate" ]]; then
		kproxy -A -A ${lan_ipaddr}:$netup_port &> /dev/null
		kproxy -D -F 1 &> /dev/null
	else
		kproxy -A -F 1 &> /dev/null
		kproxy -A -A ${lan_ipaddr}:$netdown_port &> /dev/null
	fi  
}

net_inet_stat_file_update()
{
	local netstate=$1
	
	{
		flock 33
		grep "$netstate" /tmp/state/inet_stat &> /dev/null
		if [[ $? -eq 0 ]];then
			return 1
		else
			echo "$netstate" > /tmp/state/inet_stat 	
			return 0
		fi
	} 33>/var/run/net_update_actions.lock;
}

net_internet_led_update()
{
	local netstate=$1

	if [[ "inet_up" = "$netstate" ]];then
		[[ ! -e /etc/config/led_disable ]] &&  setled on green internet
	else
		setled off green internet
	fi
}

net_dnsmasq_update()
{
	local dnsmasq_pid=""

	dnsmasq_pid="$(cat /tmp/run/dnsmasq.pid 2> /dev/null)"	

	ps | awk -v pid=$dnsmasq_pid '{if (pid == $1 ) print $5}' | grep 'dnsmasq' &> /dev/null
	[[ "$?" -ne 0 ]] && return 1	
	
	kill  -SIGUSR2 $dnsmasq_pid	
}

net_autostart_wan()
{
	local wan_type
    
	wan_type="$(uci get network.wan.proto)"
	if [[ "$wan_type" != "pppoe" ]];then
		return 0
    fi 

	# When status change from alive to dead and autostart flag is set,
	# wan interface need to be restart
	ubus call network.interface.wan status|grep "autostart"|grep -qs "true"
	if [[ "$?" -eq 0 ]]; then
		ifup wan 
	fi
}

###Function name: netdown_action 
###Parameter $1 : Need ifup when inetdown?
netdown_action()
{
	###FIXME: 
	###Make sure the heisenbug whether produced by net_autostart_wan in hotplug.
	###Then move these code to the tail of this function.
	if [[ "$1" = "needup" ]]; then
		net_autostart_wan	
	fi

	net_inet_stat_file_update "inet_down"
	if [[ $? -ne 0 ]];then
		return 0
	fi
	
	net_internet_led_update "inet_down"

	net_kproxy_update "inet_down"
	
	#net_dnsmasq_update 
}

hwf_config_save()
{
	. /lib/functions.sh

	local i_file=$(uci get watchd.file.file 2>/dev/null)

	mtdpart="$(find_mtd_part hwf_config)"
	[ -z "$mtdpart" ] && return 1

	magic=$(dd if=$mtdpart bs=1 skip=0 2>/dev/null | hexdump -n 2 -e '2/1 "%02x"')

	if [ -n "$i_file" ]; then
		if [ -f "$i_file" ]; then
			hwf-config -s
			rm -rf $i_file
		else
			# no gzip magic number, save config
			if [ "$magic" != "1f8b" ]; then
				hwf-config -s
			fi
		fi
	fi
}

netup_action()
{
	net_inet_stat_file_update "inet_up"
	if [[ $? -ne 0 ]];then
		return 0
	fi

	net_internet_led_update "inet_up"

	net_kproxy_update "inet_up"
	
	#net_dnsmasq_update

	hwf_config_save 
}
