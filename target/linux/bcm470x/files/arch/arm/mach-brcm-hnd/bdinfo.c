/*
 *  Turbo Wireless boards board-info parse support
 *
 *  Copyright (C) 2012-2012 eric
 *
 *  This program is free software; you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License version 2 as published
 *  by the Free Software Foundation.
 */

#include <linux/export.h>
#include "bdinfo.h"

#define	BDINFO_END_MAGIC	"BDINFO_END"

static int set_bdinfo_str(u32* dst, u32 len, const u8* val_str)
{
	if (!val_str || strlen(val_str) > len - 1)
	{
		printk("val_str is null or too long\n");
		return -1;
	}
	else
	{
		strcpy((u8 *)dst, val_str);
		return 0;
	}
}

static u8 fac_mac[BDINFO_MAC_STR_LEN] = "00:BA:BE:00:00:00";	/* default mac, a magic */

bdinfo_entry_t bdinfo_table[] = 
{
	{
		"fac_mac",
		(u32 *)&fac_mac,
		sizeof(fac_mac),
		set_bdinfo_str
	}
};

int parse_bdinfo(const u8* bdinfo, const u32 max_len)
{
	u8 entry_name[BDINFO_ENTRY_NAME_LEN];
	u8 entry_val[BDINFO_ENTRY_VAL_STR_LEN];

	const u8* p = bdinfo;
	u32 assigns = 0;

	bdinfo_entry_t* entryp;

	if ((*((u32 *)p) == 0xffffffff) 
		|| (*((u32 *)p) == 0x0))
	{
		printk(KERN_NOTICE "bdinfo: fac txt empty, use default.");
		return 0;
	}

	while ((memcmp(p, BDINFO_END_MAGIC, sizeof(BDINFO_END_MAGIC) - 1)) 
			&& (p < bdinfo + max_len))
	{
		memset(entry_name, 0, sizeof(entry_name));
		memset(entry_val, 0, sizeof(entry_val));

		/*
		 * avoid array cross over, if change BDINFO_ENTRY_NAME_LEN and BDINFO_ENTRY_VAL_STR_LEN
		 * please change the magic num
		 */
		assigns = sscanf(p, "%64s = %128s", entry_name, entry_val);

		if (assigns == 2)
		{
			for (entryp = bdinfo_table; 
				entryp < bdinfo_table + sizeof(bdinfo_table)/sizeof(bdinfo_entry_t); 
				entryp++)
			{
				if (strcmp(entryp->name, entry_name) == 0)
				{
					entryp->fval_set(entryp->valp, entryp->val_len, entry_val);
				}
			}
        }
		else
		{
			printk(KERN_NOTICE "bdinfo: fac error format, use default.");
			return 0;
		}

		p += (strlen(entry_name) + strlen(entry_val) + (sizeof(" = \n") - 1));
	}

	return 0;
}

int macstr_to_hex(u8 *hex, const u8 *str)
{
	int			i = 0;
	u8			c;
	const u8*	p;

	if (NULL == hex || NULL == str)
	{
		goto err;
	}

	p = str;

	for (i = 0; i < 6; i++)
	{
		if (*p == '-' || *p == ':')
		{
			p++;
		}

		if (*p >= '0' && *p <= '9')
		{
			c  = *p++ - '0';
		}
		else if (*p >= 'a' && *p <= 'f')
		{
			c  = *p++ - 'a' + 0xa;
		}
		else if (*p >= 'A' && *p <= 'F')
		{
			c  = *p++ - 'A' + 0xa;
		}
		else
		{
			goto err;
		}

		c <<= 4;	/* high 4bits of a byte */

		if (*p >= '0' && *p <= '9')
		{
			c |= *p++ - '0';
		}
		else if (*p >= 'a' && *p <= 'f')
		{
			c |= (*p++ - 'a' + 0xa);
		}
		else if (*p >= 'A' && *p <= 'F')
		{
			c |= (*p++ - 'A' + 0xa);
		}
		else
		{
			goto err;
		}

		hex[i] = c;
	}

	return 0;

err:
	return -1;
}

u8* bdinfo_get_fac_mac(void)
{
	return fac_mac;
}

EXPORT_SYMBOL(bdinfo_get_fac_mac);
