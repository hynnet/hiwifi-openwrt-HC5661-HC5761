/* LowLevel function for soft SPI environment support
 * Author : PLX semiconductor.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 *
 */
#include <common.h>

#if defined(CFG_ENV_IS_IN_SPI) /* Environment is in SPI Flash */

#include <command.h>
#include <environment.h>
#include <linux/stddef.h>
#include <malloc.h>
#include <spi_flash.h>

#ifdef ENV_IS_EMBEDDED
extern uchar environment[];
env_t *env_ptr = (env_t *)(&environment[0]);
#else /* ! ENV_IS_EMBEDDED */
env_t *env_ptr = 0;
#endif /* ENV_IS_EMBEDDED */

char * env_name_spec = "SPIflash";

static struct spi_flash *flash;

static u32 cfg_env_addr;

#ifndef CONFIG_SF_DEFAULT_SPEED
# define CONFIG_SF_DEFAULT_SPEED        1000000
#endif
#ifndef CONFIG_SF_DEFAULT_MODE
# define CONFIG_SF_DEFAULT_MODE         SPI_MODE_3
#endif

extern uchar default_environment[];
extern int default_environment_size;

static void use_default()
{
        DECLARE_GLOBAL_DATA_PTR;

        puts ("*** Warning - bad CRC or NAND, using default environment\n\n");

        memset (env_ptr, 0, sizeof(env_t));
        memcpy (env_ptr->data,
                        default_environment,
                        default_environment_size);

      env_ptr->crc = crc32(0, env_ptr->data, CFG_ENV_SIZE - offsetof(env_t, data));
              gd->env_valid = 1;
}
              
uchar env_get_char_spec (int index)
{
        DECLARE_GLOBAL_DATA_PTR;

        return ( *((uchar *)(gd->env_addr + index)) );
}

void env_relocate_spec (void)
{
#if !defined(ENV_IS_EMBEDDED)
	struct spi_flash *new;
	int ret;

	new = spi_flash_probe(0, 0, CONFIG_SF_DEFAULT_SPEED, CONFIG_SF_DEFAULT_MODE);
	if (!new) {
		printf("Failed to initialize SPI flash at %u:%u\n", 0, 0);
		use_default();
		return 1;
	}

	if (flash)
		spi_flash_free(flash);
	flash = new;

	cfg_env_addr = (flash->size) - CFG_ENV_OFFSET;

	printf(" Reading Environment from %s...\n", env_name_spec);

	ret = spi_flash_read(flash, cfg_env_addr, CFG_ENV_SIZE, (void *)env_ptr);

	if (ret) {
		printf("SPI flash read failed\n");
		return use_default();
	}

	if (crc32(0, env_ptr->data, CFG_ENV_SIZE - offsetof(env_t, data)) != env_ptr->crc)
	{
		return use_default();
	}
#endif /* ! ENV_IS_EMBEDDED */
}

int saveenv(void)
{
	int ret;

	ret = spi_flash_erase(flash, cfg_env_addr, CFG_ENV_SIZE);
	//Synology use M25P32 SPI. The sector erase size is 64KB.  
	//ret = spi_flash_erase(flash, cfg_env_addr, CFG_ENV_OFFSET);

        if (ret) {
                printf("SPI flash erase failed\n");
                return 1;
        }

	env_ptr->crc = crc32(0, env_ptr->data, CFG_ENV_SIZE - offsetof(env_t, data));
	ret = spi_flash_write(flash, cfg_env_addr, CFG_ENV_SIZE, (void *)env_ptr);

	if (ret) {
		printf("SPI flash write failed\n");
	}
	return ret;
}

/************************************************************************
 * Initialize Environment use
 *
 * We are still running from ROM, so data use is limited
 * Use a (moderately small) buffer on the stack
 */
int env_init(void)
{
	DECLARE_GLOBAL_DATA_PTR;
	
	gd->env_addr  = (ulong)&default_environment[0];
        gd->env_valid = 1;

        return (0);
}

#endif /* CFG_ENV_IS_IN_SPI */
