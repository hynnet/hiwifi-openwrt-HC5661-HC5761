/******************************************************************************
 *
 * Copyright (c) 2013 Beijing Geek-geek Technology Co., Ltd.
 * All rights reserved.
 *
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Geek-geek Corporation;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Geek-geek Corporation.
 *
 * FILE NAME  :   iw_lua.c
 * VERSION    :   1.0
 * DESCRIPTION:   Wireless Information Library - Lua Bindings.
 * AUTHOR     :	  Abu Cheng
 *
 ******************************************************************************/
#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>
#include <string.h>
#include <errno.h>
#ifndef TARGETENV_android
#include <error.h>
#endif /* TARGETENV_android */
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <proto/ethernet.h>
#include <proto/bcmip.h>

#ifndef TARGETENV_android
typedef u_int64_t u64;
typedef u_int32_t u32;
typedef u_int16_t u16;
typedef u_int8_t u8;
typedef u_int64_t __u64;
typedef u_int32_t __u32;
typedef u_int16_t __u16;
typedef u_int8_t __u8;
#endif /* TARGETENV_android */

#include <linux/sockios.h>
#include <linux/ethtool.h>
#include <signal.h>
#include <typedefs.h>
#include <wlioctl.h>
#include <bcmutils.h>
#include <sys/wait.h>
#include <netdb.h>
#include <netinet/in.h>
#include <bcmendian.h>
#include <bcmwifi_channels.h>
#include <bcmsrom_fmt.h>
#include <bcmsrom_tbl.h>
#include "wlu_common.h"
#include "wlu.h"
#include <bcmcdc.h>
#include "wlu_remote.h"
#include "wlu_client_shared.h"
#include "wlu_pipe.h"
#include <miniopt.h>

#define WL_DUMP_BUF_LEN (127 * 1024)

typedef struct lua_cmd lua_cmd_t;
typedef int (lua_func_t)(lua_State *L, void *wl, lua_cmd_t *cmd, char *argv);

int wlu_iovar_getint(void *wl, const char *iovar, int *pval);
int wlu_var_getbuf(void *wl, const char *iovar, void *param, int param_len, void **bufptr);

struct lua_cmd
{
	const char *sname;
	const char *name;
	lua_func_t *func;
};

/* dword align allocation */
static union {
	char bufdata[WL_DUMP_BUF_LEN];
	uint32 alignme;
} bufstruct_wlu;
static char *buf = (char*) &bufstruct_wlu.bufdata;

static int cmd_split(char *cmd, char *argv[], int num)
{
	char *p;
	int i;

	for(i=0; i<num; i++)
		argv[i] = NULL;

	p = cmd;
	i = 0;
	while(i<num)
	{
		/* find the first NOT SPACE CHAR */
		while(*p==' '&&*p!=0)
		{
			p++;
		}
		if(*p == 0)
			break;
		
		argv[i++] = p;		

		while(*p!=' '&&*p!=0)
		{
			p++;
		}
		if(*p==0)
			break;
		
		*p = 0;
		p++;
	}
	return i;
}

static int
wl_if_wpa_rsn_ies(uint8* cp, uint len)
{
	uint8 *parse = cp;
	uint parse_len = len;
	uint8 *wpaie;
	uint8 *rsnie;

	while ((wpaie = wlu_parse_tlvs(parse, parse_len, DOT11_MNG_WPA_ID)))
		if (wlu_is_wpa_ie(&wpaie, &parse, &parse_len))
			break;
	if (wpaie)
		return 1;

	rsnie = wlu_parse_tlvs(cp, len, DOT11_MNG_RSN_ID);
	if (rsnie)
		return 1;

	return 0;
}

static int
lua_ctl_scan(lua_State *L, void *wl, lua_cmd_t *cmd, char *argv)
{
	int params_size = WL_SCAN_PARAMS_FIXED_SIZE + WL_NUMCHANNELS * sizeof(uint16);
	wl_scan_params_t *params;
	int err = 0;

	params_size += WL_SCAN_PARAMS_SSID_MAX * sizeof(wlc_ssid_t);
	params = (wl_scan_params_t*)malloc(params_size);
	if (params == NULL) {
		fprintf(stderr, "Error allocating %d bytes for scan params\n", params_size);
		return -1;
	}
	memset(params, 0, params_size);
	memcpy(&params->bssid, &ether_bcast, ETHER_ADDR_LEN);
	params->bss_type = DOT11_BSSTYPE_ANY;
	params->scan_type = 0;
	params->nprobes = -1;
	params->active_time = -1;
	params->passive_time = -1;
	params->home_time = -1;
	params->channel_num = 0;

	params_size = (char*)params->channel_list - (char*)params;

	err = wlu_set(wl, WLC_SCAN, params, params_size);

	free(params);
	return err;
}

static int
lua_ctl_aclchk(lua_State *L, void *wl, lua_cmd_t *cmd, char *argv)
{
	struct maclist *maclist = (struct maclist *) buf;
	struct maclist *acllist = (struct maclist *) (buf + WLC_IOCTL_MAXLEN);
	uint i, j, max = (WLC_IOCTL_MAXLEN - sizeof(int)) / ETHER_ADDR_LEN;
	struct ether_addr *ea, *tea;
	int ret, val, deauth;

	if ((ret = wlu_get(wl, WLC_GET_MACMODE, &val, sizeof(int))) < 0)
		return ret;

	val = dtoh32(val);

	if (val == 0)
	{
		return 0;
	}

	maclist->count = htod32(max);
	if ((ret = wlu_get(wl, WLC_GET_ASSOCLIST, maclist, WLC_IOCTL_MAXLEN)) < 0)
	{
		return ret;
	}
	maclist->count = dtoh32(maclist->count);

	acllist->count = htod32(max);
	if ((ret = wlu_get(wl, WLC_GET_MACLIST, acllist, WLC_IOCTL_MAXLEN)) < 0)
	{
		return ret;
	}
	acllist->count = dtoh32(maclist->count);

	for (i = 0, ea = maclist->ea; i < maclist->count && i < max; i++, ea++)
	{
		deauth = (val == 2) ? 1 : 0;
		for (j = 0, tea = acllist->ea; j < acllist->count && j < max; j++, tea++)
		{
			if (memcmp(ea, tea, sizeof(struct ether_addr)) == 0)
			{
				deauth = !deauth;
			}
		}
		if (deauth)
		{
			/* No reason code furnished, so driver will use its default */
			ret = wlu_set(wl, WLC_SCB_DEAUTHENTICATE, ea, ETHER_ADDR_LEN);
		}
	}
	return 0;
}

static int
lua_get_ch(lua_State *L, void *wl, lua_cmd_t *cmd, char *argv)
{
	int err;
	uint32 val = 0;
	chanspec_t chanspec = 0;

	if ((err = wlu_iovar_getint(wl, cmd->name, (int*)&val)) < 0)
	{
		return err;
	}

	chanspec = wl_chspec32_from_driver(val);
	val = wf_chspec_ctlchan(chanspec);
	lua_pushnumber(L, val);
	return 0;
}

static int
lua_get_status(lua_State *L, void *wl, lua_cmd_t *cmd, char *argv)
{
	int ret;
	struct ether_addr bssid;

	if ((ret = wlu_get(wl, WLC_GET_BSSID, &bssid, ETHER_ADDR_LEN)) == 0)
	{
		/* The adapter is associated. */
		lua_pushnumber(L, 1);
	}
	else
	{
		lua_pushnumber(L, 0);
	}

	return 0;
}

static int
lua_get_acl(lua_State *L, void *wl, lua_cmd_t *cmd, char *argv)
{
	int ret;
	int val;
	char *str_policy[] = {
		"open",
		"deny",
		"allow"
	};

	if ((ret = wlu_get(wl, WLC_GET_MACMODE, &val, sizeof(int))) < 0)
		return ret;

	val = dtoh32(val);

	lua_pushstring(L, str_policy[val]);	

	return ret;
}

static int
lua_get_scan(lua_State *L, void *wl, lua_cmd_t *cmd, char *argv)
{
	int ret;
	wl_scan_results_t *list = (wl_scan_results_t*)buf;
	wl_bss_info_t *bi;
	char ssid[33];
	uint i;

	ret = wl_get_scan(wl, WLC_SCAN_RESULTS, buf, WL_DUMP_BUF_LEN);

	if (ret == 0)
	{
		lua_newtable(L);
		bi = list->bss_info;
		for (i = 0; i < list->count; i++, bi = (wl_bss_info_t*)((int8*)bi + dtoh32(bi->length)))
		{
			lua_newtable(L);

			lua_pushstring(L, wl_ether_etoa(&bi->BSSID));
			lua_setfield(L, -2, "bssid");

			memset(ssid, 0, sizeof(ssid));
			memcpy(ssid, bi->SSID, bi->SSID_len);
			lua_pushstring(L, ssid);
			lua_setfield(L, -2, "ssid");

			lua_pushnumber(L, (int16)(dtoh16(bi->RSSI)));
			lua_setfield(L, -2, "rssi");

			lua_pushnumber(L, bi->ctl_ch);
			lua_setfield(L, -2, "channel");

			lua_pushstring(L, bi->n_cap ? "11n" : "11g");
			lua_setfield(L, -2, "bgn_mode");

			if (bi->capability & DOT11_CAP_PRIVACY)
			{
				if (dtoh32(bi->ie_length) && 
					wl_if_wpa_rsn_ies((uint8 *)(((uint8 *)bi) + dtoh16(bi->ie_offset)), dtoh32(bi->ie_length)))
				{
					lua_pushstring(L, "psk");
					lua_setfield(L, -2, "auth");
				}
				else
				{
					lua_pushstring(L, "wep");
					lua_setfield(L, -2, "auth");
				}
			}
			else
			{
				lua_pushstring(L, "none");
				lua_setfield(L, -2, "auth");
			}

			lua_pushstring(L, (dtoh32(bi->nbss_cap) & HT_CAP_40MHZ) ? "40MHz" : "20MHz");
			lua_setfield(L, -2, "cwm");

			lua_rawseti(L, -2, i+1);
		}
	}

	return ret;
}

static int
lua_get_stalist(lua_State *L, void *wl, lua_cmd_t *cmd, char *argv)
{
	sta_info_t *sta;
	struct maclist *maclist = (struct maclist *) buf;
	uint i, max = (WLC_IOCTL_MAXLEN - sizeof(int)) / ETHER_ADDR_LEN;
	struct ether_addr *ea;
	int buflen, ret;
	char *param;

	maclist->count = htod32(max);
	if ((ret = wlu_get(wl, WLC_GET_ASSOCLIST, maclist, WLC_IOCTL_MAXLEN)) < 0)
	{
		return ret;
	}
	maclist->count = dtoh32(maclist->count);
	lua_newtable(L);
	for (i = 0, ea = maclist->ea; i < maclist->count && i < max; i++, ea++)
	{
		strcpy(buf, "sta_info");
		buflen = strlen(buf) + 1;
		param = (char *)(buf + buflen);
		memcpy(param, (char*)&ea, ETHER_ADDR_LEN);

		if ((ret = wlu_get(wl, WLC_GET_VAR, buf, WLC_IOCTL_MEDLEN)) < 0)
			return ret;

		/* display the sta info */
		sta = (sta_info_t *)buf;
		sta->flags = dtoh32(sta->flags);

		lua_newtable(L);
		lua_pushstring(L, wl_ether_etoa(ea));
		lua_setfield(L, -2, "macaddr");

		lua_pushstring(L, (sta->flags & WL_STA_N_CAP) ? " 11n" : "11g");
		lua_setfield(L, -2, "mode");

		lua_pushboolean(L, ((sta->flags & WL_STA_PS) != 0));
		lua_setfield(L, -2, "pwr");

		lua_pushnumber(L, sta->RSSI);
		lua_setfield(L, -2, "ccq");

		lua_pushnumber(L, sta->tx_rate);
		lua_setfield(L, -2, "rate");

		lua_pushstring(L, (sta->flags & WL_STA_AUTHO) ? "psk" : "none");
		lua_setfield(L, -2, "auth");

		lua_pushstring(L, (sta->flags & WL_STA_AUTHO) ? "aes" : "none");
		lua_setfield(L, -2, "cipher");

		lua_pushnumber(L, dtoh32(sta->tx_pkts));
		lua_setfield(L, -2, "tx_packets");

		lua_pushnumber(L, dtoh32(sta->rx_ucast_pkts));
		lua_setfield(L, -2, "rx_packets");

		lua_pushnumber(L, sta->idle);
		lua_setfield(L, -2, "inact");

		lua_pushnumber(L, sta->in);
		lua_setfield(L, -2, "assoc_time");

		lua_rawseti(L, -2, i+1);
	}

	return ret;
}

static int
lua_get_acllist(lua_State *L, void *wl, lua_cmd_t *cmd, char *argv)
{
	struct maclist *maclist = (struct maclist *) buf;
	uint i, max = (WLC_IOCTL_MAXLEN - sizeof(int)) / ETHER_ADDR_LEN;
	struct ether_addr *ea;
	int ret;

	maclist->count = htod32(max);
	if ((ret = wlu_get(wl, WLC_GET_MACLIST, maclist, WLC_IOCTL_MAXLEN)) < 0)
	{
		return ret;
	}
	maclist->count = dtoh32(maclist->count);
	lua_newtable(L);
	for (i = 0, ea = maclist->ea; i < maclist->count && i < max; i++, ea++)
	{
		lua_newtable(L);

		lua_pushstring(L, wl_ether_etoa(ea));
		lua_setfield(L, -2, "macaddr");

		lua_rawseti(L, -2, i+1);
	}

	return ret;
}

static int
lua_set_led(lua_State *L, void *wl, lua_cmd_t *cmd, char *argv)
{
	int err;
	wl_led_info_t led;
	void	*ptr = NULL;

	memset(&led, 0, sizeof(wl_led_info_t));

	/* Read the original back so we don't toggle the activehi */
	if ((err = wlu_var_getbuf(wl, cmd->name, (void*)&led,
		sizeof(wl_led_info_t), &ptr)) < 0) {
		return err;
	}
	led.behavior = (uint32)strtoul(argv, NULL, 10);
	led.activehi = ((wl_led_info_t*)ptr)->activehi;

	if ((err = wlu_var_setbuf(wl, cmd->name, (void*)&led,
		sizeof(wl_led_info_t))) < 0) {
		return err;
	}
	return err;
}

static int
lua_set_acl(lua_State *L, void *wl, lua_cmd_t *cmd, char *argv)
{
	int ret;
	int val;

	if (!stricmp(argv, "deny"))
	{
		val = 1;
	}
	else if (!stricmp(argv, "allow"))
	{
		val = 2;
	}
	else
	{
		val = 0;
	}

	val = htod32(val);
	ret = wlu_set(wl, WLC_SET_MACMODE, &val, sizeof(int));

	return ret;
}

static int
lua_set_acllist(lua_State *L, void *wl, lua_cmd_t *cmd, char *buf)
{
	struct maclist *maclist = (struct maclist *) buf;
	struct ether_addr *ea;
	uint max = (WLC_IOCTL_MAXLEN - sizeof(int)) / ETHER_ADDR_LEN;
	char *nargv[128];
	char **argv;
	uint len;

	cmd_split(buf, (char **)nargv, 128);
	argv = (char **)nargv;

	/* Clear list */
	maclist->count = htod32(0);
	ea = maclist->ea;
	while (*argv && maclist->count < max)
	{
		if (!wl_ether_atoe(*argv, ea))
		{
			return -1;
		}
		maclist->count++;
		ea++;
		argv++;
	}

	/* Set new list */
	len = sizeof(maclist->count) + maclist->count * sizeof(maclist->ea);
	maclist->count = htod32(maclist->count);
	return wlu_set(wl, WLC_SET_MACLIST, maclist, len);
}

struct lua_cmd l_ctl_cmds[] = 
{
	{ "scan", "scan", lua_ctl_scan },
	{ "aclchk", "aclchk", lua_ctl_aclchk },
	{ NULL, NULL, NULL }
};

struct lua_cmd l_get_cmds[] = 
{
	{ "ch", "chanspec", lua_get_ch },
	{ "status", "status", lua_get_status },
	{ "acl", "macmode", lua_get_acl },
	{ "aplist", "scanresults", lua_get_scan },
	{ "stalist", "sta_info", lua_get_stalist },
	{ "acllist", "mac", lua_get_acllist },
	{ NULL, NULL, NULL }
};

struct lua_cmd l_set_cmds[] = 
{
	{ "led", "led", lua_set_led },
	{ "acl", "macmode", lua_set_acl },
	{ "acllist", "mac", lua_set_acllist },
	{ NULL, NULL, NULL }
};

static int iw_L_ctl(lua_State *L)
{
	const char *ifname = luaL_checkstring(L, 1);
	const char *command = luaL_checkstring(L, 2);
	struct lua_cmd *cmd = NULL;
	struct ifreq ifr;

	memset(&ifr, 0, sizeof(ifr));

	if (strcmp(ifname, "wlan0") == 0)
	{
		strncpy(ifr.ifr_name, "wl0.1", IFNAMSIZ);
	}
	else if (strcmp(ifname, "wlan1") == 0)
	{
		strncpy(ifr.ifr_name, "wl0", IFNAMSIZ);
	}
	else
	{
		strncpy(ifr.ifr_name, ifname, IFNAMSIZ);
	}

	/* search for command */
	for (cmd = l_ctl_cmds; cmd->name && strcmp(cmd->sname, command); cmd++);

	if (NULL != cmd->name)
	{
		(*cmd->func)(L, (void *) &ifr, cmd, NULL);
	}

	lua_pushnil(L);	

	return 1;
}

static int iw_L_get(lua_State *L)
{
	const char *ifname = luaL_checkstring(L, 1);
	const char *command = luaL_checkstring(L, 2);
	struct lua_cmd *cmd = NULL;
	struct ifreq ifr;
	int ret = -1;

	memset(&ifr, 0, sizeof(ifr));

	if (strcmp(ifname, "wlan0") == 0)
	{
		strncpy(ifr.ifr_name, "wl0.1", IFNAMSIZ);
	}
	else if (strcmp(ifname, "wlan1") == 0)
	{
		strncpy(ifr.ifr_name, "wl0", IFNAMSIZ);
	}
	else
	{
		strncpy(ifr.ifr_name, ifname, IFNAMSIZ);
	}

	/* search for command */
	for (cmd = l_get_cmds; cmd->name && strcmp(cmd->sname, command); cmd++);

	if (NULL != cmd->name)
	{
		ret = (*cmd->func)(L, (void *) &ifr, cmd, NULL);
	}

	if (0 != ret)
	{
		lua_pushnil(L);	
	}

	return 1;
}

static int iw_L_set(lua_State *L)
{
	const char *ifname = luaL_checkstring(L, 1);
	const char *command = luaL_checkstring(L, 2);
	const char *argv = luaL_checkstring(L, 3);
	struct lua_cmd *cmd = NULL;
	struct ifreq ifr;

	memset(&ifr, 0, sizeof(ifr));

	if (strcmp(ifname, "wlan0") == 0)
	{
		strncpy(ifr.ifr_name, "wl0.1", IFNAMSIZ);
	}
	else if (strcmp(ifname, "wlan1") == 0)
	{
		strncpy(ifr.ifr_name, "wl0", IFNAMSIZ);
	}
	else
	{
		strncpy(ifr.ifr_name, ifname, IFNAMSIZ);
	}

	/* search for command */
	for (cmd = l_set_cmds; cmd->name && strcmp(cmd->sname, command); cmd++);

	if (NULL != cmd->name)
	{
		(*cmd->func)(L, (void *) &ifr, cmd, (char *)argv);
	}

	lua_pushnil(L);	

	return 1;
}

static const luaL_reg R_hcwifi[] = {
	{ "ctl", iw_L_ctl },
	{ "get", iw_L_get },
	{ "set", iw_L_set },
	{ NULL, NULL }
};

LUALIB_API int luaopen_hcwifi(lua_State *L) {
	luaL_register(L, "hcwifi", R_hcwifi);

	return 1;
}
