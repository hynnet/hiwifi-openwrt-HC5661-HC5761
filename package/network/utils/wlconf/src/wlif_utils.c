/*
 * Wireless interface translation utility functions
 *
 * Copyright (C) 2012, Broadcom Corporation. All Rights Reserved.
 * 
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
 * SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION
 * OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN
 * CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 * $Id: wlif_utils.c 349051 2012-08-06 22:19:21Z $
 */

#include <typedefs.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <bcmparams.h>
#include <bcmnvram.h>
#include <bcmutils.h>
#include <netconf.h>
#include <nvparse.h>
#include <shutils.h>
#include <wlutils.h>
#include <wlif_utils.h>

#ifndef MAX_NVPARSE
#define MAX_NVPARSE 255
#endif

int
get_wlname_by_mac(unsigned char *mac, char *wlname)
{
	char eabuf[18];
	char tmptr[] = "wlXXXXX_hwaddr";
	char *wl_hw;
	int i, j;

	ether_etoa(mac, eabuf);
	/* find out the wl name from mac */
	for (i = 0; i < MAX_NVPARSE; i++) {
		sprintf(wlname, "wl%d", i);
		sprintf(tmptr, "wl%d_hwaddr", i);
		wl_hw = nvram_get(tmptr);
		if (wl_hw) {
			if (!strncasecmp(wl_hw, eabuf, sizeof(eabuf)))
				return 0;
		}

		for (j = 1; j < WL_MAXBSSCFG; j++) {
			sprintf(wlname, "wl%d.%d", i, j);
			sprintf(tmptr, "wl%d.%d_hwaddr", i, j);
			wl_hw = nvram_get(tmptr);
			if (wl_hw) {
				if (!strncasecmp(wl_hw, eabuf, sizeof(eabuf)))
					return 0;
			}
		}
	}

	return -1;
}

bool
wl_wlif_is_psta(char *ifname)
{
	int32 psta = FALSE;

	if (wl_probe(ifname) < 0)
		return FALSE;

	wl_iovar_getint(ifname, "psta_if", &psta);

	return psta ? TRUE : FALSE;
}

/*
 * Get LAN or WAN ifname by wl mac
 * NOTE: We pass ifname in case of same mac in vifs (like URE TR mode)
 */
char *
get_ifname_by_wlmac(unsigned char *mac, char *name)
{
	char nv_name[16], os_name[16], if_name[16];
	char *ifname = "br-lan";

	/*
	  * In case of URE mode, wl0.1 and wl0 have same mac,
	  * we need extra identity (name).
	  */
	if (name && !strncmp(name, "wl", 2))
		snprintf(nv_name, sizeof(nv_name), "%s", name);
	else if (get_wlname_by_mac(mac, nv_name))
		return 0;

	if (nvifname_to_osifname(nv_name, os_name, sizeof(os_name)) < 0)
		return 0;

	if (osifname_to_nvifname(os_name, nv_name, sizeof(nv_name)) < 0)
		return 0;

	if (strstr(name, "."))
	{
		return ifname;
	}
	else
	{
		return name;
	}
}

#define CHECK_NAS(mode) ((mode) & (WPA_AUTH_UNSPECIFIED | WPA_AUTH_PSK | \
				   WPA2_AUTH_UNSPECIFIED | WPA2_AUTH_PSK))
#define CHECK_PSK(mode) ((mode) & (WPA_AUTH_PSK | WPA2_AUTH_PSK))
#define CHECK_RADIUS(mode) ((mode) & (WPA_AUTH_UNSPECIFIED | WLIFU_AUTH_RADIUS | \
				      WPA2_AUTH_UNSPECIFIED))

/* Get wireless security setting by interface name */
int
get_wsec(wsec_info_t *info, unsigned char *mac, char *osifname)
{
	int i, unit, wds = 0, wds_wsec = 0, dwds = 0;
	char nv_name[16], os_name[16], wl_prefix[16], comb[32], key[8];
	char wds_role[8], wds_ssid[48], wds_psk[80], wds_akms[16], wds_crypto[16],
	        remote[ETHER_ADDR_LEN];
	char akm[16], *akms, *akmnext, *value, *infra;

	if (info == NULL || mac == NULL)
		return WLIFU_ERR_INVALID_PARAMETER;

	if (nvifname_to_osifname(osifname, os_name, sizeof(os_name))) {
		if (get_wlname_by_mac(mac, nv_name))
			return WLIFU_ERR_INVALID_PARAMETER;
		else if (nvifname_to_osifname(nv_name, os_name, sizeof(os_name)))
			return WLIFU_ERR_INVALID_PARAMETER;
	}
	else if (osifname_to_nvifname(os_name, nv_name, sizeof(nv_name)))
			return WLIFU_ERR_INVALID_PARAMETER;

	/* check if i/f exists and retrieve the i/f index */
	if (wl_probe(os_name) ||
		wl_ioctl(os_name, WLC_GET_INSTANCE, &unit, sizeof(unit)))
		return WLIFU_ERR_NOT_WL_INTERFACE;

	/* get wl_prefix */
	if (strstr(os_name, "wds")) {
		/* the wireless interface must be configured to run NAS */
		snprintf(wl_prefix, sizeof(wl_prefix), "wl%d", unit);
		wds = 1;
	}
	else if (wl_wlif_is_psta(os_name))
		snprintf(wl_prefix, sizeof(wl_prefix), "wl%d", unit);
	else if (osifname_to_nvifname(os_name, wl_prefix, sizeof(wl_prefix)))
		return WLIFU_ERR_INVALID_PARAMETER;

	strcat(wl_prefix, "_");
	memset(info, 0, sizeof(wsec_info_t));

	/* if dwds is enabled then dont configure the wds interface */
	dwds = atoi(nvram_safe_get(strcat_r(wl_prefix, "dwds", comb)));
	if (dwds)
		wds = 0;

	/* get wds setting */
	if (wds) {
		/* remote address */
		if (wl_ioctl(os_name, WLC_WDS_GET_REMOTE_HWADDR, remote, ETHER_ADDR_LEN))
			return WLIFU_ERR_WL_REMOTE_HWADDR;
		memcpy(info->remote, remote, ETHER_ADDR_LEN);

		/* get per wds settings */
		for (i = 0; i < MAX_NVPARSE; i ++) {
			char macaddr[18];
			uint8 ea[ETHER_ADDR_LEN];

			if (get_wds_wsec(unit, i, macaddr, wds_role, wds_crypto, wds_akms, wds_ssid,
			                 wds_psk) &&
			    ((ether_atoe(macaddr, ea) && !bcmp(ea, remote, ETHER_ADDR_LEN)) ||
			     ((mac[0] == '*') && (mac[1] == '\0')))) {
			     /* found wds settings */
			     wds_wsec = 1;
			     break;
			}
		}
	}

	/* interface unit */
	info->unit = unit;
	/* interface os name */
	strcpy(info->osifname, os_name);
	/* interface address */
	memcpy(info->ea, mac, ETHER_ADDR_LEN);
	/* ssid */
	if (wds && wds_wsec)
		strncpy(info->ssid, wds_ssid, MAX_SSID_LEN);
	else {
		value = nvram_safe_get(strcat_r(wl_prefix, "ssid", comb));
		strncpy(info->ssid, value, MAX_SSID_LEN);
	}
	/* auth */
	if (nvram_match(strcat_r(wl_prefix, "auth", comb), "1"))
		info->auth = 1;
	/* nas auth mode */
	value = nvram_safe_get(strcat_r(wl_prefix, "auth_mode", comb));
	info->akm = !strcmp(value, "radius") ? WLIFU_AUTH_RADIUS : 0;
	if (wds && wds_wsec)
		akms = wds_akms;
	else
		akms = nvram_safe_get(strcat_r(wl_prefix, "akm", comb));
	foreach(akm, akms, akmnext) {
		if (!strcmp(akm, "wpa"))
			info->akm |= WPA_AUTH_UNSPECIFIED;
		if (!strcmp(akm, "psk"))
			info->akm |= WPA_AUTH_PSK;
		if (!strcmp(akm, "wpa2"))
			info->akm |= WPA2_AUTH_UNSPECIFIED;
		if (!strcmp(akm, "psk2"))
			info->akm |= WPA2_AUTH_PSK;
	}
	/* wsec encryption */
	value = nvram_safe_get(strcat_r(wl_prefix, "wep", comb));
	info->wsec = !strcmp(value, "enabled") ? WEP_ENABLED : 0;
	if (wds && wds_wsec)
		value = wds_crypto;
	else
		value = nvram_safe_get(strcat_r(wl_prefix, "crypto", comb));
	if (CHECK_NAS(info->akm)) {
		if (!strcmp(value, "tkip"))
			info->wsec |= TKIP_ENABLED;
		else if (!strcmp(value, "aes"))
			info->wsec |= AES_ENABLED;
		else if (!strcmp(value, "tkip+aes"))
			info->wsec |= TKIP_ENABLED|AES_ENABLED;
	}
	/* nas role setting, may overwrite later in wds case */
	value = nvram_safe_get(strcat_r(wl_prefix, "mode", comb));
	infra = nvram_safe_get(strcat_r(wl_prefix, "infra", comb));
	if (!strcmp(value, "ap")) {
		info->flags |= WLIFU_WSEC_AUTH;
	}
	else if (!strcmp(value, "sta") || !strcmp(value, "wet") ||
	         !strcmp(value, "psr") || !strcmp(value, "psta")) {
		if (!strcmp(infra, "0")) {
			/* IBSS, so we must act as Authenticator and Supplicant */
			info->flags |= WLIFU_WSEC_AUTH;
			info->flags |= WLIFU_WSEC_SUPPL;
			/* Adhoc Mode */
			info->ibss = TRUE;
		}
		else {
			info->flags |= WLIFU_WSEC_SUPPL;
		}
	}
	else if (!strcmp(value, "wds")) {
		;
	}
	else {
		/* Unsupported network mode */
		return WLIFU_ERR_NOT_SUPPORT_MODE;
	}
	/* overwrite flags */
	if (wds) {
		char buf[32];
		unsigned char *ptr, lrole;

		/* did not find WDS link configuration, use wireless' */
		if (!wds_wsec)
			strcpy(wds_role, "auto");

		/* get right role */
		if (!strcmp(wds_role, "sup"))
			lrole = WL_WDS_WPA_ROLE_SUP;
		else if (!strcmp(wds_role, "auth"))
			lrole = WL_WDS_WPA_ROLE_AUTH;
		else /* if (!strcmp(wds_role, "auto")) */
			lrole = WL_WDS_WPA_ROLE_AUTO;

		strcpy(buf, "wds_wpa_role");
		ptr = (unsigned char *)buf + strlen(buf) + 1;
		bcopy(info->remote, ptr, ETHER_ADDR_LEN);
		ptr[ETHER_ADDR_LEN] = lrole;
		if (wl_ioctl(os_name, WLC_SET_VAR, buf, sizeof(buf)))
			return WLIFU_ERR_WL_WPA_ROLE;
		else if (wl_ioctl(os_name, WLC_GET_VAR, buf, sizeof(buf)))
			return WLIFU_ERR_WL_WPA_ROLE;
		lrole = *buf;

		/* overwrite these flags */
		info->flags = WLIFU_WSEC_WDS;
		if (lrole == WL_WDS_WPA_ROLE_SUP) {
			info->flags |= WLIFU_WSEC_SUPPL;
		}
		else if (lrole == WL_WDS_WPA_ROLE_AUTH) {
			info->flags |= WLIFU_WSEC_AUTH;
		}
		else {
			/* unable to determine WPA role */
			return WLIFU_ERR_WL_WPA_ROLE;
		}
	}
	/* user-supplied psk passphrase */
	if (CHECK_PSK(info->akm)) {
		if (wds && wds_wsec) {
			strncpy((char *)info->psk, wds_psk, MAX_USER_KEY_LEN);
			info->psk[MAX_USER_KEY_LEN] = 0;
		}
		else {
			value = nvram_safe_get(strcat_r(wl_prefix, "wpa_psk", comb));
			strncpy((char *)info->psk, value, MAX_USER_KEY_LEN);
			info->psk[MAX_USER_KEY_LEN] = 0;
		}
	}
	/* user-supplied radius server secret */
	if (CHECK_RADIUS(info->akm))
		info->secret = nvram_safe_get(strcat_r(wl_prefix, "radius_key", comb));
	/* AP specific settings */
	value = nvram_safe_get(strcat_r(wl_prefix, "mode", comb));
	if (!strcmp(value, "ap")) {
		/* gtk rekey interval */
		if (CHECK_NAS(info->akm)) {
			value = nvram_safe_get(strcat_r(wl_prefix, "wpa_gtk_rekey", comb));
			info->gtk_rekey_secs = (int)strtoul(value, NULL, 0);
		}
		/* wep key */
		if (info->wsec & WEP_ENABLED) {
			/* key index */
			value = nvram_safe_get(strcat_r(wl_prefix, "key", comb));
			info->wep_index = (int)strtoul(value, NULL, 0);
			/* key */
			sprintf(key, "key%s", nvram_safe_get(strcat_r(wl_prefix, "key", comb)));
			info->wep_key = nvram_safe_get(strcat_r(wl_prefix, key, comb));
		}
		/* radius server host/port */
		if (CHECK_RADIUS(info->akm)) {
			/* update radius server address */
			info->radius_addr = nvram_safe_get(strcat_r(wl_prefix, "radius_ipaddr",
			                                            comb));
			value = nvram_safe_get(strcat_r(wl_prefix, "radius_port", comb));
			info->radius_port = htons((int)strtoul(value, NULL, 0));
			/* 802.1x session timeout/pmk cache duration */
			value = nvram_safe_get(strcat_r(wl_prefix, "net_reauth", comb));
			info->ssn_to = (int)strtoul(value, NULL, 0);
		}
	}
	/* preauth */
	value = nvram_safe_get(strcat_r(wl_prefix, "preauth", comb));
	info->preauth = (int)strtoul(value, NULL, 0);

	/* verbose */
	value = nvram_safe_get(strcat_r(wl_prefix, "nas_dbg", comb));
	info->debug = (int)strtoul(value, NULL, 0);



	return WLIFU_WSEC_SUCCESS;
}
