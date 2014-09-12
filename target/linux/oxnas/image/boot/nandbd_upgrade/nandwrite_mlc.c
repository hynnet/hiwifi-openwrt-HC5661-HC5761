
#include "ox820.h"
#include "common.h"

#include "mtd-user.h"

int nandwrite_mlc(char *image_path, int dev, loff_t mtdoffset,
	struct mtd_info_user *meminfo)
{
	int cnt = 0;
	int image = -1;
	int imglen = 0, pagesize, blocksize, badblocks = 0;
	unsigned int offs;
	int ret;
	bool read_next = true;
	unsigned char *writebuf = NULL;
	int retCode = 0;

	uint32_t nblock, npage, skip;

	int total_blocks, pagesperblock, blockskip;

	image = open(image_path, O_RDONLY);
	if (image == -1) {
		perror("open error");
		return -1;
	}

	imglen = lseek(image, 0, SEEK_END);
	lseek (image, 0, SEEK_SET);

	pagesize = meminfo->writesize;
	blocksize = meminfo->erasesize;
	// Check, if length fits into device
	total_blocks = meminfo->size / blocksize;
	pagesperblock = blocksize / pagesize;
	blockskip = (MLC_MAX_IMG_SIZ / pagesize + 1) * CONFIG_PAGE_REPLICATION * CONFIG_BLOCK_REPLICATION / pagesperblock;

	if ((blockskip * 2) > total_blocks || imglen > MLC_MAX_IMG_SIZ) {
		show_nand_info(stderr, meminfo);
		perror("Assigned max image size does not fit into device");
		retCode = -2;
		goto closeall;
	}

	// Allocate a buffer big enough to contain all the data for one page
	writebuf = (unsigned char*)MALLOC(pagesize);
	erase_buffer(writebuf, pagesize);

	while ((imglen > 0)	&& (mtdoffset < meminfo->size))
	{
		int readlen = pagesize;
		int tinycnt = 0;

		skip = 0; badblocks = 0;

		if (read_next) {
			erase_buffer(writebuf, readlen);
			/* Read up to one page data */
			while (tinycnt < readlen) {
				cnt = read(image, writebuf + tinycnt, readlen - tinycnt);
				if (cnt == 0) { /* EOF */
					break;
				} else if (cnt < 0) {
					perror ("File I/O error on input");
					retCode = -3;
					goto closeall;
				}
				tinycnt += cnt;
			}
			imglen -= tinycnt;
			read_next = false;
		}

		for (nblock = 0; nblock < CONFIG_BLOCK_REPLICATION; nblock++)
		{
//			offs = mtdoffset + nblock * blocksize + skip * blocksize;
			offs = mtdoffset + skip * blocksize;
			// skip bad blocks
			ret = nand_block_isbad(dev, offs);
			if (ret < 0) {
				retCode = -5;
				goto closeall;
			} else if (ret == 1) {
#if 0
				loff_t checkblock;
				char have_copy = 0;

				if (!quiet) {
					fprintf(stderr, "Skip bad block at address 0x%x, (block %u)\n",
							offs, offs / blocksize);
				}

				badblocks++;
				// make sure we have at least one copy
				for (checkblock = 0; checkblock < CONFIG_BLOCK_REPLICATION; checkblock++)
				{
					offs = mtdoffset + checkblock * blocksize + skip * blocksize;
					ret = nand_block_isbad(dev, offs);
					if (ret < 0) goto closeall;
					else if (ret == 0) {
						have_copy = 1;
						break;
					}
				}
				if (!have_copy)
				{
					printf("Too many bad blocks\n");
					goto closeall;
				}
				skip += blockskip;
				continue;
#else
				if (!quiet) {
					uint32_t block_mask = meminfo->erasesize - 1;
					printf("Bad block 0x%x\n", (offs & (~block_mask)));
				}
				if (++badblocks >= CONFIG_BLOCK_REPLICATION) {
					printf("Too many bad blocks\n");
					retCode = -4;
					goto closeall;
				}
				skip += blockskip;
				continue;
#endif
			}

			for (npage = 0; npage < CONFIG_PAGE_REPLICATION; npage++)
			{
				offs = mtdoffset + npage * pagesize + skip * blocksize;
				/* Write out the Page data */
				if (pwrite(dev, writebuf, pagesize, offs) != pagesize) {
					fprintf(stderr, "Bad page for copy %u of block %x for address %x\n",
							npage, nblock, offs);
				}
			}
			skip += blockskip;
			read_next = true;
		} // for nblock
		mtdoffset += pagesize * CONFIG_PAGE_REPLICATION;
	} // while (imglen > 0)

closeall:
	if (writebuf) {
		free(writebuf);
	}

	close(image);

	if (imglen > 0)
	{
		fprintf(stderr, "Data was only partially written due to error\n");
	}

	return retCode;
}
