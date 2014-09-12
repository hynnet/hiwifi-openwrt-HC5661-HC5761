#!/bin/sh

. /usr/share/libubox/jshn.sh

VPNIF=vpn
WANIF=wan

install() {
    # add vpn if to wan zone
    firewall_add $VPNIF
    return 0
}

uninstall() {
    stop
    rm -f /tmp/dnsmasq.d/pptp && /etc/init.d/dnsmasq restart
    
    uci delete network.$VPNIF
    uci commit network.$VPNIF
    
    firewall_del $VPNIF
    return 0
}

status() {
    json_load "$(ubus call network.interface.$VPNIF status -S)" 2>/dev/null
    json_get_var if_up up
    json_get_var if_pend pending

    if [ ${if_up:-0} -eq 1 ] ; then
        stat=running
    elif [ ${if_pend:-0} -eq 0 ] ; then
        stat=stopped
    else
        stat=dialing
    fi

    echo -n $stat
    return 0
}

start() {
    /etc/init.d/xl2tpd start
    ifup $VPNIF
    return 0
}

stop() { 
    json_load "$(ubus call network.interface.$VPNIF status -S)" 2>/dev/null
    json_get_var if_up up
    json_get_var if_pend pending

    if [ ${if_up:-0} -eq 0 ] && [ ${if_pend:-0} -eq 0 ] ; then
        return 0;
    fi

    ifdown $VPNIF
    sleep 3

    route|grep -qsw default
    if [ $? -ne 0 ]; then
        ifup $WANIF
    fi

    /etc/init.d/xl2tpd stop
    return 0
}


firewall_add () {
    idx=$(uci show firewall|grep -w @zone|grep -E "name=${WANIF}$"|cut -d= -f1|grep -oE "[0-9]+")
    oldifs=$(uci get firewall.@zone[$idx].network)

    echo "$oldifs"|grep -qsw "$1"
    if [ $? -ne 0 ]; then
        newifs="$oldifs $1"
        uci set firewall.@zone[$idx].network="$newifs"
        uci commit firewall
    fi
}

firewall_del () {
    if [ "X$1" = "X$WANIF" ];then
        return
    fi

    idx=$(uci show firewall|grep -w @zone|grep -E "name=${WANIF}$"|cut -d= -f1|grep -oE "[0-9]+")
    oldifs=$(uci get firewall.@zone[$idx].network)
    
    echo "$oldifs"|grep -qsw "$1"
    if [ $? -eq 0 ]; then
        newifs=$(echo "$oldifs"|sed "s/[[:space:]]\+$1//g")
        uci set firewall.@zone[$idx].network="$newifs"
        uci commit firewall
    fi
}

$1
