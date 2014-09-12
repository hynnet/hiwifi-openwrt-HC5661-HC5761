/*
 *  Turbo Wireless board info control haed file
 *
 *  Copyright (C) 2012-2012 duke
 *
 */

#ifndef __BOARD_INFO_H__
#define __BOARD_INFO_H__

#define VERSION_LEN			4
#define RSA_PUB_KEY_LEN		252
#define ENC_DES_KEY_LEN		128
#define TXT_FAC_INFO_LEN	2048
#define RESERVED1_LEN		5120
#define ENC_INFO_LEN		8192
#define RESERVED2_LEN		40960
#define MD5_SUM_LEN			640


typedef struct _BOARD_INFO {
	char	info_version[VERSION_LEN];
	char	rsa_pub_key[RSA_PUB_KEY_LEN];
	char	enc_des_key[ENC_DES_KEY_LEN];
	char	txt_fac_info[TXT_FAC_INFO_LEN];
	char	reserved1[RESERVED1_LEN];
	char	enc_fac_info[ENC_INFO_LEN];
	char	enc_srv_info[ENC_INFO_LEN];
	char	reserved2[RESERVED2_LEN];
	char	md5_sum[MD5_SUM_LEN];
} BOARD_INFO;

#define BDINFO_START_ADDR			0x10000
#define	BDINFO_END_MAGIC			"BDINFO_END"
#define BDINFO_LINE_BUF_LEN			(256+1)
#define BDINFO_ENTRY_NAME_LEN		(64+1)
#define BDINFO_ENTRY_VAL_LEN		(128+1)

int bdinfo_del_entry(const char* name);
int bdinfo_get_value(const char* name, char* value);
int bdinfo_get_value_n(const char* name, char* value, int val_len);
int bdinfo_set_value(const char* name, const char* value);
int bdinfo_version();

#endif //__BOARD_INFO_H__
