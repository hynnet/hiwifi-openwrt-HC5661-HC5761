/*
 * Frontend command-line utility for Linux NVRAM layer
 *
 * Copyright (C) 2012, Broadcom Corporation. All Rights Reserved.
 * 
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
 * SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION
 * OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN
 * CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 * $Id: main.c 364585 2012-10-24 18:17:16Z $
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <typedefs.h>
#include <bcmnvram.h>

void
usage(void)
{
	fprintf(stderr,
	        "usage: nvram [get name] [set name=value] "
	        "[unset name] [show] [commit] ...\n");
	exit(0);
}

/* NVRAM utility */
int
main(int argc, char **argv)
{
	char *name, *value, buf[MAX_NVRAM_SPACE];
	int size;

	/* Skip program name */
	--argc;
	++argv;

	if (!*argv)
		usage();

	/* Process the arguments */
	for (; *argv; ++argv) {
		if (!strcmp(*argv, "get")) {
			if (*++argv) {
				if ((value = nvram_get(*argv)))
					puts(value);
			}
		} else if (!strcmp(*argv, "set")) {
			if (*++argv) {
				strncpy(value = buf, *argv, sizeof(buf));
				name = strsep(&value, "=");
				nvram_set(name, value);
			}
		} else if (!strcmp(*argv, "unset")) {
			if (*++argv)
				nvram_unset(*argv);
		} else if (!strcmp(*argv, "commit")) {
			nvram_commit();
		} else if (!strcmp(*argv, "show") ||
		           !strcmp(*argv, "dump")) {
			nvram_getall(buf, sizeof(buf));
			for (name = buf; *name; name += strlen(name) + 1)
				puts(name);
			size = sizeof(struct nvram_header) + (int) name - (int) buf;
			if (**argv != 'd')
				fprintf(stderr, "size: %d bytes (%d left)\n",
				        size, MAX_NVRAM_SPACE - size);
		} else
			usage();
	}

	return 0;
}
