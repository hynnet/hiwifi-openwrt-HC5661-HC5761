/*
 * libdbi - database independent abstraction layer for C.
 * Copyright (C) 2001-2003, David Parker and Mark Tobenkin.
 * http://libdbi.sourceforge.net
 * 
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 * 
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 * 
 * $Id: dbd.h,v 1.29 2005/08/15 19:18:18 mhoenicka Exp $
 */


#ifndef __DBD_H__
#define __DBD_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <dbi/dbi.h>
#include <dbi/dbi-dev.h>

/* FUNCTIONS EXPORTED BY EACH DRIVER */
void dbd_register_driver(const dbi_info_t **_driver_info, const char ***_custom_functions, const char ***_reserved_words);
int dbd_initialize(dbi_driver_t *driver);
int dbd_connect(dbi_conn_t *conn);
int dbd_disconnect(dbi_conn_t *conn);
int dbd_fetch_row(dbi_result_t *result, unsigned long long rowidx);
int dbd_free_query(dbi_result_t *result);
int dbd_goto_row(dbi_result_t *result, unsigned long long rowidx);
int dbd_get_socket(dbi_conn_t *conn);
const char *dbd_get_encoding(dbi_conn_t *conn);
const char* dbd_encoding_from_iana(const char *iana_encoding);
const char* dbd_encoding_to_iana(const char *iana_encoding);
char *dbd_get_engine_version(dbi_conn_t *conn, char *versionstring);
dbi_result_t *dbd_list_dbs(dbi_conn_t *conn, const char *pattern);
dbi_result_t *dbd_list_tables(dbi_conn_t *conn, const char *db, const char *pattern);
dbi_result_t *dbd_query(dbi_conn_t *conn, const char *statement);
dbi_result_t *dbd_query_null(dbi_conn_t *conn, const unsigned char *statement, size_t st_length);
size_t dbd_quote_string(dbi_driver_t *driver, const char *orig, char *dest);
size_t dbd_quote_binary(dbi_conn_t *conn, const unsigned char *orig, size_t from_length, unsigned char **ptr_dest);
size_t dbd_conn_quote_string(dbi_conn_t *conn, const char *orig, char *dest);
const char *dbd_select_db(dbi_conn_t *conn, const char *db);
int dbd_geterror(dbi_conn_t *conn, int *errno, char **errstr);
unsigned long long dbd_get_seq_last(dbi_conn_t *conn, const char *sequence);
unsigned long long dbd_get_seq_next(dbi_conn_t *conn, const char *sequence);
int dbd_ping(dbi_conn_t *conn);

/* _DBD_* DRIVER AUTHORS HELPER FUNCTIONS */
dbi_result_t *_dbd_result_create(dbi_conn_t *conn, void *handle, unsigned long long numrows_matched, unsigned long long numrows_affected);
void _dbd_result_set_numfields(dbi_result_t *result, unsigned int numfields);
void _dbd_result_add_field(dbi_result_t *result, unsigned int fieldidx, char *name, unsigned short type, unsigned int attribs);
dbi_row_t *_dbd_row_allocate(unsigned int numfields);
void _dbd_row_finalize(dbi_result_t *result, dbi_row_t *row, unsigned long long rowidx);
void _dbd_internal_error_handler(dbi_conn_t *conn, const char *errmsg, const int errno);
dbi_result_t *_dbd_result_create_from_stringarray(dbi_conn_t *conn, unsigned long long numrows_matched, const char **stringarray);
void _dbd_register_driver_cap(dbi_driver_t *driver, const char *capname, int value);
void _dbd_register_conn_cap(dbi_conn_t *conn, const char *capname, int value);
int _dbd_result_add_to_conn(dbi_result_t *result);
time_t _dbd_parse_datetime(const char *raw, unsigned int attribs);
size_t _dbd_escape_chars(char *dest, const char *orig, size_t orig_size, const char *toescape);
size_t _dbd_encode_binary(const unsigned char *in, size_t n, unsigned char *out);
size_t _dbd_decode_binary(const unsigned char *in, unsigned char *out);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif	/* __DBD_H__ */
