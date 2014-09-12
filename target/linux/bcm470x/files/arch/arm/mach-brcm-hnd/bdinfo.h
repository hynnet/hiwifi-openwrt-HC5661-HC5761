/*
 *  Turbo Wireless boards board-info parse support
 *
 *  Copyright (C) 2012-2012 eric
 *
 *  This program is free software; you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License version 2 as published
 *  by the Free Software Foundation.
 */

#ifndef __BDINFO_H__
#define __BDINFO_H__

#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/string.h>
#include <linux/init.h>
 
#define	BDINFO_MAC_STR_LEN	(17+1)
#define BDINFO_PIN_STR_LEN	(8+1)
#define BDINFO_UUID_STR_LEN	(36+1)
#define BDINFO_SN_STR_LEN	(16+1)

#define BDINFO_LINE_BUF_LEN			(256+1)
#define BDINFO_ENTRY_NAME_LEN		(64+1)
#define BDINFO_ENTRY_VAL_STR_LEN	(128+1)

typedef struct bdinfo_entry
{
	u8		name[BDINFO_ENTRY_NAME_LEN];
	u32*	valp;
	u32		val_len;
	int 	(*fval_set)(			/* pointer of the function that set value for entry */
				u32*	dst,		/* address that will be set value */ 
				u32		len,		/* value len */
				const u8 *val_str);	/* value with string type read from bdinfo mtd */  
}bdinfo_entry_t;

int parse_bdinfo(const u8* bdinfo, const u32 max_len);
int macstr_to_hex(u8 *hex, const u8 *str);
u8* bdinfo_get_fac_mac(void);

#endif	/* __BDINFO_H__ */

