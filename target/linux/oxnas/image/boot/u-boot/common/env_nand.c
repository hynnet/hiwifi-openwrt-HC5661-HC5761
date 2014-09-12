/*
 * (C) Copyright 2004
 * Jian Zhang, Texas Instruments, jzhang@ti.com.

 * (C) Copyright 2000-2004
 * Wolfgang Denk, DENX Software Engineering, wd@denx.de.
 *
 * (C) Copyright 2001 Sysgo Real-Time Solutions, GmbH <www.elinos.com>
 * Andreas Heppel <aheppel@sysgo.de>

 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

/* #define DEBUG */

#include <common.h>

#if defined(CFG_ENV_IS_IN_NAND) /* Environment is in Nand Flash */

#include <command.h>
#include <environment.h>
#include <linux/stddef.h>
#include <malloc.h>
#include <linux/mtd/nand.h>
#include <nand.h>
#if ((CONFIG_COMMANDS&(CFG_CMD_ENV|CFG_CMD_NAND)) == (CFG_CMD_ENV|CFG_CMD_NAND))
#define CMD_SAVEENV
#endif

#if defined(CFG_ENV_SIZE_REDUND)
#error CFG_ENV_SIZE_REDUND  not supported yet
#endif

#if defined(CFG_ENV_ADDR_REDUND)
#error CFG_ENV_ADDR_REDUND and CFG_ENV_IS_IN_NAND not supported yet
#endif

#ifdef CONFIG_INFERNO
#error CONFIG_INFERNO not supported yet
#endif

/* references to names in cmd_nand.c */
#define NANDRW_READ	0x01
#define NANDRW_WRITE	0x00
#define NANDRW_JFFS2	0x02
extern struct nand_chip nand_dev_desc[];
int nand_rw (struct nand_chip* nand, int cmd,
	    size_t start, size_t len,
	    size_t * retlen, u_char * buf);

/* references to names in env_common.c */
extern uchar default_environment[];
extern int default_environment_size;

char * env_name_spec = "NAND";


#ifdef ENV_IS_EMBEDDED
extern uchar environment[];
env_t *env_ptr = (env_t *)(&environment[0]);
#else /* ! ENV_IS_EMBEDDED */
env_t *env_ptr = 0;
#endif /* ENV_IS_EMBEDDED */


/* local functions */
static void use_default(void);


uchar env_get_char_spec (int index)
{
	DECLARE_GLOBAL_DATA_PTR;

	return ( *((uchar *)(gd->env_addr + index)) );
}


/* this is called before nand_init()
 * so we can't read Nand to validate env data.
 * Mark it OK for now. env_relocate() in env_common.c
 * will call our relocate function which will does
 * the real validation.
 */
int env_init(void)
{
	DECLARE_GLOBAL_DATA_PTR;

  	gd->env_addr  = (ulong)&default_environment[0];
	gd->env_valid = 1;

	return (0);
}

#ifdef CMD_SAVEENV
int saveenv(void)
{
	int	i, ret = 0;
	size_t	size;
	int badcnt=0, badreg=0;
	off_t from;
	nand_erase_options_t opts;
	nand_info_t *nand;

	nand=&nand_info[nand_curr_device];

	from = CFG_ENV_OFFSET;
	for (i=0; i < CFG_ENV_NAND_REDUNDANT_REGION ; i++){
		while(nand_block_isbad(nand, from)) {
			printf("block at offset %p is bad, skipping bad block.\n", from);
			from += nand->erasesize;
			badcnt++;

			if (badcnt >= CFG_ENV_BAD_BLOCKS) {
				badreg++;
				printf("Too many bad blocks within Enviroinment region %d.\n", badreg);

				/* bad regions to save env.*/
				if ( badreg >= CFG_ENV_NAND_REDUNDANT_REGION ) {
					printf("Writing to NAND ... Fail\n");
					return 1;
				}
				printf("Check Redundant region at offset %p\n", CFG_ENV_NAND_REDUNDANT_OFFSET);
				from = CFG_ENV_NAND_REDUNDANT_OFFSET;
				badcnt = 0;
				break;
			}
		}
		if ( badreg == i+1 )
			continue;

		puts ("Erasing Nand...");
		memset(&opts, 0, sizeof(opts));
		opts.offset = from;
		opts.length = nand->erasesize;
        	opts.jffs2  = 0;
        	opts.quiet  = 0;
	
		ret = nand_erase_opts(nand, &opts);
        	printf("%s\n", ret ? "ERROR" : "OK");

		if ( ret == 1 )
			return ret;

		size = CFG_ENV_SIZE;
		ret = nand_write(nand, from, &size, (u_char *)env_ptr);

		printf(" %d bytes %s: %s\n", CFG_ENV_SIZE, "written", ret ? "ERROR" : "OK");

		from = CFG_ENV_NAND_REDUNDANT_OFFSET;
		badcnt = 0;
	}
	puts ("Writing to Nand... ");
	puts ("done\n");

	return ret == 0 ? 0 : 1;
}
#endif /* CMD_SAVEENV */


void env_relocate_spec (void)
{
#if !defined(ENV_IS_EMBEDDED)
	int ret, badcnt=0, badreg=0;
	size_t size;
	nand_info_t *nand;
	off_t from;

	nand=&nand_info[nand_curr_device];

	size = CFG_ENV_SIZE;

	from = CFG_ENV_OFFSET;

	while(nand_block_isbad(nand, from)) {
		printf("block at offset %p is bad, skipping bad block.\n", from);
      		from += nand->erasesize;
		badcnt++;

		if (badcnt >= CFG_ENV_BAD_BLOCKS) {
			badreg++;
			printf("Too many bad blocks within Enviroinment region %d.\n", badreg);
			if ( badreg >= CFG_ENV_NAND_REDUNDANT_REGION )
				return use_default();
	
			printf("Check Redundant region at offset %p\n", CFG_ENV_NAND_REDUNDANT_OFFSET);
			from = CFG_ENV_NAND_REDUNDANT_OFFSET;
			badcnt = 0;
		}
	}

	ret = nand_read(nand, from, &size, (u_char *)env_ptr);

	if (ret)
		return use_default();

	if (crc32(0, env_ptr->data, CFG_ENV_SIZE - offsetof(env_t, data)) != env_ptr->crc)
		return use_default();
#endif /* ! ENV_IS_EMBEDDED */

}

static void use_default()
{
	DECLARE_GLOBAL_DATA_PTR;

	puts ("*** Warning - bad CRC, using default environment\n\n");

  	if (default_environment_size > CFG_ENV_SIZE){
		puts ("*** Error - default environment is too large\n\n");
		return;
	}

	memset (env_ptr, 0, sizeof(env_t));
	memcpy (env_ptr->data,
			default_environment,
			default_environment_size);

	env_ptr->crc = crc32(0, env_ptr->data, CFG_ENV_SIZE - offsetof(env_t, data));
 	gd->env_valid = 1;

}

#endif /* CFG_ENV_IS_IN_NAND */
