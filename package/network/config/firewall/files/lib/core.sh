# Copyright (C) 2009-2010 OpenWrt.org

FW_LIBDIR=${FW_LIBDIR:-/lib/firewall}

. $FW_LIBDIR/fw.sh
include /lib/network

fw_hwf_log() {
    echo "[$$] [$1] : $(date)" >>/tmp/firewall.log
}

fw_start() {
	fw_init

	FW_DEFAULTS_APPLIED=

	fw_is_loaded && {
		fw_hwf_log "firewall already loaded"
		exit 1
	}

	uci_set_state firewall core "" firewall_state

	fw_clear DROP

	fw_callback pre core

	fw_hwf_log "Loading defaults"
	fw_config_once fw_load_defaults defaults

	fw_hwf_log "Loading zones"
	config_foreach fw_load_zone zone

	fw_hwf_log "Loading forwardings"
	config_foreach fw_load_forwarding forwarding

	fw_hwf_log "Loading rules"
	config_foreach fw_load_rule rule

	fw_hwf_log "Loading redirects"
	config_foreach fw_load_redirect redirect

	[ -z "$FW_NOTRACK_DISABLED" ] && {
		fw_hwf_log "Optimizing conntrack"
		config_foreach fw_load_notrack_zone zone
	}

	fw_hwf_log "Loading interfaces"
	config_foreach fw_configure_interface interface add

	fw_hwf_log "Loading includes"
	config_foreach fw_load_include include

	fw_callback post core

	uci_set_state firewall core zones "$FW_ZONES"
	uci_set_state firewall core loaded 1
}

fw_stop() {
	fw_init

	fw_callback pre stop

	local z n i
	config_get z core zones
	for z in $z; do
		config_get n core "${z}_networks"
		for n in $n; do
			config_get i core "${n}_ifname"
			[ -n "$i" ] && env -i ACTION=remove ZONE="$z" \
				INTERFACE="$n" DEVICE="$i" /sbin/hotplug-call firewall
		done

		config_get i core "${z}_tcpmss"
		[ "$i" == 1 ] && {
			fw del i m FORWARD zone_${z}_MSSFIX
			fw del i m zone_${z}_MSSFIX
		}
	done

	fw_clear ACCEPT

	fw_callback post stop

	uci_revert_state firewall
	config_clear

	local h
	for h in $FW_HOOKS; do unset $h; done

	unset FW_HOOKS
	unset FW_INITIALIZED
}

fw_restart() {
	fw_stop
	fw_start
}

fw_reload() {
	fw_restart
}

fw_is_loaded() {
	local bool=$(uci_get_state firewall.core.loaded)
	return $((! ${bool:-0}))
}


fw_die() {
	echo "Error:" "$@" >&2
	fw_log error "$@"
	fw_stop
	exit 1
}

fw_log() {
	local level="$1"
	[ -n "$2" ] && shift || level=notice
	[ "$level" != error ] || echo "Error: $@" >&2
	logger -t firewall -p user.$level "$@"
}


fw_init() {
	[ -z "$FW_INITIALIZED" ] || return 0

	. $FW_LIBDIR/config.sh

	scan_interfaces
	fw_config_append firewall

	local hooks="core stop defaults zone notrack synflood"
	local file lib hk pp
	for file in $FW_LIBDIR/core_*.sh; do
		. $file
		hk=$(basename $file .sh)
		hk=${hk#core_}
		append hooks $hk
	done
	for file in $FW_LIBDIR/*.sh; do
		lib=$(basename $file .sh)
		lib=${lib##[0-9][0-9]_}
		case $lib in
			core*|fw|config|uci_firewall) continue ;;
		esac
		. $file
		for hk in $hooks; do
			for pp in pre post; do
				type ${lib}_${pp}_${hk}_cb >/dev/null && {
					append FW_CB_${pp}_${hk} ${lib}
					append FW_HOOKS FW_CB_${pp}_${hk}
				}
			done
		done
	done

	fw_callback post init

	FW_INITIALIZED=1
	return 0
}
