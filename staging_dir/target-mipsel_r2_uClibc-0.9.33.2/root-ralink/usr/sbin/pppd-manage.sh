#!/bin/sh

username=
password=
dft_cfg=/etc/ppp/account/gddx-default
new_cfg=/etc/ppp/account/gddx-new
tmp_cfg=/etc/ppp/account/gddx-tmp
wif_cfg=/etc/ppp/account/gddx-wificfg
PPPD_MANAGE_LOG_FILE=/tmp/log/pppd-manage.log
fail_again_go=0
let "get_signal=0x0"
let "SUCCESS_SIG=0x1"
let "FAILED_SIG=0x10"
let "QUIT_SIG=0x100"
let "CLEAN_PPPD_SIG=0x1000"
let "TERM_SIG=0x1000"

manage_log()
{
    echo "$@" >>$PPPD_MANAGE_LOG_FILE
}

set_account()
{
	username=$(head -1 $1)
	password=$(tail -1 $1)
	manage_log "set_account by: $1 username: $username, password: $password"
}

sync_account()
{
	mv $tmp_cfg $new_cfg
	manage_log "sync_account"
}

get_peer_account()
{
	/sbin/pppoe-obtain-account br-lan 192.168.199.106:8000/ $tmp_cfg $wif_cfg 2>/dev/null
	manage_log "get account done"
	if [ $ret -ne 0 ];then
		manage_log "get_peer_account err"	
	else
		uci set wireless.@wifi-iface[0].ssid=$(head -1 $wif_cfg)
		uci set wireless.@wifi-iface[0].key=$(tail -1 $wif_cfg)
		uci commit
		/sbin/wifi
	fi
	manage_log "get_peer_account $ret"
	return $ret
}

compare_account()
{
	manage_log "compare_account"
	if [ "$(head -1 $new_cfg)" = "$(head -1 $tmp_cfg)" ];then
		if [ "$(tail -1 $new_cfg)" = "$(tail -1 $tmp_cfg)" ];then
			return 0
		fi
	else
		return 1
	fi
}

stop_pppd()
{
	manage_log "start stop_pppd: $(pidof pppd)"
	while true;
	do
		killall pppd
		sleep 1
		if [ $(pidof pppd) ];then
			date
		else
			manage_log "break"
			break
		fi
	done
	manage_log "end stop_pppd: $(pidof pppd)"
}

call_pppd()
{
	manage_log "call_pppd with $1 username: $username, password: $password"

	stop_pppd

	if [ $(pidof pppd) ];then
		manage_log "pppd is running before i start pppd"
		exit
	fi

	[ $get_signal -ne 0 ] && manage_log "warn signal $get_signal not handle yet"
	let "get_signal&=&CLEAN_PPPD_SIG"
	
	/usr/sbin/pppd nodetach ipparam wan ifname pppoe-wan nodefaultroute \
	usepeerdns persist maxfail 1 user $username password $password ip-up-script \
	/lib/netifd/ppp-up ipv6-up-script /lib/netifd/ppp-up ip-down-script \
	/lib/netifd/ppp-down ipv6-down-script /lib/netifd/ppp-down mtu 1480 \
	mru 1480 plugin rp-pppoe.so nic-$(uci get network.wan.ifname) &

	while true;
	do
		sleep 1
		if [ $get_signal -ne 0 ];then
			break
		fi
	done

	manage_log "call_pppd got signal $get_signal"
	return 0
}

#/lib/netifd/ppp-up
trap "let \"get_signal|=$SUCCESS_SIG\"" USR1
#/etc/ppp/manage
trap "let \"get_signal|=$FAILED_SIG\"" USR2
#_proto_notify
trap "let \"get_signal|=$TERM_SIG\"" SIGTERM

manage_log "in pppd-manage.sh"

while true;
do
	if [ -s $new_cfg ]; then
		set_account $new_cfg
		call_pppd
		case "$get_signal" in
			"$SUCCESS_SIG")
				manage_log "all ok"
				let "get_signal-=$SUCCESS_SIG"
				while true;
				do
					if [ $get_signal -ne 0 ];then
						break;
					fi
					sleep 3
				done
			;;
			"$FAILED_SIG")
				let "get_signal-=$FAILED_SIG"
				if [ $fail_again_go -eq 1 ];then
					manage_log "fail_again_go exit"
					exit
				fi
				set_account $dft_cfg
				call_pppd
				case "$get_signal" in
					"$SUCCESS_SIG")
						let "get_signal-=$SUCCESS_SIG"
						get_peer_account
						if [ $? -ne 0 ];then
							manage_log "get_peer_account failed exit"
							return
						fi
						compare_account
						if [ $? -eq 0 ];then
							manage_log "same again exit"
							return
						fi
						sync_account
						fail_again_go=1
						continue
					;;
				esac
		esac
	else
		set_account $dft_cfg
		call_pppd
		case "$get_signal" in
			"$SUCCESS_SIG")
				let "get_signal-=$SUCCESS_SIG"
				get_peer_account
				if [ $? -ne 0 ];then
					manage_log "get_peer_account failed"
					return
				fi
				sync_account
			;;
			"$FAILED_SIG")
				manage_log "get FAILED_SIG exit"
				return
			;;
		esac
	fi

	if [ $get_signal -ne 0 ];then
		if [ $((get_signal & $TERM_SIG)) ];then
			manage_log "handle TERM_SIG"
		fi
		stop_pppd
		manage_log "exit $get_signal"
		return
	fi
done