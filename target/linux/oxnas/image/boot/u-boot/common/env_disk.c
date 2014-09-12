/*
 * (C) Copyright 2006
 * Oxford Semiconductor Ltd
 *
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
#include <common.h>

#if defined(CFG_ENV_IS_IN_DISK)

#include <command.h>
#include <environment.h>
#include <ide.h>

extern int is_device_present(int device_number);

/* Point to the environment as held in SRAM */
env_t *env_ptr = NULL;

char *env_name_spec = "Disk";

/* The default environment compiled into U-Boot */
extern uchar default_environment[];

uchar env_get_char_spec(int index)
{
    DECLARE_GLOBAL_DATA_PTR;

    return *((uchar *)(gd->env_addr + index));
}

#define offsetof(TYPE, MEMBER) ((size_t) &((TYPE *)0)->MEMBER)

void env_relocate_spec(void)
{
    /* Compute the CRC of the environment in SRAM, copied from disk at boot */
    env_t *sram_env = (env_t*)CFG_ENV_ADDR;
    ulong  crc = crc32(0, sram_env->data, CFG_ENV_SIZE - offsetof(env_t, data));

    /* Copy the SRAM environment and CRC to the working environment */
    memcpy(env_ptr->data, sram_env->data, CFG_ENV_SIZE - offsetof(env_t, data));
    env_ptr->crc = crc;
}

int saveenv(void)
{
    /* Compute the CRC of the working environment */
    env_ptr->crc = crc32(0, env_ptr->data, CFG_ENV_SIZE - offsetof(env_t, data));

    /* Copy the working environment to the reserved area on each disk device */
    int status = 1;
    int i;
    for (i=0; i < CFG_IDE_MAXDEVICE; ++i) {
        if (!is_device_present(i)) {
            continue;
        }

		/* Write environment to the main environment area on disk */
        unsigned long written = ide_write(i, CFG_ENV_DISK_SECTOR, CFG_ENV_SIZE/512, (ulong*)env_ptr);
        if (written != CFG_ENV_SIZE/512) {
			printf("Saving environment to disk %d primary image failed\n", i);
            status = 0;
        } else {
			/* Write environment to the redundant environment area on disk */
			written = ide_write(i, CFG_ENV_DISK_REDUNDANT_SECTOR, CFG_ENV_SIZE/512, (ulong*)env_ptr);
			if (written != CFG_ENV_SIZE/512) {
				printf("Saving environment to disk %d secondary image failed\n", i);
				status = 0;
			}
		}
    }

    return status;
}

static int check_sram_env_integrity(void)
{
    DECLARE_GLOBAL_DATA_PTR;

	env_t *sram_env = (env_t*)CFG_ENV_ADDR;
	ulong crc = crc32(0, sram_env->data, CFG_ENV_SIZE - offsetof(env_t, data));

	if (crc == sram_env->crc) {
		gd->env_addr  = (ulong)sram_env->data;
		gd->env_valid = 1;
	}

	return gd->env_valid;
}

int env_init(void)
{
    DECLARE_GLOBAL_DATA_PTR;

	/* Have not yet found a valid environment */
	gd->env_valid = 0;

	/* Need SATA available to load environment from alternate disk locations */
	ide_init();

	int i;
    for (i=0; i < CFG_IDE_MAXDEVICE; ++i) {
        if (!is_device_present(i)) {
            continue;
        }

		/* Read environment from the primary environment area on disk */
        unsigned long read = ide_read(i, CFG_ENV_DISK_SECTOR, CFG_ENV_SIZE/512, (ulong*)CFG_ENV_ADDR);
        if (read == CFG_ENV_SIZE/512) {
			/* Check integrity of primary environment data */
			if (check_sram_env_integrity()) {
				printf("Environment successfully read from disk %d primary image\n", i);
				break;
			}
        }

		/* Read environment from the secondary environment area on disk */
		read = ide_read(i, CFG_ENV_DISK_REDUNDANT_SECTOR, CFG_ENV_SIZE/512, (ulong*)CFG_ENV_ADDR);
		if (read == CFG_ENV_SIZE/512) {
			/* Check integrity of secondary environment data */
			if (check_sram_env_integrity()) {
				printf("Environment successfully read from disk %d secondary image\n", i);
				break;
			}
		}
	}

	if (!gd->env_valid) {
		printf("Failed to read valid environment from disk, using built-in default\n");
        gd->env_addr  = (ulong)default_environment;
        gd->env_valid = 0;
	}

    return 0;
}
#endif // CFG_ENV_IS_IN_DISK
