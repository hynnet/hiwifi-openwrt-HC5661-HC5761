/*  *********************************************************************
    HCWIFI
    support flash bdinfo and recovery
    ********************************************************************* */

#include "lib_types.h"
#include "lib_string.h"
#include "lib_queue.h"
#include "lib_malloc.h"
#include "lib_printf.h"

#include "cfe_iocb.h"
#include "cfe_devfuncs.h"
#include "cfe_ioctl.h"
#include "cfe_timer.h"
#include "cfe_error.h"

#include "ui_command.h"
#include "cfe.h"

#include "cfe_fileops.h"
#include "cfe_boot.h"
#include "bsp_config.h"

#include "cfe_loader.h"

#include "net_ebuf.h"
#include "net_ether.h"
#include "net_api.h"

#include "addrspace.h"
#include "initdata.h"
#include "url.h"

#include <osl.h>
#include <bcmutils.h>
#include <bcmendian.h>
#include <trxhdr.h>
#include <hndsoc.h>
#include <siutils.h>
#include <sbchipc.h>
#include "bsp_priv.h"

typedef enum _hc_img_type
{
    BDINFO_IMG_TYPE = 1,
    SYSUP_IMG_TYPE,
    FLASH_IMG_TYPE,
}hc_img_type;

int ui_init_hccmd(void);
static int ui_cmd_hccmd(ui_cmdline_t *cmd,int argc,char *argv[]);
extern void ui_get_flash_buf(uint8_t **bufptr, int *bufsize);
extern unsigned int flash_crc32(const unsigned char *databuf, unsigned int  datalen);
extern void ui_check_flashdev(char *in, char *out);

/*  *********************************************************************
    *  ui_init_hccmd()
    *  
    *  Initialize the hc flash commands, add them to the table.
    *  
    *  Input parameters: 
    *  	   nothing
    *  	   
    *  Return value:
    *  	   0 if ok, else error
    ********************************************************************* */

int ui_init_hccmd(void)
{
    cmd_addcmd("hccmd",
	       ui_cmd_hccmd,
	       NULL,
	       "Update a hccmd flash memory device",
	       "bdinfo [options] filename\n\n"
	       "Copies data from a source file name or device to a hccmd flash memory device.\n"
               "The source device can be a disk file (FAT filesystem), a remote file\n"
               "(TFTP) or a flash device.  The destination device may be a flash or eeprom.\n"
#if !CFG_EMBEDDED_PIC
	       "If the destination device is your boot flash (usually flash0), the flash\n"
	       "command will restart the firmware after the flash update is complete\n"
#endif
               "",
	       "-mem;Use memory as source instead of a device");


    return 0;
}

/*  *********************************************************************
    *  bdinfo_validate(ptr)
    *  
    *  Validate the bdinfo header to make sure we can program it.
    *  
    *  Input parameters: 
    *  	   ptr - pointer to flash header
    *      outptr - pointer to data that we should program
    *	   outsize - size of data we should program
    *  	   
    *  Return value:
    *  	   0 if ok
    *  	   else error occured
    ********************************************************************* */
#define BDINFO_IMG_PARTTEN_SIZE     5
#define BDINFO_VERSION_SIZE         4
#define BDINFO_MIN_SIZE     (BDINFO_VERSION_SIZE + BDINFO_IMG_PARTTEN_SIZE) 
#define NVRAM_FLAG_OFFSET       0x400
#define CFE_SIZE            0x40000                 /* 256K */

static int img_chk_validate(uint8_t *ptr, int size)
{
    unsigned int calccrc;
    struct trx_header *trx = (struct trx_header *) ptr;
    uint8_t bdinfo_img_pattern[BDINFO_IMG_PARTTEN_SIZE] = {0x2d, 0x2d, 0x2d, 0x2d, 0x2d};
    uint8_t flash_img_pattern[]={0xff, 0x04, 0x00, 0xea, 0x14, 0xf0, 0x9f, 0xe5, 0x14, 0xf0, 0x9f, 0xe5};
    uint8_t nvram_flag[] = {0x46, 0x4c, 0x53, 0x48};
    int flash_img_flag = 0;

    if (size < BDINFO_MIN_SIZE)
    {
        return -1;
    }

    if (!memcmp(bdinfo_img_pattern, (ptr + BDINFO_VERSION_SIZE), sizeof(bdinfo_img_pattern)))
    {
        return BDINFO_IMG_TYPE;
    }

    if (!memcmp(flash_img_pattern, ptr, sizeof(flash_img_pattern)))
    {
        if (size < NVRAM_FLAG_OFFSET)
        {
            return -1;
        }

        if (!memcmp(nvram_flag, (ptr + NVRAM_FLAG_OFFSET), sizeof(nvram_flag)))
        {
            flash_img_flag = 1;
        }
    }

    if (flash_img_flag)
    {
        if (size <= CFE_SIZE)
        {
            xprintf("size is too small\n");

            return -1;
        }
        trx = (struct trx_header *) (ptr + CFE_SIZE);
    }

    if (ltoh32(trx->magic) != TRX_MAGIC)
    {
        return -1;
    }

    if (size < ltoh32(trx->len))
    {
        xprintf("sysup image size error\n");
        return -1;
    }


    /* Checksum over header */
    calccrc = hndcrc32((uint8 *) &(trx->flag_version),
                   ltoh32(trx->len) - OFFSETOF(struct trx_header, flag_version),
                   CRC32_INIT_VALUE);

    if (ltoh32(trx->crc32) != calccrc)
    {
        xprintf("sysup image crc error\n");
    }
    else
    {
        if (flash_img_flag)
        {
            return FLASH_IMG_TYPE;
        }
        else
        {
            return SYSUP_IMG_TYPE;
        }
    }

    return -1;
}

static int program_flash(char *flashdev, uint8_t *ptr, int size, int offset)
{
    int res;
    int fh;
    int amtcopy;
    flash_info_t flashinfo;
#if !CFG_EMBEDDED_PIC
    int retlen;
#endif
#ifdef CFG_NFLASH
    char buf[16];
#endif

    if (NULL == flashdev)
        return -1;

    if (size == 0)
        return 0;

#ifdef CFG_NFLASH
    ui_check_flashdev(flashdev, buf);
    flashdev = buf;
#endif 

    /*
     * Make sure it's a flash device.
     */
    res = cfe_getdevinfo(flashdev);
    if (res < 0) {
        ui_showerror(CFE_ERR_DEVNOTFOUND,flashdev);
        return -1;
    }

    if ((res != CFE_DEV_FLASH) && (res != CFE_DEV_NVRAM)) {
        xprintf("Device '%s' is not a flash or eeprom device.\n",flashdev);
        return -1;
    }

    /*
     * Open the destination flash device.
     */

    fh = cfe_open(flashdev);
    if (fh < 0) {
       xprintf("Could not open device '%s'\n",flashdev);
       return -1;
    }

    if (cfe_ioctl(fh,IOCTL_FLASH_GETINFO,
          (unsigned char *) &flashinfo,
          sizeof(flash_info_t),
          &res,0) == 0) {
        /* Truncate write if source size is greater than flash size */
        if ((size+ offset) > flashinfo.flash_size) {
            size = flashinfo.flash_size - offset;
        }
    }

    /*
     * If overwriting the boot flash, we need to use the special IOCTL
     * that will force a reboot after writing the flash.
     */

    if (flashinfo.flash_flags & FLASH_FLAG_INUSE) {
#if CFG_EMBEDDED_PIC
        xprintf("\n\n** DO NOT TURN OFF YOUR MACHINE UNTIL THE FLASH UPDATE COMPLETES!! **\n\n");
#else
#if CFG_NETWORK
        if (net_getparam(NET_DEVNAME)) {
            xprintf("Closing network.\n");
            net_uninit();
        }
#endif
        xprintf("Rewriting boot flash device '%s'\n",flashdev);
        xprintf("\n\n**DO NOT TURN OFF YOUR MACHINE UNTIL IT REBOOTS!**\n\n");
        cfe_ioctl(fh,IOCTL_FLASH_WRITE_ALL, ptr,size,&retlen,0);
        /* should not return */
        return -1;
#endif
    } 

    /*
     * Program the flash
     */

    xprintf("Programming Flash...");

    amtcopy = cfe_writeblk(fh,offset,ptr,size);

    if (size == amtcopy) {
       xprintf("done. %d bytes written\n",amtcopy);
       res = 0;
    }
    else {
       ui_showerror(amtcopy,"Failed.");
       res = -1;
    }
    
    /*  
     * done!
     */

    cfe_close(fh);

    return res;           
}

/*  *********************************************************************
    *  ui_cmd_hccmd(cmd,argc,argv)
    *  
    *  The 'hccmd' command lives here.  Program the bdinfo flash block,
    *  or if a device name is specified, program the alternate
    *  flash device.
    *  
    *  Input parameters: 
    *  	   cmd - command table entry
    *  	   argc,argv - parameters
    *  	   
    *  Return value:
    *  	   0 if ok
    *  	   else error
    ********************************************************************* */


static int ui_cmd_hccmd(ui_cmdline_t *cmd,int argc,char *argv[])
{
    uint8_t *ptr = NULL;
    int res;
    int bootdev;
    char *fname;
    char *flashdev;
    cfe_loadargs_t la;
    int copysize;
    int memsrc = 0; /* Source is memory address */
    int size = 0;
    int bufsize = 0;
    int img_type;

    bootdev = soc_boot_dev((void *)sih);

    /* Get staging buffer */
    memsrc = cmd_sw_isset(cmd,"-mem");

    /* If memory is not being used as a source, then get staging buffer */
    if (!memsrc)
	    ui_get_flash_buf(&ptr, &bufsize);

    /*
     * Parse command line parameters
     */

    fname = cmd_getarg(cmd,0);

    if (!fname) {
	   return ui_showusage(cmd);
	}

    /* Fix up the ptr and size for reading from memory
     * and skip loading to go directly to programming
     */
    if (memsrc) {
	    ptr = (uint8_t *)xtoi(fname);
	    bufsize = size;
	    res = size;
	    xprintf("Reading from %s: ",fname);
	    goto program;
    }

    xprintf("Reading %s: ",fname);

    res = ui_process_url(fname, cmd, &la);
    if (res < 0) {
	ui_showerror(res,"Invalid file name %s",fname);
	return res;
	}

    la.la_address = (intptr_t) ptr;
    la.la_options = 0;
    la.la_maxsize = bufsize;
    la.la_flags =  LOADFLG_SPECADDR;

    res = cfe_load_program("raw",&la);

    if (res < 0) {
	  ui_showerror(res,"Failed.");
	  return res;
	}

    xprintf("Done. %d bytes read\n",res);

program:
    copysize = res;

	img_type = img_chk_validate(ptr, res);

    if (img_type < 0)
    {
        xprintf("img header chk failed\n");
        return -1;
    }

    switch(img_type)
    {
    case BDINFO_IMG_TYPE:
#ifdef CFG_NFLASH
        if (bootdev == SOC_BOOTDEV_NANDFLASH)
        {    
            flashdev = "nflash0.bdinfo";
        }
#endif
#ifdef CFG_SFLASH
        if (bootdev == SOC_BOOTDEV_SFLASH)
        {    
            flashdev = "flash0.bdinfo";
        }
#endif    
        break;
    case SYSUP_IMG_TYPE:
#ifdef CFG_NFLASH
        if (bootdev == SOC_BOOTDEV_NANDFLASH)
        {    
            flashdev = "nflash0.trx";
        }
#endif
#ifdef CFG_SFLASH
        if (bootdev == SOC_BOOTDEV_SFLASH)
        {    
            flashdev = "flash0.trx";
        }
#endif
        break;
    }

    if (img_type == FLASH_IMG_TYPE)
    {
#ifdef CFG_NFLASH
        if (bootdev == SOC_BOOTDEV_NANDFLASH)
        {    
            flashdev = "nflash0.boot";
        }
#endif
#ifdef CFG_SFLASH
        if (bootdev == SOC_BOOTDEV_SFLASH)
        {    
            flashdev = "flash0.boot";
        }
#endif
        res = program_flash(flashdev, ptr, CFE_SIZE, 0);

#ifdef CFG_NFLASH
        if (bootdev == SOC_BOOTDEV_NANDFLASH)
        {    
            flashdev = "nflash0.trx";
        }
#endif
#ifdef CFG_SFLASH
        if (bootdev == SOC_BOOTDEV_SFLASH)
        {    
            flashdev = "flash0.trx";
        }
#endif
        res = program_flash(flashdev, ptr + CFE_SIZE, copysize - CFE_SIZE, 0);
    }
    else
    {
        program_flash(flashdev, ptr, copysize, 0);
    }

    if (img_type == BDINFO_IMG_TYPE)
    {
        xprintf("write bdinfo ok\n");

        while(1)
        {
            ui_docommand("ping 192.168.1.88");
            OSL_DELAY(300*1000);
        }
    }
    else
    {
        xprintf("copy image ok\n");
        OSL_DELAY(1000);
        ui_docommand("reboot");
    }

    return res;
}
