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
 * $Id: dbi.h.in,v 1.3 2008/02/06 19:34:27 mhoenicka Exp $
 */

#ifndef __DBI_H__
#define __DBI_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdlib.h>
#include <stdarg.h>
#include <time.h>
#include <limits.h> /* for the *_MAX definitions */

#if defined _MSC_VER && _MSC_VER >= 1300
#define LIBDBI_API_DEPRECATED __declspec(deprecated)
#elif defined __GNUC__ && (__GNUC__ > 3 || (__GNUC__ == 3 && __GNUC_MINOR__ >= 2))
#define LIBDBI_API_DEPRECATED __attribute__((__deprecated__))
#else
#define LIBDBI_API_DEPRECATED
#endif

/* opaque type definitions */
typedef void * dbi_driver;
typedef void * dbi_conn;
typedef void * dbi_result;

/* other type definitions */
typedef enum {
  DBI_ERROR_USER = -10, /* must be the first in the list */
  DBI_ERROR_DBD = -9,
  DBI_ERROR_BADOBJECT, 
  DBI_ERROR_BADTYPE,
  DBI_ERROR_BADIDX,
  DBI_ERROR_BADNAME,
  DBI_ERROR_UNSUPPORTED, 
  DBI_ERROR_NOCONN,
  DBI_ERROR_NOMEM,
  DBI_ERROR_BADPTR,
  DBI_ERROR_NONE = 0,
  DBI_ERROR_CLIENT
} dbi_error_flag;

/* some _MAX definitions. The size_t hack may not be portable */
#ifndef SIZE_T_MAX
#  define SIZE_T_MAX UINT_MAX
#endif
#ifndef ULLONG_MAX
#  define ULLONG_MAX ULONG_LONG_MAX
#endif

typedef struct {
	unsigned char month;
	unsigned char day;
	signed short year; // may be negative (B.C.)
} dbi_date;

typedef struct {
	// when used as an interval value, at most one of these values may be negative.
	// when used as a counter, the hour may be greater than 23.
	// when used as a time of day, everything is as you would expect.

	signed long hour;
	signed char minute;
	signed char second;
	signed short millisecond;
	signed long utc_offset; // seconds east of UTC
} dbi_time;

typedef struct {
	dbi_date date;
	dbi_time time;
} dbi_datetime;


/* function callback definitions */
typedef void (*dbi_conn_error_handler_func)(dbi_conn, void *);

/* definitions of the library interface versions */
#define LIBDBI_LIB_CURRENT 0
#define LIBDBI_LIB_REVISION 5
#define LIBDBI_LIB_AGE 0

/* values for the int in field_types[] */
#define DBI_TYPE_INTEGER 1
#define DBI_TYPE_DECIMAL 2
#define DBI_TYPE_STRING 3
#define DBI_TYPE_BINARY 4
#define DBI_TYPE_DATETIME 5

/* values for the bitmask in field_type_attributes[] */
#define DBI_INTEGER_UNSIGNED	(1 << 0)
#define DBI_INTEGER_SIZE1		(1 << 1)
#define DBI_INTEGER_SIZE2		(1 << 2)
#define DBI_INTEGER_SIZE3		(1 << 3)
#define DBI_INTEGER_SIZE4		(1 << 4)
#define DBI_INTEGER_SIZE8		(1 << 5)
#define DBI_INTEGER_SIZEMASK	(DBI_INTEGER_SIZE1|DBI_INTEGER_SIZE2 \
								|DBI_INTEGER_SIZE3|DBI_INTEGER_SIZE4 \
								|DBI_INTEGER_SIZE8) // isolate the size flags

#define DBI_DECIMAL_UNSIGNED	(1 << 0)
#define DBI_DECIMAL_SIZE4		(1 << 1)
#define DBI_DECIMAL_SIZE8		(1 << 2)
#define DBI_DECIMAL_SIZEMASK	(DBI_DECIMAL_SIZE4|DBI_DECIMAL_SIZE8)

#define DBI_STRING_FIXEDSIZE	(1 << 0) /* XXX unused as of now */

#define DBI_DATETIME_DATE		(1 << 0)
#define DBI_DATETIME_TIME		(1 << 1)

/* values for the bitmask in field_flags (unique to each row) */
#define DBI_VALUE_NULL			(1 << 0)


/* error code for type retrieval functions */
#define DBI_TYPE_ERROR 0

  /* error code for attribute retrieval functions */
#define DBI_ATTRIBUTE_ERROR   SHRT_MAX

/* functions with a return type of size_t return this in case of an
   error if 0 is a valid return value */
#define DBI_LENGTH_ERROR      SIZE_T_MAX

/* functions with a return type of unsigned long long return this in
   case of an error if 0 is a valid return value */
#define DBI_ROW_ERROR         ULLONG_MAX

/* functions with a return type of unsigned int return this in case of an error */
#define DBI_FIELD_ERROR       UINT_MAX

/* error code for field attribute retrieval functions */
#define DBI_FIELD_FLAG_ERROR  -1

  /* error code for bind* functions */
#define DBI_BIND_ERROR  -1

  /* needed by get_engine_version functions */
#define VERSIONSTRING_LENGTH 32

int dbi_initialize(const char *driverdir);
void dbi_shutdown();
const char *dbi_version();
int dbi_set_verbosity(int verbosity);

dbi_driver dbi_driver_list(dbi_driver Current); /* returns next driver. if current is NULL, return first driver. */
dbi_driver dbi_driver_open(const char *name); /* goes thru linked list until it finds the right one */
int dbi_driver_is_reserved_word(dbi_driver Driver, const char *word);
void *dbi_driver_specific_function(dbi_driver Driver, const char *name);
size_t LIBDBI_API_DEPRECATED dbi_driver_quote_string_copy(dbi_driver Driver, const char *orig, char **newstr);
size_t LIBDBI_API_DEPRECATED dbi_driver_quote_string(dbi_driver Driver, char **orig);
const char* dbi_driver_encoding_from_iana(dbi_driver Driver, const char* iana_encoding);
const char* dbi_driver_encoding_to_iana(dbi_driver Driver, const char* db_encoding);
int dbi_driver_cap_get(dbi_driver Driver, const char *capname);

const char *dbi_driver_get_name(dbi_driver Driver);
const char *dbi_driver_get_filename(dbi_driver Driver);
const char *dbi_driver_get_description(dbi_driver Driver);
const char *dbi_driver_get_maintainer(dbi_driver Driver);
const char *dbi_driver_get_url(dbi_driver Driver);
const char *dbi_driver_get_version(dbi_driver Driver);
const char *dbi_driver_get_date_compiled(dbi_driver Driver);

dbi_conn dbi_conn_new(const char *name); /* shortcut for dbi_conn_open(dbi_driver_open("foo")) */
dbi_conn dbi_conn_open(dbi_driver Driver); /* returns an actual instance of the conn */
dbi_driver dbi_conn_get_driver(dbi_conn Conn);
int dbi_conn_set_option(dbi_conn Conn, const char *key, const char *value); /* if value is NULL, remove option from list */
int dbi_conn_set_option_numeric(dbi_conn Conn, const char *key, int value);
const char *dbi_conn_get_option(dbi_conn Conn, const char *key);
int dbi_conn_get_option_numeric(dbi_conn Conn, const char *key);
const char *dbi_conn_require_option(dbi_conn Conn, const char *key); /* like get, but generate an error if key isn't found */
int dbi_conn_require_option_numeric(dbi_conn Conn, const char *key); /* ditto */
const char *dbi_conn_get_option_list(dbi_conn Conn, const char *current); /* returns key of next option, or the first option key if current is NULL */
void dbi_conn_clear_option(dbi_conn Conn, const char *key);
void dbi_conn_clear_options(dbi_conn Conn);
int dbi_conn_cap_get(dbi_conn Conn, const char *capname);
int dbi_conn_disjoin_results(dbi_conn Conn);
void dbi_conn_close(dbi_conn Conn);

int dbi_conn_error(dbi_conn Conn, const char **errmsg_dest);
void dbi_conn_error_handler(dbi_conn Conn, dbi_conn_error_handler_func function, void *user_argument);
dbi_error_flag LIBDBI_API_DEPRECATED dbi_conn_error_flag(dbi_conn Conn);
int dbi_conn_set_error(dbi_conn Conn, int errnum, const char *formatstr, ...);

int dbi_conn_connect(dbi_conn Conn);
int dbi_conn_get_socket(dbi_conn Conn);
unsigned int dbi_conn_get_engine_version(dbi_conn Conn);
char *dbi_conn_get_engine_version_string(dbi_conn Conn, char *versionstring);
const char *dbi_conn_get_encoding(dbi_conn Conn);
dbi_result dbi_conn_get_db_list(dbi_conn Conn, const char *pattern);
dbi_result dbi_conn_get_table_list(dbi_conn Conn, const char *db, const char *pattern);
dbi_result dbi_conn_query(dbi_conn Conn, const char *statement); 
dbi_result dbi_conn_queryf(dbi_conn Conn, const char *formatstr, ...);
dbi_result dbi_conn_query_null(dbi_conn Conn, const unsigned char *statement, size_t st_length); 
int dbi_conn_select_db(dbi_conn Conn, const char *db);
unsigned long long dbi_conn_sequence_last(dbi_conn Conn, const char *name); /* name of the sequence or table */
unsigned long long dbi_conn_sequence_next(dbi_conn Conn, const char *name);
int dbi_conn_ping(dbi_conn Conn);
size_t dbi_conn_quote_string_copy(dbi_conn Conn, const char *orig, char **newstr);
size_t dbi_conn_quote_string(dbi_conn Conn, char **orig);
size_t dbi_conn_quote_binary_copy(dbi_conn Conn, const unsigned char *orig, size_t from_length, unsigned char **newstr);
size_t dbi_conn_escape_string_copy(dbi_conn Conn, const char *orig, char **newstr);
size_t dbi_conn_escape_string(dbi_conn Conn, char **orig);
size_t dbi_conn_escape_binary_copy(dbi_conn Conn, const unsigned char *orig, size_t from_length, unsigned char **newstr);

dbi_conn dbi_result_get_conn(dbi_result Result);
int dbi_result_free(dbi_result Result);
int dbi_result_seek_row(dbi_result Result, unsigned long long rowidx);
int dbi_result_first_row(dbi_result Result);
int dbi_result_last_row(dbi_result Result);
int dbi_result_has_prev_row(dbi_result Result);
int dbi_result_prev_row(dbi_result Result);
int dbi_result_has_next_row(dbi_result Result);
int dbi_result_next_row(dbi_result Result);
unsigned long long dbi_result_get_currow(dbi_result Result);
unsigned long long dbi_result_get_numrows(dbi_result Result);
unsigned long long dbi_result_get_numrows_affected(dbi_result Result);
size_t LIBDBI_API_DEPRECATED dbi_result_get_field_size(dbi_result Result, const char *fieldname);
size_t LIBDBI_API_DEPRECATED dbi_result_get_field_size_idx(dbi_result Result, unsigned int fieldidx);
size_t dbi_result_get_field_length(dbi_result Result, const char *fieldname);
size_t dbi_result_get_field_length_idx(dbi_result Result, unsigned int fieldidx);
unsigned int dbi_result_get_field_idx(dbi_result Result, const char *fieldname);
const char *dbi_result_get_field_name(dbi_result Result, unsigned int fieldidx);
unsigned int dbi_result_get_numfields(dbi_result Result);
unsigned short dbi_result_get_field_type(dbi_result Result, const char *fieldname);
unsigned short dbi_result_get_field_type_idx(dbi_result Result, unsigned int fieldidx);
unsigned int dbi_result_get_field_attrib(dbi_result Result, const char *fieldname, unsigned int attribmin, unsigned int attribmax);
unsigned int dbi_result_get_field_attrib_idx(dbi_result Result, unsigned int fieldidx, unsigned int attribmin, unsigned int attribmax);
unsigned int dbi_result_get_field_attribs(dbi_result Result, const char *fieldname);
unsigned int dbi_result_get_field_attribs_idx(dbi_result Result, unsigned int fieldidx);
int dbi_result_field_is_null(dbi_result Result, const char *fieldname);
int dbi_result_field_is_null_idx(dbi_result Result, unsigned int fieldidx);
int dbi_result_disjoin(dbi_result Result);

unsigned int dbi_result_get_fields(dbi_result Result, const char *format, ...);
unsigned int dbi_result_bind_fields(dbi_result Result, const char *format, ...);

signed char dbi_result_get_char(dbi_result Result, const char *fieldname);
unsigned char dbi_result_get_uchar(dbi_result Result, const char *fieldname);
short dbi_result_get_short(dbi_result Result, const char *fieldname);
unsigned short dbi_result_get_ushort(dbi_result Result, const char *fieldname);
int dbi_result_get_int(dbi_result Result, const char *fieldname);
unsigned int dbi_result_get_uint(dbi_result Result, const char *fieldname);
int LIBDBI_API_DEPRECATED dbi_result_get_long(dbi_result Result, const char *fieldname); /* deprecated */
unsigned int LIBDBI_API_DEPRECATED dbi_result_get_ulong(dbi_result Result, const char *fieldname); /* deprecated */
long long dbi_result_get_longlong(dbi_result Result, const char *fieldname);
unsigned long long dbi_result_get_ulonglong(dbi_result Result, const char *fieldname);

float dbi_result_get_float(dbi_result Result, const char *fieldname);
double dbi_result_get_double(dbi_result Result, const char *fieldname);

const char *dbi_result_get_string(dbi_result Result, const char *fieldname);
const unsigned char *dbi_result_get_binary(dbi_result Result, const char *fieldname);

char *dbi_result_get_string_copy(dbi_result Result, const char *fieldname);
unsigned char *dbi_result_get_binary_copy(dbi_result Result, const char *fieldname);

time_t dbi_result_get_datetime(dbi_result Result, const char *fieldname);

int dbi_result_bind_char(dbi_result Result, const char *fieldname, char *bindto);
int dbi_result_bind_uchar(dbi_result Result, const char *fieldname, unsigned char *bindto);
int dbi_result_bind_short(dbi_result Result, const char *fieldname, short *bindto);
int dbi_result_bind_ushort(dbi_result Result, const char *fieldname, unsigned short *bindto);
int LIBDBI_API_DEPRECATED dbi_result_bind_long(dbi_result Result, const char *fieldname, int *bindto);
int LIBDBI_API_DEPRECATED dbi_result_bind_ulong(dbi_result Result, const char *fieldname, unsigned int *bindto);
int dbi_result_bind_int(dbi_result Result, const char *fieldname, int *bindto);
int dbi_result_bind_uint(dbi_result Result, const char *fieldname, unsigned int *bindto);
int dbi_result_bind_longlong(dbi_result Result, const char *fieldname, long long *bindto);
int dbi_result_bind_ulonglong(dbi_result Result, const char *fieldname, unsigned long long *bindto);

int dbi_result_bind_float(dbi_result Result, const char *fieldname, float *bindto);
int dbi_result_bind_double(dbi_result Result, const char *fieldname, double *bindto);

int dbi_result_bind_string(dbi_result Result, const char *fieldname, const char **bindto);
int dbi_result_bind_binary(dbi_result Result, const char *fieldname, const unsigned char **bindto);

int dbi_result_bind_string_copy(dbi_result Result, const char *fieldname, char **bindto);
int dbi_result_bind_binary_copy(dbi_result Result, const char *fieldname, unsigned char **bindto);

int dbi_result_bind_datetime(dbi_result Result, const char *fieldname, time_t *bindto);

/* and now for the same exact thing in index form: */

signed char dbi_result_get_char_idx(dbi_result Result, unsigned int fieldidx);
unsigned char dbi_result_get_uchar_idx(dbi_result Result, unsigned int fieldidx);
short dbi_result_get_short_idx(dbi_result Result, unsigned int fieldidx);
unsigned short dbi_result_get_ushort_idx(dbi_result Result, unsigned int fieldidx);
int LIBDBI_API_DEPRECATED dbi_result_get_long_idx(dbi_result Result, unsigned int fieldidx);
int dbi_result_get_int_idx(dbi_result Result, unsigned int fieldidx);
unsigned int dbi_result_get_uint_idx(dbi_result Result, unsigned int fieldidx);
unsigned int LIBDBI_API_DEPRECATED dbi_result_get_ulong_idx(dbi_result Result, unsigned int fieldidx);
long long dbi_result_get_longlong_idx(dbi_result Result, unsigned int fieldidx);
unsigned long long dbi_result_get_ulonglong_idx(dbi_result Result, unsigned int fieldidx);

float dbi_result_get_float_idx(dbi_result Result, unsigned int fieldidx);
double dbi_result_get_double_idx(dbi_result Result, unsigned int fieldidx);

const char *dbi_result_get_string_idx(dbi_result Result, unsigned int fieldidx);
const unsigned char *dbi_result_get_binary_idx(dbi_result Result, unsigned int fieldidx);

char *dbi_result_get_string_copy_idx(dbi_result Result, unsigned int fieldidx);
unsigned char *dbi_result_get_binary_copy_idx(dbi_result Result, unsigned int fieldidx);

time_t dbi_result_get_datetime_idx(dbi_result Result, unsigned int fieldidx);

/*
int dbi_result_bind_char_idx(dbi_result Result, unsigned int fieldidx, char *bindto);
int dbi_result_bind_uchar_idx(dbi_result Result, unsigned int fieldidx, unsigned char *bindto);
int dbi_result_bind_short_idx(dbi_result Result, unsigned int fieldidx, short *bindto);
int dbi_result_bind_ushort_idx(dbi_result Result, unsigned int fieldidx, unsigned short *bindto);
int dbi_result_bind_long_idx(dbi_result Result, unsigned int fieldidx, long *bindto);
int dbi_result_bind_ulong_idx(dbi_result Result, unsigned int fieldidx, unsigned long *bindto);
int dbi_result_bind_longlong_idx(dbi_result Result, unsigned int fieldidx, long long *bindto);
int dbi_result_bind_ulonglong_idx(dbi_result Result, unsigned int fieldidx, unsigned long long *bindto);

int dbi_result_bind_float_idx(dbi_result Result, unsigned int fieldidx, float *bindto);
int dbi_result_bind_double_idx(dbi_result Result, unsigned int fieldidx, double *bindto);

int dbi_result_bind_string_idx(dbi_result Result, unsigned int fieldidx, const char **bindto);
int dbi_result_bind_binary_idx(dbi_result Result, unsigned int fieldidx, const unsigned char **bindto);

int dbi_result_bind_string_copy_idx(dbi_result Result, unsigned int fieldidx, char **bindto);
int dbi_result_bind_binary_copy_idx(dbi_result Result, unsigned int fieldidx, unsigned char **bindto);

int dbi_result_bind_datetime_idx(dbi_result Result, unsigned int fieldidx, time_t *bindto);
*/

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif	/* __DBI_H__ */
