
#include "ox820.h"
#include "common.h"

int nand_block_isbad (int fd, loff_t offs)
{
	int ret;

	if ((ret = ioctl(fd, MEMGETBADBLOCK, &offs)) < 0) {
		perror("ioctl(MEMGETBADBLOCK)");
	}

	return ret;
}

static void display_help (void)
{
	printf(
"Usage: nandwrite [OPTION] MTD_DEVICE\n"
"Writes to the specified MTD device.\n"
"\n"
"  -q, --quiet              Don't display progress messages\n"
"  -i, --info               Dump flash info\n"
"      --blockreplic=cnt    Block replication in MLC write\n"
"      --pagereplic=cnt     Page replication in MLC write\n"
"      --maximage=size      Maximum kernel image size\n"
"      --sblock=addr        Block address of stage1 image to be written\n"
"      --ublock=addr        Block address of u-boot image to be written\n"
"      --kblock=addr        Block address of kernel image to be written\n"
"  -s fname, --stage1=fname Set the file name of stage1 boot loader\n"
"  -u fname, --uboot=fname  Set the file name of u-boot\n"
"  -k fname, --kernel=fname Set the file name of kernel\n"
"      --help              Display this help and exit\n"
	);
	exit (EXIT_SUCCESS);
}

static const char	*mtd_device;
static bool		viewinfo = false;

bool		quiet = false;
uint8_t	CONFIG_BLOCK_REPLICATION = 2;
uint8_t	CONFIG_PAGE_REPLICATION = 2;
uint32_t MLC_MAX_IMG_SIZ = 3 * 1024 * 1024;
uint8_t CONFIG_STAGE1_BLOCK = SDK_BUILD_NAND_STAGE1_BLOCK;
uint8_t CONFIG_UBOOT_BLOCK = SDK_BUILD_NAND_STAGE2_BLOCK;
uint8_t CONFIG_KERNEL_BLOCK = SDK_BUILD_NAND_KERNEL_BLOCK;


static char *stage1 = NULL;
static char *uboot = NULL;
static char *kernel = NULL;

#define _GNU_SOURCE
#include <getopt.h>
static void process_options (int argc, char * const argv[])
{
	int error = 0;
	int size;

	for (;;) {
		int option_index = 0;
		static const char *short_options = "s:qil:u:k:";
		static const struct option long_options[] = {
			{"help", no_argument, 0, 0},
			{"blockreplic", required_argument, 0, 0},
			{"pagereplic", required_argument, 0, 0},
			{"maximage", required_argument, 0, 0},
			{"sblock", required_argument, 0, 0},
			{"ublock", required_argument, 0, 0},
			{"kblock", required_argument, 0, 0},
			{"info", no_argument, 0, 'i'},
			{"quiet", no_argument, 0, 'q'},
			{"stage1", required_argument, 0, 's'},
			{"uboot", required_argument, 0, 'u'},
			{"kernel", required_argument, 0, 'k'},
			{0, 0, 0, 0},
		};

		int c = getopt_long(argc, argv, short_options,
				long_options, &option_index);
		if (c == EOF) {
			break;
		}

		switch (c) {
			case 0:
				switch (option_index) {
					case 0:
						display_help();
						break;
					case 1:
						CONFIG_BLOCK_REPLICATION = strtoul(optarg, NULL, 0);
						break;
					case 2:
						CONFIG_PAGE_REPLICATION = strtoul(optarg, NULL, 0);
						break;
					case 3:
						MLC_MAX_IMG_SIZ = strtoul(optarg, NULL, 0);
						break;
					case 4:
						CONFIG_STAGE1_BLOCK = strtoul(optarg, NULL, 0);
						break;
					case 5:
						CONFIG_UBOOT_BLOCK = strtoul(optarg, NULL, 0);
						break;
					case 6:
						CONFIG_KERNEL_BLOCK = strtoul(optarg, NULL, 0);
						break;
				}
				break;
			case 'q':
				quiet = true;
				break;
			case 'i':
				viewinfo = true;
				break;
			case 's':
				size = strlen(optarg) + 1;
				stage1 = MALLOC(size);
				snprintf(stage1, size, "%s", optarg);
				break;
			case 'u':
				size = strlen(optarg) + 1;
				uboot = MALLOC(size);
				snprintf(uboot, size, "%s", optarg);
				break;
			case 'k':
				size = strlen(optarg) + 1;
				kernel = MALLOC(size);
				snprintf(kernel, size, "%s", optarg);
				break;
			case '?':
				error++;
				break;
		}
	}

	argc -= optind;
	argv += optind;

	if (argc != 1 || error)
		display_help ();

	mtd_device = argv[0];
}

void show_nand_info(FILE *stream, struct mtd_info_user *meminfo)
{
	fprintf(stream, "type = %u, flags = 0x%x\n", meminfo->type, meminfo->flags);
	fprintf(stream, "size = %u MiB\n", meminfo->size/1024/1024);
	fprintf(stream, "erasesize = %u KiB\n", meminfo->erasesize/1024);
	fprintf(stream, "writesize = %u KiB\n", meminfo->writesize/1024);
	fprintf(stream, "oobsize = %u Byte\n", meminfo->oobsize);
	fprintf(stream, "ecctype = %u\n", meminfo->ecctype);
	fprintf(stream, "eccsize = %u Byte\n", meminfo->eccsize);

	fprintf(stream, "Max Image Size = %u MiB\n", MLC_MAX_IMG_SIZ/1024/1024);
	fprintf(stream, "Block Replication = %u\n", CONFIG_BLOCK_REPLICATION);
	fprintf(stream, "Page Replication = %u\n", CONFIG_PAGE_REPLICATION);
	fprintf(stream, "Stage1 block = %u\n", CONFIG_STAGE1_BLOCK);
	fprintf(stream, "Uboot block = %u\n", CONFIG_UBOOT_BLOCK);
	fprintf(stream, "kernel block = %u\n", CONFIG_KERNEL_BLOCK);
}

/*
 * Main program
 */
int main(int argc, char * const argv[])
{
	int dev = -1;
	struct mtd_info_user meminfo;
	struct nand_oobinfo old_oobinfo;
	loff_t mtdoffset;
	int retCode = 0;

	process_options(argc, argv);

	/* Open the device */
	if ((dev = open(mtd_device, O_RDWR)) == -1) {
		perror(mtd_device);
		exit (EXIT_FAILURE);
	}

	/* Fill in MTD device capability structure */
	if (ioctl(dev, MEMGETINFO, &meminfo) != 0) {
		perror("MEMGETINFO");
		close(dev);
		exit (EXIT_FAILURE);
	}

	/* Make sure device page sizes are valid */
	if (!(meminfo.oobsize == 16 && meminfo.writesize == 512) &&
			!(meminfo.oobsize == 8 && meminfo.writesize == 256) &&
			!(meminfo.oobsize == 64 && meminfo.writesize == 2048) &&
			!(meminfo.oobsize == 128 && meminfo.writesize == 4096)) {
		fprintf(stderr, "Unknown flash (not normal NAND)\n");
		close(dev);
		exit (1);
	}

	if (viewinfo) {
		show_nand_info(stdout, &meminfo);

		if (ioctl (dev, MEMGETOOBSEL, &old_oobinfo) != 0) {
			perror ("MEMGETOOBSEL");
			close (dev);
			exit (1);
		}
		printf("oob useecc = %u\n", old_oobinfo.useecc);
		printf("oob eccbytes = %u\n", old_oobinfo.eccbytes);

		exit(0);
	}

	if (stage1) {
		mtdoffset = CONFIG_STAGE1_BLOCK * meminfo.erasesize;
		fprintf(stderr, "Writing %s to %s, offset 0x%x\n", stage1,
				mtd_device, (unsigned int)mtdoffset);
		if ((retCode = write_fec(stage1, dev, mtdoffset, &meminfo)) < 0)
			goto out;
	}
	if (uboot) {
		mtdoffset = CONFIG_UBOOT_BLOCK * meminfo.erasesize;
		fprintf(stderr, "Writing %s to %s, offset 0x%x\n", uboot,
				mtd_device, (unsigned int)mtdoffset);
		if ((retCode = write_fec(uboot, dev, mtdoffset, &meminfo)) < 0)
			goto out;
	}
	if (kernel) {
		mtdoffset = CONFIG_KERNEL_BLOCK * meminfo.erasesize;
		fprintf(stderr, "Writing %s to %s, offset 0x%x\n", kernel,
				mtd_device, (unsigned int)mtdoffset);
		if ((retCode = nandwrite_mlc(kernel, dev, mtdoffset, &meminfo)) < 0)
			goto out;
	}

out:
	close(dev);
	return retCode;
}

