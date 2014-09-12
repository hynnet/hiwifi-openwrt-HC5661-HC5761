
#include "ox820.h"
#include "common.h"

#define FEC_ENCODE_SCALING	8
int write_fec(char *infile, int dev, loff_t mtdoffset,
		struct mtd_info_user *meminfo)
{
	int image = -1;
	int n, retCode = 0;

	unsigned char *inbuf, *outbuf;
	int pagesize = meminfo->writesize;
	int rdsize = pagesize / FEC_ENCODE_SCALING;

	if ((image = open(infile, O_RDONLY)) < 0) {
		perror("open error");
		return -1;
	}
	if (lseek(dev, mtdoffset, SEEK_SET) != mtdoffset) {
		perror("lseek error");
		return -4;
	}

	inbuf = (unsigned char *)MALLOC(rdsize);
	outbuf = (unsigned char *)MALLOC(pagesize);
	while ( (n = read(image, inbuf, rdsize)) > 0) {
		unsigned char *ppos = outbuf;
		int i, j;
		erase_buffer(outbuf, pagesize);
		for (i = 0; i < n; i++) {
			unsigned char plain = inbuf[i];
			unsigned char cypher;
			for (j = 0; j < 8; j++) {
				if (plain & 1)
					cypher = 0x55;
				else
					cypher = 0xAA;
				plain >>= 1;
				*ppos++ = cypher;
			}
		}
		if (nand_block_isbad(dev, mtdoffset)) {
			mtdoffset += meminfo->erasesize;
			if (lseek(dev, mtdoffset, SEEK_SET) != mtdoffset) {
				perror("lseek error");
				retCode = -4;
				goto out;
			}
		}
		/* Should check bad block first */
		if (write(dev, outbuf, pagesize) != pagesize) { 
			perror("Write Error");
			retCode = -2;
			goto out;
		}
		mtdoffset += pagesize;
	}

	if (n < 0) {
		perror("Read Error");
		retCode = -3;
	}
out:
	close(image);
	free(inbuf);
	free(outbuf);
	return retCode;
}

