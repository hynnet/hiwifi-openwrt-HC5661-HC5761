/*
 * Broadcom Home Gateway Reference Design
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
 * $Id: ezc.h 241182 2011-02-17 21:50:03Z $
 */

#ifndef _ezc_h_
#define _ezc_h_

#define EZC_VERSION_STR		"2"

#define EZC_FLAGS_READ		0x0001
#define EZC_FLAGS_WRITE		0x0002

#define EZC_SUCCESS		0
#define EZC_ERR_NOT_ENABLED 	1
#define EZC_ERR_INVALID_STATE 	2
#define EZC_ERR_INVALID_DATA 	3

/* ezc function callback with two args: name and value */
typedef void (*ezc_cb_fn_t)(char *, char *);

int ezc_register_cb(ezc_cb_fn_t fn);

void do_apply_ezconfig_post(char *url, FILE *stream, int len, char *boundary);
void do_ezconfig_asp(char *url, FILE *stream);

#endif /* _ezc_h_ */
