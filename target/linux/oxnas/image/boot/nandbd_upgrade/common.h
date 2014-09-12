#ifndef _COMMON_INC_
#define _COMMON_INC_

#include <stdio.h>
#include <stdlib.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <fcntl.h>

#include <ctype.h>
#include <errno.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include <asm/types.h>

#include "mtd-user.h"

extern int nand_block_isbad (int, loff_t);
extern void show_nand_info(FILE *, struct mtd_info_user *);
extern int nandwrite_mlc(char *, int, loff_t, struct mtd_info_user *);
extern int write_fec(char *, int, loff_t, struct mtd_info_user *);

extern bool		quiet;
//----------------------------------------------------------------------------
static inline void *MALLOC(size_t size)
{
	void *ptr = malloc(size);

	if (ptr == NULL) {
		perror("malloc error");
		exit(-errno);
	}
	return ptr;
}

static inline void erase_buffer(void *buffer, size_t size)
{
	const uint8_t kEraseByte = 0xff;

	if (buffer != NULL && size > 0) {
		memset(buffer, kEraseByte, size);
	}
}

#endif
