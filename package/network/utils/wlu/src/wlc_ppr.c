/*
 *  PHY module Power-per-rate API. Provides interface functions and definitions for
 * ppr structure for use containing regulatory and board limits and tx power targets.
 *
 * Copyright (C) 2012, Broadcom Corporation
 * All Rights Reserved.
 * 
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom Corporation.
 *
 * $Id: $
 */

#if defined(__NetBSD__) || defined(__FreeBSD__)
#if defined(_KERNEL)
#include <wlc_cfg.h>
#endif /* defined(_KERNEL) */
#endif /* defined(__NetBSD__) || defined(__FreeBSD__) */

#ifdef PPR_API
#include <typedefs.h>
#include <wlc_ppr.h>
#include <bcmendian.h>

#ifndef BCMDRIVER

#ifndef WL_BEAMFORMING
#define WL_BEAMFORMING /* enable TxBF definitions for utility code */
#endif

#ifndef bcopy
#include <string.h>
#include <stdlib.h>
#define	bcopy(src, dst, len)	memcpy((dst), (src), (len))
#endif

#ifndef ASSERT
#define ASSERT(exp)	do {} while (0)
#endif
#endif /* BCMDRIVER */

/* This marks the start of a packed structure section. */
#include <packed_section_start.h>

#define PPR_SERIALIZATION_VER 1

/* ppr deserialization header */
typedef BWL_PRE_PACKED_STRUCT struct ppr_deser_header {
	uint8  version;
	uint8  bw;
	uint16 reserved;
	uint32 flags;
} BWL_POST_PACKED_STRUCT ppr_deser_header_t;

typedef BWL_PRE_PACKED_STRUCT struct ppr_ser_mem_flag {
	uint32 magic_word;
	uint32 flag;
} BWL_POST_PACKED_STRUCT ppr_ser_mem_flag_t;

/* Flag bits in serialization/deserialization */
#define PPR_MAX_TX_CHAIN_MASK 0x00000003	/* mask of Tx chains */
#define PPR_BEAMFORMING      0x00000004		/* bit indicates BF is on */
#define PPR_SER_MEM_WORD     0xBEEFC0FF		/* magic word indicates serialization start */

/* size of serialization header */
#define SER_HDR_LEN    sizeof(ppr_deser_header_t)

/* Per band tx powers */
typedef BWL_PRE_PACKED_STRUCT struct pprpb {
	/* start of 20MHz tx power limits */
	int8 p_1x1dsss[WL_RATESET_SZ_DSSS];			/* Legacy CCK/DSSS */
	int8 p_1x1ofdm[WL_RATESET_SZ_OFDM]; 		/* 20 MHz Legacy OFDM transmission */
	int8 p_1x1vhtss1[WL_RATESET_SZ_VHT_MCS];	/* 8HT/10VHT pwrs starting at 1x1mcs0 */
#if (PPR_MAX_TX_CHAINS > 1)
	int8 p_1x2dsss[WL_RATESET_SZ_DSSS];			/* Legacy CCK/DSSS */
	int8 p_1x2cdd_ofdm[WL_RATESET_SZ_OFDM];		/* 20 MHz Legacy OFDM CDD transmission */
	int8 p_1x2cdd_vhtss1[WL_RATESET_SZ_VHT_MCS]; /* 8HT/10VHT pwrs starting at 1x2cdd_mcs0 */
	int8 p_2x2stbc_vhtss1[WL_RATESET_SZ_VHT_MCS]; /* 8HT/10VHT pwrs starting at 2x2stbc_mcs0 */
	int8 p_2x2vhtss2[WL_RATESET_SZ_VHT_MCS];	/* 8HT/10VHT pwrs starting at 2x2sdm_mcs8 */
#if (PPR_MAX_TX_CHAINS > 2)
	int8 p_1x3dsss[WL_RATESET_SZ_DSSS];			/* Legacy CCK/DSSS */
	int8 p_1x3cdd_ofdm[WL_RATESET_SZ_OFDM];		/* 20 MHz Legacy OFDM CDD transmission */
	int8 p_1x3cdd_vhtss1[WL_RATESET_SZ_VHT_MCS]; /* 8HT/10VHT pwrs starting at 1x3cdd_mcs0 */
	int8 p_2x3stbc_vhtss1[WL_RATESET_SZ_VHT_MCS]; /* 8HT/10VHT pwrs starting at 2x3stbc_mcs0 */
	int8 p_2x3vhtss2[WL_RATESET_SZ_VHT_MCS]; /* 8HT/10VHT pwrs starting at 2x3sdm_mcs8 spexp1 */
	int8 p_3x3vhtss3[WL_RATESET_SZ_VHT_MCS]; /* 8HT/10VHT pwrs starting at 3x3sdm_mcs16 */
#endif

#ifdef WL_BEAMFORMING
	int8 p_1x2txbf_ofdm[WL_RATESET_SZ_OFDM];	/* 20 MHz Legacy OFDM TXBF transmission */
	int8 p_1x2txbf_vhtss1[WL_RATESET_SZ_VHT_MCS]; /* 8HT/10VHT pwrs starting at 1x2txbf_mcs0 */
	int8 p_2x2txbf_vhtss2[WL_RATESET_SZ_VHT_MCS]; /* 8HT/10VHT pwrs starting at 2x2txbf_mcs8 */
#if (PPR_MAX_TX_CHAINS > 2)
	int8 p_1x3txbf_ofdm[WL_RATESET_SZ_OFDM];	/* 20 MHz Legacy OFDM TXBF transmission */
	int8 p_1x3txbf_vhtss1[WL_RATESET_SZ_VHT_MCS]; /* 8HT/10VHT pwrs starting at 1x3txbf_mcs0 */
	int8 p_2x3txbf_vhtss2[WL_RATESET_SZ_VHT_MCS]; /* 8HT/10VHT pwrs starting at 2x3txbf_mcs8 */
	int8 p_3x3txbf_vhtss3[WL_RATESET_SZ_VHT_MCS]; /* 8HT/10VHT pwrs starting at 3x3txbf_mcs16 */
#endif
#endif /* WL_BEAMFORMING */
#endif /* PPR_MAX_TX_CHAINS > 1 */
} BWL_POST_PACKED_STRUCT pprpbw_t;

#define PPR_CHAIN1_FIRST OFFSETOF(pprpbw_t, p_1x1dsss)
#define PPR_CHAIN1_END   (OFFSETOF(pprpbw_t, p_1x1vhtss1) + sizeof(((pprpbw_t *)0)->p_1x1vhtss1))
#define PPR_CHAIN1_SIZE  PPR_CHAIN1_END
#if (PPR_MAX_TX_CHAINS > 1)
#define PPR_CHAIN2_FIRST OFFSETOF(pprpbw_t, p_1x2dsss)
#define PPR_CHAIN2_END   (OFFSETOF(pprpbw_t, p_2x2vhtss2) + sizeof(((pprpbw_t *)0)->p_2x2vhtss2))
#define PPR_CHAIN2_SIZE  (PPR_CHAIN2_END - PPR_CHAIN2_FIRST)
#if (PPR_MAX_TX_CHAINS > 2)
#define PPR_CHAIN3_FIRST OFFSETOF(pprpbw_t, p_1x3dsss)
#define PPR_CHAIN3_END   (OFFSETOF(pprpbw_t, p_3x3vhtss3) + sizeof(((pprpbw_t *)0)->p_3x3vhtss3))
#define PPR_CHAIN3_SIZE  (PPR_CHAIN3_END - PPR_CHAIN3_FIRST)
#endif

#ifdef WL_BEAMFORMING
#define PPR_BF_CHAIN2_FIRST OFFSETOF(pprpbw_t, p_1x2txbf_ofdm)
#define PPR_BF_CHAIN2_END   (OFFSETOF(pprpbw_t, p_2x2txbf_vhtss2) + \
	sizeof(((pprpbw_t *)0)->p_2x2txbf_vhtss2))
#define PPR_BF_CHAIN2_SIZE  (PPR_BF_CHAIN2_END - PPR_BF_CHAIN2_FIRST)
#if (PPR_MAX_TX_CHAINS > 2)
#define PPR_BF_CHAIN3_FIRST OFFSETOF(pprpbw_t, p_1x3txbf_ofdm)
#define PPR_BF_CHAIN3_END   (OFFSETOF(pprpbw_t, p_3x3txbf_vhtss3) + \
	sizeof(((pprpbw_t *)0)->p_3x3txbf_vhtss3))
#define PPR_BF_CHAIN3_SIZE  (PPR_BF_CHAIN3_END - PPR_BF_CHAIN3_FIRST)
#endif

#endif /* WL_BEAMFORMING */
#endif /* PPR_MAX_TX_CHAINS > 1 */

#define PPR_BW_MAX WL_TX_BW_80 /* maximum supported bandwidth */

/* Structure to contain ppr values for all bandwidths, e.g. for board limits */
typedef BWL_PRE_PACKED_STRUCT struct ppr_bw_all {
	/* 20MHz tx power limits */
	pprpbw_t b20;
	/* 40MHz tx power limits */
	pprpbw_t b40;
	/* 20in40MHz tx power limits */
	pprpbw_t b20in40;
	/* 80MHz tx power limits */
	pprpbw_t b80;
	/* 20in80MHz tx power limits */
	pprpbw_t b20in80;
	/* 40in80MHz tx power limits */
	pprpbw_t b40in80;
} BWL_POST_PACKED_STRUCT ppr_bw_all_t;


/* Structure to contain ppr values for a 20MHz channel */
typedef BWL_PRE_PACKED_STRUCT struct ppr_bw_20 {
	/* 20MHz tx power limits */
	pprpbw_t b20;
} BWL_POST_PACKED_STRUCT ppr_bw_20_t;


/* Structure to contain ppr values for a 40MHz channel */
typedef BWL_PRE_PACKED_STRUCT struct ppr_bw_40 {
	/* 40MHz tx power limits */
	pprpbw_t b40;
	/* 20in40MHz tx power limits */
	pprpbw_t b20in40;
} BWL_POST_PACKED_STRUCT ppr_bw_40_t;

/* Structure to contain ppr values for an 80MHz channel */
typedef BWL_PRE_PACKED_STRUCT struct ppr_bw_80 {
	/* 80MHz tx power limits */
	pprpbw_t b80;
	/* 20in80MHz tx power limits */
	pprpbw_t b20in80;
	/* 40in80MHz tx power limits */
	pprpbw_t b40in80;
} BWL_POST_PACKED_STRUCT ppr_bw_80_t;

/*
 * This is the initial implementation of the structure we're hiding. It is sized to contain only
 * the set of powers it requires, so the union is not necessarily the size of the largest member.
 */

BWL_PRE_PACKED_STRUCT struct ppr {
	wl_tx_bw_t ch_bw;

	BWL_PRE_PACKED_STRUCT union {
		ppr_bw_all_t all;
		ppr_bw_20_t ch20;
		ppr_bw_40_t ch40;
		ppr_bw_80_t ch80;
	} ppr_bw;
} BWL_POST_PACKED_STRUCT;

/* This marks the end of a packed structure section. */
#include <packed_section_end.h>

/* Returns a flag of ppr conditions (chains, txbf etc.) */
static uint32 ppr_get_flag(void)
{
	uint32 flag = 0;
	flag  |= PPR_MAX_TX_CHAINS & PPR_MAX_TX_CHAIN_MASK;
#ifdef WL_BEAMFORMING
	flag  |= PPR_BEAMFORMING;
#endif
	return flag;
}

#define COPY_PPR_TOBUF(x, y) do { bcopy(&pprbuf[x], *buf, y); \
	*buf += y; ret += y; } while (0);

/* Serialize ppr data of a bandwidth into the given buffer */
static uint ppr_serialize_block(const uint8 *pprbuf, uint8** buf, uint32 serflag)
{
	uint ret = 0;
#if (PPR_MAX_TX_CHAINS > 1)
	uint chain   = serflag & PPR_MAX_TX_CHAIN_MASK; /* chain number in serialized block */
	bool bf      = (serflag & PPR_BEAMFORMING) != 0;
#endif
	bcopy(pprbuf, *buf, PPR_CHAIN1_SIZE);
	*buf += PPR_CHAIN1_SIZE;
	ret  += PPR_CHAIN1_SIZE;
#if (PPR_MAX_TX_CHAINS > 1)
	BCM_REFERENCE(bf);
	if (chain > 1) {
		COPY_PPR_TOBUF(PPR_CHAIN2_FIRST, PPR_CHAIN2_SIZE);
	}
#if (PPR_MAX_TX_CHAINS > 2)
	if (chain > 2) {
		COPY_PPR_TOBUF(PPR_CHAIN3_FIRST, PPR_CHAIN3_SIZE);
	}
#endif
#ifdef WL_BEAMFORMING
	if (bf) {
		COPY_PPR_TOBUF(PPR_BF_CHAIN2_FIRST, PPR_BF_CHAIN2_SIZE);
	}
#if (PPR_MAX_TX_CHAINS > 2)
	if (bf && chain > 2) {
		COPY_PPR_TOBUF(PPR_BF_CHAIN3_FIRST, PPR_BF_CHAIN3_SIZE);
	}
#endif
#endif /* WL_BEAMFORMING */
#endif /* (PPR_MAX_TX_CHAINS > 1) */
	return ret;
}

/* Serialize ppr data of each bandwidth into the given buffer, returns bytes copied */
static uint ppr_serialize_data(const ppr_t *pprptr, uint8* buf, uint32 serflag)
{
	uint ret = sizeof(ppr_deser_header_t);
	ppr_deser_header_t* header = (ppr_deser_header_t*)buf;
	ASSERT(pprptr && buf);
	header->version = PPR_SERIALIZATION_VER;
	header->bw      = (uint8)pprptr->ch_bw;
	header->flags   = HTON32(ppr_get_flag());

	buf += sizeof(*header);
	switch (header->bw) {
	case WL_TX_BW_20:
		{
			const uint8* pprbuf = (const uint8*)&pprptr->ppr_bw.ch20.b20;
			ret += ppr_serialize_block(pprbuf, &buf, serflag);
		}
		break;
	case WL_TX_BW_40:
		{
			const uint8* pprbuf = (const uint8*)&pprptr->ppr_bw.ch40.b40;
			ret += ppr_serialize_block(pprbuf, &buf, serflag);
			pprbuf = (const uint8 *) &pprptr->ppr_bw.ch40.b20in40;
			ret += ppr_serialize_block(pprbuf, &buf, serflag);
		}
		break;
	case WL_TX_BW_80:
		{
			const uint8* pprbuf = (const uint8*)&pprptr->ppr_bw.ch80.b80;
			ret += ppr_serialize_block(pprbuf, &buf, serflag);
			pprbuf = (const uint8*)&pprptr->ppr_bw.ch80.b20in80;
			ret += ppr_serialize_block(pprbuf, &buf, serflag);
			pprbuf = (const uint8*)&pprptr->ppr_bw.ch80.b40in80;
			ret += ppr_serialize_block(pprbuf, &buf, serflag);
		}
		break;
	case WL_TX_BW_ALL:
		{
			const uint8* pprbuf = (const uint8*)&pprptr->ppr_bw.all.b20;
			ret += ppr_serialize_block(pprbuf, &buf, serflag);
			pprbuf = (const uint8*)&pprptr->ppr_bw.all.b40;
			ret += ppr_serialize_block(pprbuf, &buf, serflag);
			pprbuf = (const uint8*)&pprptr->ppr_bw.all.b20in40;
			ret += ppr_serialize_block(pprbuf, &buf, serflag);
			pprbuf = (const uint8*)&pprptr->ppr_bw.all.b80;
			ret += ppr_serialize_block(pprbuf, &buf, serflag);
			pprbuf = (const uint8*)&pprptr->ppr_bw.all.b20in80;
			ret += ppr_serialize_block(pprbuf, &buf, serflag);
			pprbuf = (const uint8*)&pprptr->ppr_bw.all.b40in80;
			ret += ppr_serialize_block(pprbuf, &buf, serflag);
		}
		break;
	default:
		ASSERT(0);
	}
	return ret;
}

/* Copy serialized ppr data of a bandwidth */
static void ppr_copy_serdata(uint8* pobuf, const uint8** inbuf, uint32 flag)
{
	uint chain   = flag & PPR_MAX_TX_CHAIN_MASK;
	bool bf      = (flag & PPR_BEAMFORMING) != 0;
	BCM_REFERENCE(chain);
	BCM_REFERENCE(bf);
	bcopy(*inbuf, pobuf, PPR_CHAIN1_SIZE);
	*inbuf += PPR_CHAIN1_SIZE;
#if (PPR_MAX_TX_CHAINS > 1)
	if (chain > 1) {
		bcopy(*inbuf, &pobuf[PPR_CHAIN2_FIRST], PPR_CHAIN2_SIZE);
		*inbuf += PPR_CHAIN2_SIZE;
	}
#if (PPR_MAX_TX_CHAINS > 2)
	if (chain > 2) {
		bcopy(*inbuf, &pobuf[PPR_CHAIN3_FIRST], PPR_CHAIN3_SIZE);
		*inbuf += PPR_CHAIN3_SIZE;
	}
#endif

#ifdef WL_BEAMFORMING
	if (bf) {
		bcopy(*inbuf, &pobuf[PPR_BF_CHAIN2_FIRST], PPR_BF_CHAIN2_SIZE);
		*inbuf += PPR_BF_CHAIN2_SIZE;
	}
#if (PPR_MAX_TX_CHAINS > 2)
	if (bf && chain > 2) {
		bcopy(*inbuf, &pobuf[PPR_BF_CHAIN3_FIRST], PPR_BF_CHAIN3_SIZE);
		*inbuf += PPR_BF_CHAIN3_SIZE;
	}
#endif
#endif  /* WL_BEAMFORMING */
#endif /* (PPR_MAX_TX_CHAINS > 1) */
}

/* Deserialize data into a ppr_t structure */
static void ppr_deser_cpy(ppr_t* pptr, const uint8* inbuf, uint32 flag, wl_tx_bw_t bw)
{
	pptr->ch_bw = bw;
	switch (bw) {
	case WL_TX_BW_20:
		{
			uint8* pobuf = (uint8*)&pptr->ppr_bw.ch20;
			ppr_copy_serdata(pobuf, &inbuf, flag);
		}
		break;
	case WL_TX_BW_40:
		{
			uint8* pobuf = (uint8*)&pptr->ppr_bw.ch40.b40;
			ppr_copy_serdata(pobuf, &inbuf, flag);
			pobuf = (uint8*)&pptr->ppr_bw.ch40.b20in40;
			ppr_copy_serdata(pobuf, &inbuf, flag);
		}
		break;
	case WL_TX_BW_80:
		{
			uint8* pobuf = (uint8*)&pptr->ppr_bw.ch80.b80;
			ppr_copy_serdata(pobuf, &inbuf, flag);
			pobuf = (uint8*)&pptr->ppr_bw.ch80.b20in80;
			ppr_copy_serdata(pobuf, &inbuf, flag);
			pobuf = (uint8*)&pptr->ppr_bw.ch80.b40in80;
			ppr_copy_serdata(pobuf, &inbuf, flag);
		}
		break;
	case WL_TX_BW_ALL:
		{
			uint8* pobuf = (uint8*)&pptr->ppr_bw.all.b20;
			ppr_copy_serdata(pobuf, &inbuf, flag);
			pobuf = (uint8*)&pptr->ppr_bw.all.b40;
			ppr_copy_serdata(pobuf, &inbuf, flag);
			pobuf = (uint8*)&pptr->ppr_bw.all.b20in40;
			ppr_copy_serdata(pobuf, &inbuf, flag);
			pobuf = (uint8*)&pptr->ppr_bw.all.b80;
			ppr_copy_serdata(pobuf, &inbuf, flag);
			pobuf = (uint8*)&pptr->ppr_bw.all.b20in80;
			ppr_copy_serdata(pobuf, &inbuf, flag);
			pobuf = (uint8*)&pptr->ppr_bw.all.b40in80;
			ppr_copy_serdata(pobuf, &inbuf, flag);
		}
		break;
	default:
		ASSERT(0);
	}
}

/* Get a pointer to the power values for a given channel bandwidth */
static pprpbw_t* ppr_get_bw_powers_20(ppr_t* p, wl_tx_bw_t bw)
{
	pprpbw_t* pwrs = NULL;

	if (bw == WL_TX_BW_20)
		pwrs = &p->ppr_bw.ch20.b20;
	/* else */
	/*   ASSERT(0); */
	return pwrs;
}


/* Get a pointer to the power values for a given channel bandwidth */
static pprpbw_t* ppr_get_bw_powers_40(ppr_t* p, wl_tx_bw_t bw)
{
	pprpbw_t* pwrs = NULL;

	switch (bw) {
	case WL_TX_BW_40:
		pwrs = &p->ppr_bw.ch40.b40;
	break;
	case WL_TX_BW_20:

	case WL_TX_BW_20IN40:
		pwrs = &p->ppr_bw.ch40.b20in40;
	break;
	default:
		/* ASSERT(0); */
	break;
	}
	return pwrs;
}


/* Get a pointer to the power values for a given channel bandwidth */
static pprpbw_t* ppr_get_bw_powers_80(ppr_t* p, wl_tx_bw_t bw)
{
	pprpbw_t* pwrs = NULL;

	switch (bw) {
	case WL_TX_BW_80:
		pwrs = &p->ppr_bw.ch80.b80;
	break;
	case WL_TX_BW_20:
	case WL_TX_BW_20IN40:
	case WL_TX_BW_20IN80:
		pwrs = &p->ppr_bw.ch80.b20in80;
	break;
	case WL_TX_BW_40:
	case WL_TX_BW_40IN80:
		pwrs = &p->ppr_bw.ch80.b40in80;
	break;
	default:
		/* ASSERT(0); */
	break;
	}
	return pwrs;
}


/* Get a pointer to the power values for a given channel bandwidth */
static pprpbw_t* ppr_get_bw_powers_all(ppr_t* p, wl_tx_bw_t bw)
{
	pprpbw_t* pwrs = NULL;

	switch (bw) {
	case WL_TX_BW_20:
		pwrs = &p->ppr_bw.all.b20;
	break;
	case WL_TX_BW_40:
		pwrs = &p->ppr_bw.all.b40;
	break;
	case WL_TX_BW_80:
		pwrs = &p->ppr_bw.all.b80;
	break;
	case WL_TX_BW_20IN40:
		pwrs = &p->ppr_bw.all.b20in40;
	break;
	case WL_TX_BW_20IN80:
		pwrs = &p->ppr_bw.all.b20in80;
	break;
	case WL_TX_BW_40IN80:
		pwrs = &p->ppr_bw.all.b40in80;
	break;
	default:
		/* ASSERT(0); */
	break;
	}
	return pwrs;
}


typedef pprpbw_t* (*wlc_ppr_get_bw_pwrs_fn_t)(ppr_t* p, wl_tx_bw_t bw);

typedef struct {
	wl_tx_bw_t ch_bw;		/* Bandwidth of the channel for which powers are stored */
	/* Function to retrieve the powers for the requested bandwidth */
	wlc_ppr_get_bw_pwrs_fn_t fn;
} wlc_ppr_get_bw_pwrs_pair_t;


static const wlc_ppr_get_bw_pwrs_pair_t ppr_get_bw_pwrs_fn[] = {
	{WL_TX_BW_20, ppr_get_bw_powers_20},
	{WL_TX_BW_40, ppr_get_bw_powers_40},
	{WL_TX_BW_80, ppr_get_bw_powers_80},
	{WL_TX_BW_ALL, ppr_get_bw_powers_all}
};


/* Get a pointer to the power values for a given channel bandwidth */
static pprpbw_t* ppr_get_bw_powers(ppr_t* p, wl_tx_bw_t bw)
{
	uint32 i;

	for (i = 0; i < (int)ARRAYSIZE(ppr_get_bw_pwrs_fn); i++) {
		if (ppr_get_bw_pwrs_fn[i].ch_bw == p->ch_bw)
			return ppr_get_bw_pwrs_fn[i].fn(p, bw);
	}

	ASSERT(0);
	return NULL;
}


/*
 * Rate group power finder functions: ppr_get_xxx_group()
 * To preserve the opacity of the PPR struct, even inside the API we try to limit knowledge of
 * its details. Almost all API functions work on the powers for individual rate groups, rather than
 * directly accessing the struct. Once the section of the structure corresponding to the bandwidth
 * has been identified using ppr_get_bw_powers(), the ppr_get_xxx_group() functions use knowledge
 * of the number of spatial streams, the number of tx chains, and the expansion mode to return a
 * pointer to the required group of power values.
 */

/* Get a pointer to the power values for the given dsss rate group for a given channel bandwidth */
static int8* ppr_get_dsss_group(pprpbw_t* bw_pwrs, wl_tx_chains_t tx_chains)
{
	int8* group_pwrs = NULL;

	switch (tx_chains) {
#if (PPR_MAX_TX_CHAINS > 1)
#if (PPR_MAX_TX_CHAINS > 2)
	case WL_TX_CHAINS_3:
		group_pwrs = bw_pwrs->p_1x3dsss;
		break;
#endif
	case WL_TX_CHAINS_2:
		group_pwrs = bw_pwrs->p_1x2dsss;
		break;
#endif /* PPR_MAX_TX_CHAINS > 1 */
	case WL_TX_CHAINS_1:
		group_pwrs = bw_pwrs->p_1x1dsss;
		break;
	default:
		ASSERT(0);
		break;
	}
	return group_pwrs;
}


/* Get a pointer to the power values for the given ofdm rate group for a given channel bandwidth */
static int8* ppr_get_ofdm_group(pprpbw_t* bw_pwrs, wl_tx_mode_t mode,
	wl_tx_chains_t tx_chains)
{
	int8* group_pwrs = NULL;
	BCM_REFERENCE(mode);
	switch (tx_chains) {
#if (PPR_MAX_TX_CHAINS > 1)
#if (PPR_MAX_TX_CHAINS > 2)
	case WL_TX_CHAINS_3:
#ifdef WL_BEAMFORMING
		if (mode == WL_TX_MODE_TXBF)
			group_pwrs = bw_pwrs->p_1x3txbf_ofdm;
		else
#endif
			group_pwrs = bw_pwrs->p_1x3cdd_ofdm;
		break;
#endif /* PPR_MAX_TX_CHAINS > 2 */
	case WL_TX_CHAINS_2:
#ifdef WL_BEAMFORMING
		if (mode == WL_TX_MODE_TXBF)
			group_pwrs = bw_pwrs->p_1x2txbf_ofdm;
		else
#endif
			group_pwrs = bw_pwrs->p_1x2cdd_ofdm;
		break;
#endif /* PPR_MAX_TX_CHAINS > 1 */
	case WL_TX_CHAINS_1:
		group_pwrs = bw_pwrs->p_1x1ofdm;
		break;
	default:
		ASSERT(0);
		break;
	}
	return group_pwrs;
}


/*
 * Tables to provide access to HT/VHT rate group powers. This avoids an ugly nested switch with
 * messy conditional compilation.
 *
 * Access to a given table entry is via table[chains - Nss][mode], except for the Nss3 table, which
 * only has one row, so it can be indexed directly by table[mode].
 *
 * Separate tables are provided for each of Nss1, Nss2 and Nss3 because they are all different
 * sizes. A combined table would be very sparse, and this arrangement also simplifies the
 * conditional compilation.
 *
 * Each row represents a given number of chains, so there's no need for a zero row. Because
 * chains >= Nss is always true, there is no one-chain row for Nss2 and there are no one- or
 * two-chain rows for Nss3. With the tables correctly sized, we can index the rows
 * using [chains - Nss].
 *
 * Then, inside each row, we index by mode:
 * WL_TX_MODE_NONE, WL_TX_MODE_STBC, WL_TX_MODE_CDD, WL_TX_MODE_TXBF.
 */

#define OFFSNONE (-1)

#ifdef WL_BEAMFORMING
#define TXBFOFFS(x, y)	 OFFSETOF(x, y)
#else
#define TXBFOFFS(x, y)	 OFFSNONE
#endif


static const int mcs_groups_nss1[PPR_MAX_TX_CHAINS][WL_NUM_TX_MODES] = {
	/* WL_TX_MODE_NONE
	   WL_TX_MODE_STBC
	   WL_TX_MODE_CDD
	   WL_TX_MODE_TXBF
	*/
	/* 1 chain */
	{OFFSETOF(pprpbw_t, p_1x1vhtss1),
	OFFSNONE,
	OFFSNONE,
	OFFSNONE},
#if (PPR_MAX_TX_CHAINS > 1)
	/* 2 chain */
	{OFFSNONE,
	OFFSNONE,
	OFFSETOF(pprpbw_t, p_1x2cdd_vhtss1),
	TXBFOFFS(pprpbw_t, p_1x2txbf_vhtss1)},
#if (PPR_MAX_TX_CHAINS > 2)
	/* 3 chain */
	{OFFSNONE,
	OFFSNONE,
	OFFSETOF(pprpbw_t, p_1x3cdd_vhtss1),
	TXBFOFFS(pprpbw_t, p_1x3txbf_vhtss1)}
#endif
#endif /* PPR_MAX_TX_CHAINS > 1 */
};


#if (PPR_MAX_TX_CHAINS > 1)
static const int mcs_groups_nss2[PPR_MAX_TX_CHAINS - 1][WL_NUM_TX_MODES] = {
	/* 2 chain */
	{OFFSETOF(pprpbw_t, p_2x2vhtss2),
	OFFSETOF(pprpbw_t, p_2x2stbc_vhtss1),
	OFFSNONE,
	TXBFOFFS(pprpbw_t, p_2x2txbf_vhtss2)},
#if (PPR_MAX_TX_CHAINS > 2)
	/* 3 chain */
	{OFFSETOF(pprpbw_t, p_2x3vhtss2),
	OFFSETOF(pprpbw_t, p_2x3stbc_vhtss1),
	OFFSNONE,
	TXBFOFFS(pprpbw_t, p_2x3txbf_vhtss2)}
#endif
};


#if (PPR_MAX_TX_CHAINS > 2)
static const int mcs_groups_nss3[WL_NUM_TX_MODES] = {
/* 3 chains only */
	OFFSETOF(pprpbw_t, p_3x3vhtss3),
	OFFSNONE,
	OFFSNONE,
	TXBFOFFS(pprpbw_t, p_3x3txbf_vhtss3)
};
#endif

#endif /* (PPR_MAX_TX_CHAINS > 1) */


/* Get a pointer to the power values for the given rate group for a given channel bandwidth */
static int8* ppr_get_mcs_group(pprpbw_t* bw_pwrs, wl_tx_nss_t Nss, wl_tx_mode_t mode,
	wl_tx_chains_t tx_chains)
{
	int8* group_pwrs = NULL;
	int offset;

	switch (Nss) {
#if (PPR_MAX_TX_CHAINS > 1)
#if (PPR_MAX_TX_CHAINS > 2)
	case WL_TX_NSS_3:
		if (tx_chains == WL_TX_CHAINS_3) {
			offset = mcs_groups_nss3[mode];
			if (offset != OFFSNONE) {
				group_pwrs = (int8*)bw_pwrs + offset;
			}
		}
		else
			ASSERT(0);
		break;
#endif /* PPR_MAX_TX_CHAINS > 2 */
	case WL_TX_NSS_2:
		if ((tx_chains >= WL_TX_CHAINS_2) && (tx_chains <= PPR_MAX_TX_CHAINS)) {
			offset = mcs_groups_nss2[tx_chains - Nss][mode];
			if (offset != OFFSNONE) {
				group_pwrs = (int8*)bw_pwrs + offset;
			}
		}
		else
			ASSERT(0);
		break;
#endif /* PPR_MAX_TX_CHAINS > 1 */
	case WL_TX_NSS_1:
		if (tx_chains <= PPR_MAX_TX_CHAINS) {
			offset = mcs_groups_nss1[tx_chains - Nss][mode];
			if (offset != OFFSNONE) {
				group_pwrs = (int8*)bw_pwrs + offset;
			}
		}
		else
			ASSERT(0);
		break;
	default:
		ASSERT(0);
		break;
	}
	return group_pwrs;
}


/* Size routine for user alloc/dealloc */
static uint32 ppr_pwrs_size(wl_tx_bw_t bw)
{
	uint32 size;

	switch (bw) {
	case WL_TX_BW_20:
		size = sizeof(ppr_bw_20_t);
	break;
	case WL_TX_BW_40:
		size = sizeof(ppr_bw_40_t);
	break;
	case WL_TX_BW_80:
		size = sizeof(ppr_bw_80_t);
	break;
	default:
		ASSERT(0);
		/* fall through */
	case WL_TX_BW_ALL:
		size = sizeof(ppr_bw_all_t);
	break;
	}
	return size;
}


/* Initialization routine */
void ppr_init(ppr_t* pprptr, wl_tx_bw_t bw)
{
	memset(pprptr, (int8)WL_RATE_DISABLED, ppr_size(bw));
	pprptr->ch_bw = bw;
}


/* Reinitialization routine for opaque PPR struct */
void ppr_clear(ppr_t* pprptr)
{
	memset((uchar*)&pprptr->ppr_bw, (int8)WL_RATE_DISABLED, ppr_pwrs_size(pprptr->ch_bw));
}


/* Size routine for user alloc/dealloc */
uint32 ppr_size(wl_tx_bw_t bw)
{
	return ppr_pwrs_size(bw) + sizeof(wl_tx_bw_t);
}


/* Size routine for user serialization alloc */
uint32 ppr_ser_size(const ppr_t* pprptr)
{
	return ppr_pwrs_size(pprptr->ch_bw) + SER_HDR_LEN;	/* struct size plus headers */
}

/* Size routine for user serialization alloc */
uint32 ppr_ser_size_by_bw(wl_tx_bw_t bw)
{
	return ppr_pwrs_size(bw) + SER_HDR_LEN;	/* struct size plus headers */
}

/* Constructor routine for opaque PPR struct */
ppr_t* ppr_create(osl_t *osh, wl_tx_bw_t bw)
{
	ppr_t* pprptr;

	ASSERT((bw == WL_TX_BW_20) || (bw == WL_TX_BW_40) ||
		(bw == WL_TX_BW_80) ||
		(bw == WL_TX_BW_ALL));
#ifndef BCMDRIVER
	BCM_REFERENCE(osh);
	if ((pprptr = (ppr_t*)malloc((uint)ppr_size(bw))) != NULL) {
#else
	if ((pprptr = (ppr_t*)MALLOC(osh, (uint)ppr_size(bw))) != NULL) {
#endif
		ppr_init(pprptr, bw);
	}
	return pprptr;
}

/* Init flags in the memory block for serialization, the serializer will check
 * the flag to decide which ppr to be copied
 */
int ppr_init_ser_mem_by_bw(uint8* pbuf, wl_tx_bw_t bw, uint32 len)
{
	ppr_ser_mem_flag_t *pmflag;

	if (pbuf == NULL || ppr_ser_size_by_bw(bw) > len)
		return BCME_BADARG;

	pmflag = (ppr_ser_mem_flag_t *)pbuf;
	pmflag->magic_word = HTON32(PPR_SER_MEM_WORD);
	pmflag->flag   = HTON32(ppr_get_flag());

	/* init the memory */
	memset(pbuf + sizeof(*pmflag), (uint8)WL_RATE_DISABLED, len-sizeof(*pmflag));
	return BCME_OK;
}

int ppr_init_ser_mem(uint8* pbuf, ppr_t * ppr, uint32 len)
{
	return ppr_init_ser_mem_by_bw(pbuf, ppr->ch_bw, len);
}

/* Destructor routine for opaque PPR struct */
void ppr_delete(osl_t *osh, ppr_t* pprptr)
{
	ASSERT((pprptr->ch_bw == WL_TX_BW_20) || (pprptr->ch_bw == WL_TX_BW_40) ||
		(pprptr->ch_bw == WL_TX_BW_80) ||
		(pprptr->ch_bw == WL_TX_BW_ALL));
#ifndef BCMDRIVER
	BCM_REFERENCE(osh);
	free(pprptr);
#else
	MFREE(osh, pprptr, (uint)ppr_size(pprptr->ch_bw));
#endif
}


/* Type routine for inferring opaque structure size */
wl_tx_bw_t ppr_get_ch_bw(const ppr_t* pprptr)
{
	return pprptr->ch_bw;
}

/* Type routine to get ppr supported maximum bw */
wl_tx_bw_t ppr_get_max_bw(void)
{
	return PPR_BW_MAX;
}

/* Get the dsss values for the given number of tx_chains and 20, 20in40, etc. */
int ppr_get_dsss(ppr_t* pprptr, wl_tx_bw_t bw, wl_tx_chains_t tx_chains,
	ppr_dsss_rateset_t* dsss)
{
	pprpbw_t* bw_pwrs;
	const int8* powers;
	int cnt = 0;

	ASSERT(pprptr);
	bw_pwrs = ppr_get_bw_powers(pprptr, bw);
	if (bw_pwrs != NULL) {
		powers = ppr_get_dsss_group(bw_pwrs, tx_chains);
		if (powers != NULL) {
			bcopy(powers, dsss->pwr, sizeof(*dsss));
			cnt = sizeof(*dsss);
		}
	}
	if (cnt == 0) {
		memset(dsss->pwr, (int8)WL_RATE_DISABLED, sizeof(*dsss));
	}
	return cnt;
}


/* Get the ofdm values for the given number of tx_chains and 20, 20in40, etc. */
int ppr_get_ofdm(ppr_t* pprptr, wl_tx_bw_t bw, wl_tx_mode_t mode, wl_tx_chains_t tx_chains,
	ppr_ofdm_rateset_t* ofdm)
{
	pprpbw_t* bw_pwrs;
	const int8* powers;
	int cnt = 0;

	ASSERT(pprptr);
	bw_pwrs = ppr_get_bw_powers(pprptr, bw);
	if (bw_pwrs != NULL) {
		powers = ppr_get_ofdm_group(bw_pwrs, mode, tx_chains);
		if (powers != NULL) {
			bcopy(powers, ofdm->pwr, sizeof(*ofdm));
			cnt = sizeof(*ofdm);
		}
	}
	if (cnt == 0) {
		memset(ofdm->pwr, (int8)WL_RATE_DISABLED, sizeof(*ofdm));
	}
	return cnt;
}


/* Get the HT MCS values for the group specified by Nss, with the given bw and tx chains */
int ppr_get_ht_mcs(ppr_t* pprptr, wl_tx_bw_t bw, wl_tx_nss_t Nss, wl_tx_mode_t mode,
	wl_tx_chains_t tx_chains, ppr_ht_mcs_rateset_t* mcs)
{
	pprpbw_t* bw_pwrs;
	const int8* powers;
	int cnt = 0;

	ASSERT(pprptr);
	bw_pwrs = ppr_get_bw_powers(pprptr, bw);
	if (bw_pwrs != NULL) {
		powers = ppr_get_mcs_group(bw_pwrs, Nss, mode, tx_chains);
		if (powers != NULL) {
			bcopy(powers, mcs->pwr, sizeof(*mcs));
			cnt = sizeof(*mcs);
		}
	}
	if (cnt == 0) {
		memset(mcs->pwr, (int8)WL_RATE_DISABLED, sizeof(*mcs));
	}

	return cnt;
}


/* Get the VHT MCS values for the group specified by Nss, with the given bw and tx chains */
int ppr_get_vht_mcs(ppr_t* pprptr, wl_tx_bw_t bw, wl_tx_nss_t Nss, wl_tx_mode_t mode,
	wl_tx_chains_t tx_chains, ppr_vht_mcs_rateset_t* mcs)
{
	pprpbw_t* bw_pwrs;
	const int8* powers;
	int cnt = 0;

	ASSERT(pprptr);
	bw_pwrs = ppr_get_bw_powers(pprptr, bw);
	if (bw_pwrs != NULL) {
		powers = ppr_get_mcs_group(bw_pwrs, Nss, mode, tx_chains);
		if (powers != NULL) {
			bcopy(powers, mcs->pwr, sizeof(*mcs));
			cnt = sizeof(*mcs);
		}
	}
	if (cnt == 0) {
		memset(mcs->pwr, (int8)WL_RATE_DISABLED, sizeof(*mcs));
	}
	return cnt;
}


/* Routines to set target powers per rate in a group */

/* Set the dsss values for the given number of tx_chains and 20, 20in40, etc. */
int ppr_set_dsss(ppr_t* pprptr, wl_tx_bw_t bw, wl_tx_chains_t tx_chains,
	const ppr_dsss_rateset_t* dsss)
{
	pprpbw_t* bw_pwrs;
	int8* powers;
	int cnt = 0;

	bw_pwrs = ppr_get_bw_powers(pprptr, bw);
	if (bw_pwrs != NULL) {
		powers = (int8*)ppr_get_dsss_group(bw_pwrs, tx_chains);
		if (powers != NULL) {
			bcopy(dsss->pwr, powers, sizeof(*dsss));
			cnt = sizeof(*dsss);
		}
	}
	return cnt;
}


/* Set the ofdm values for the given number of tx_chains and 20, 20in40, etc. */
int ppr_set_ofdm(ppr_t* pprptr, wl_tx_bw_t bw, wl_tx_mode_t mode, wl_tx_chains_t tx_chains,
	const ppr_ofdm_rateset_t* ofdm)
{
	pprpbw_t* bw_pwrs;
	int8* powers;
	int cnt = 0;

	bw_pwrs = ppr_get_bw_powers(pprptr, bw);
	if (bw_pwrs != NULL) {
		powers = (int8*)ppr_get_ofdm_group(bw_pwrs, mode, tx_chains);
		if (powers != NULL) {
			bcopy(ofdm->pwr, powers, sizeof(*ofdm));
			cnt = sizeof(*ofdm);
		}
	}
	return cnt;
}


/* Set the HT MCS values for the group specified by Nss, with the given bw and tx chains */
int ppr_set_ht_mcs(ppr_t* pprptr, wl_tx_bw_t bw, wl_tx_nss_t Nss, wl_tx_mode_t mode,
	wl_tx_chains_t tx_chains, const ppr_ht_mcs_rateset_t* mcs)
{
	pprpbw_t* bw_pwrs;
	int8* powers;
	int cnt = 0;

	bw_pwrs = ppr_get_bw_powers(pprptr, bw);
	if (bw_pwrs != NULL) {
		powers = (int8*)ppr_get_mcs_group(bw_pwrs, Nss, mode, tx_chains);
		if (powers != NULL) {
			bcopy(mcs->pwr, powers, sizeof(*mcs));
			cnt = sizeof(*mcs);
		}
	}
	return cnt;
}


/* Set the VHT MCS values for the group specified by Nss, with the given bw and tx chains */
int ppr_set_vht_mcs(ppr_t* pprptr, wl_tx_bw_t bw, wl_tx_nss_t Nss, wl_tx_mode_t mode,
	wl_tx_chains_t tx_chains, const ppr_vht_mcs_rateset_t* mcs)
{
	pprpbw_t* bw_pwrs;
	int8* powers;
	int cnt = 0;

	bw_pwrs = ppr_get_bw_powers(pprptr, bw);
	if (bw_pwrs != NULL) {
		powers = (int8*)ppr_get_mcs_group(bw_pwrs, Nss, mode, tx_chains);
		if (powers != NULL) {
			bcopy(mcs->pwr, powers, sizeof(*mcs));
			cnt = sizeof(*mcs);
		}
	}
	return cnt;
}


/* Routines to set rate groups to a single target value */

/* Set the dsss values for the given number of tx_chains and 20, 20in40, etc. */
int ppr_set_same_dsss(ppr_t* pprptr, wl_tx_bw_t bw, wl_tx_chains_t tx_chains, const int8 power)
{
	pprpbw_t* bw_pwrs;
	int8* dest_group;
	int cnt = 0;
	int i;

	bw_pwrs = ppr_get_bw_powers(pprptr, bw);
	if (bw_pwrs != NULL) {
		dest_group = (int8*)ppr_get_dsss_group(bw_pwrs, tx_chains);
		if (dest_group != NULL) {
			cnt = sizeof(ppr_dsss_rateset_t);
			for (i = 0; i < cnt; i++)
				*dest_group++ = power;
		}
	}
	return cnt;
}

/* Set the ofdm values for the given number of tx_chains and 20, 20in40, etc. */
int ppr_set_same_ofdm(ppr_t* pprptr, wl_tx_bw_t bw, wl_tx_mode_t mode, wl_tx_chains_t tx_chains,
	const int8 power)
{
	pprpbw_t* bw_pwrs;
	int8* dest_group;
	int cnt = 0;
	int i;

	bw_pwrs = ppr_get_bw_powers(pprptr, bw);
	if (bw_pwrs != NULL) {
		dest_group = (int8*)ppr_get_ofdm_group(bw_pwrs, mode, tx_chains);
		if (dest_group != NULL) {
			cnt = sizeof(ppr_ofdm_rateset_t);
			for (i = 0; i < cnt; i++)
				*dest_group++ = power;
		}
	}
	return cnt;
}


/* Set the HT MCS values for the group specified by Nss, with the given bw and tx chains */
int ppr_set_same_ht_mcs(ppr_t* pprptr, wl_tx_bw_t bw, wl_tx_nss_t Nss, wl_tx_mode_t mode,
	wl_tx_chains_t tx_chains, const int8 power)
{
	pprpbw_t* bw_pwrs;
	int8* dest_group;
	int cnt = 0;
	int i;

	bw_pwrs = ppr_get_bw_powers(pprptr, bw);
	if (bw_pwrs != NULL) {
		dest_group = (int8*)ppr_get_mcs_group(bw_pwrs, Nss, mode, tx_chains);
		if (dest_group != NULL) {
			cnt = sizeof(ppr_ht_mcs_rateset_t);
			for (i = 0; i < cnt; i++)
				*dest_group++ = power;
		}
	}
	return cnt;
}


/* Set the HT MCS values for the group specified by Nss, with the given bw and tx chains */
int ppr_set_same_vht_mcs(ppr_t* pprptr, wl_tx_bw_t bw, wl_tx_nss_t Nss, wl_tx_mode_t mode,
	wl_tx_chains_t tx_chains, const int8 power)
{
	pprpbw_t* bw_pwrs;
	int8* dest_group;
	int cnt = 0;
	int i;

	bw_pwrs = ppr_get_bw_powers(pprptr, bw);
	if (bw_pwrs != NULL) {
		dest_group = (int8*)ppr_get_mcs_group(bw_pwrs, Nss, mode, tx_chains);
		if (dest_group != NULL) {
			cnt = sizeof(ppr_vht_mcs_rateset_t);
			for (i = 0; i < cnt; i++)
				*dest_group++ = power;
		}
	}
	return cnt;
}


/* Helper routines to operate on the entire ppr set */

/* Ensure no rate limit is greater than the cap */
uint ppr_apply_max(ppr_t* pprptr, int8 maxval)
{
	uint i;
	int8* rptr = (int8*)&pprptr->ppr_bw;

	for (i = 0; i < ppr_pwrs_size(pprptr->ch_bw); i++, rptr++) {
		*rptr = MIN(*rptr, maxval);
	}
	return i;
}


/* Ensure no rate limit is lower than the specified minimum */
uint ppr_apply_min(ppr_t* pprptr, int8 minval)
{
	uint i;
	int8* rptr = (int8*)&pprptr->ppr_bw;

	for (i = 0; i < ppr_pwrs_size(pprptr->ch_bw); i++, rptr++) {
		*rptr = MAX(*rptr, minval);
	}
	return i;
}


/* Ensure no rate limit in this ppr set is greater than the corresponding limit in ppr_cap */
uint ppr_apply_vector_ceiling(ppr_t* pprptr, const ppr_t* ppr_cap)
{
	uint i = 0;
	int8* rptr = (int8*)&pprptr->ppr_bw;
	const int8* capptr = (const int8*)&ppr_cap->ppr_bw;

	if (pprptr->ch_bw == ppr_cap->ch_bw) {
		for (i = 0; i < ppr_pwrs_size(pprptr->ch_bw); i++, rptr++, capptr++) {
			*rptr = MIN(*rptr, *capptr);
		}
	}
	return i;
}


/* Ensure no rate limit in this ppr set is lower than the corresponding limit in ppr_min */
uint ppr_apply_vector_floor(ppr_t* pprptr, const ppr_t* ppr_min)
{
	uint i = 0;
	int8* rptr = (int8*)&pprptr->ppr_bw;
	const int8* minptr = (const int8*)&ppr_min->ppr_bw;

	if (pprptr->ch_bw == ppr_min->ch_bw) {
		for (i = 0; i < ppr_pwrs_size(pprptr->ch_bw); i++, rptr++, minptr++) {
			*rptr = MAX((uint8)*rptr, (uint8)*minptr);
		}
	}
	return i;
}


/* Get the maximum power in the ppr set */
int8 ppr_get_max(ppr_t* pprptr)
{
	uint i;
	int8* rptr = (int8*)&pprptr->ppr_bw;
	int8 maxval = *rptr++;

	for (i = 1; i < ppr_pwrs_size(pprptr->ch_bw); i++, rptr++) {
		maxval = MAX(maxval, *rptr);
	}
	return maxval;
}

/*
 * Get the minimum power in the ppr set, excluding disallowed
 * rates and (possibly) powers set to the minimum for the phy
 */
int8 ppr_get_min(ppr_t* pprptr, int8 floor)
{
	uint i;
	int8* rptr = (int8*)&pprptr->ppr_bw;
	int8 minval = WL_RATE_DISABLED;

	for (i = 0; (i < ppr_pwrs_size(pprptr->ch_bw)) && ((minval == WL_RATE_DISABLED) ||
		(minval == floor)); i++, rptr++) {
		minval = *rptr;
	}
	for (; i < ppr_pwrs_size(pprptr->ch_bw); i++, rptr++) {
		if ((*rptr != WL_RATE_DISABLED) && (*rptr != floor))
			minval = MIN(minval, *rptr);
	}
	return minval;
}


/* Get the maximum power for a given bandwidth in the ppr set */
int8 ppr_get_max_for_bw(ppr_t* pprptr, wl_tx_bw_t bw)
{
	uint i;
	const pprpbw_t* bw_pwrs;
	const int8* rptr;
	int8 maxval;

	bw_pwrs = ppr_get_bw_powers(pprptr, bw);
	if (bw_pwrs != NULL) {
		rptr = (const int8*)bw_pwrs;
		maxval = *rptr++;

		for (i = 1; i < sizeof(*bw_pwrs); i++, rptr++) {
			maxval = MAX(maxval, *rptr);
		}
	} else {
		maxval = WL_RATE_DISABLED;
	}
	return maxval;
}


/* Get the minimum power for a given bandwidth  in the ppr set */
int8 ppr_get_min_for_bw(ppr_t* pprptr, wl_tx_bw_t bw)
{
	uint i;
	const pprpbw_t* bw_pwrs;
	const int8* rptr;
	int8 minval;

	bw_pwrs = ppr_get_bw_powers(pprptr, bw);
	if (bw_pwrs != NULL) {
		rptr = (const int8*)bw_pwrs;
		minval = *rptr++;

		for (i = 1; i < sizeof(*bw_pwrs); i++, rptr++) {
			minval = MIN(minval, *rptr);
		}
	} else
		minval = WL_RATE_DISABLED;
	return minval;
}


/* Map the given function with its context value over the two power vectors */
void
ppr_map_vec_dsss(ppr_mapfn_t fn, void* context, ppr_t* pprptr1, ppr_t* pprptr2,
	wl_tx_bw_t bw, wl_tx_chains_t tx_chains)
{
	pprpbw_t* bw_pwrs1;
	pprpbw_t* bw_pwrs2;
	int8* powers1;
	int8* powers2;
	uint i;

	bw_pwrs1 = ppr_get_bw_powers(pprptr1, bw);
	bw_pwrs2 = ppr_get_bw_powers(pprptr2, bw);
	if ((bw_pwrs1 != NULL) && (bw_pwrs2 != NULL)) {
		powers1 = (int8*)ppr_get_dsss_group(bw_pwrs1, tx_chains);
		powers2 = (int8*)ppr_get_dsss_group(bw_pwrs2, tx_chains);
		if ((powers1 != NULL) && (powers2 != NULL)) {
			for (i = 0; i < WL_RATESET_SZ_DSSS; i++)
				(fn)(context, (uint8*)powers1++, (uint8*)powers2++);
		}
	}
}


/* Map the given function with its context value over the two power vectors */
void
ppr_map_vec_ofdm(ppr_mapfn_t fn, void* context, ppr_t* pprptr1, ppr_t* pprptr2,
	wl_tx_bw_t bw, wl_tx_mode_t mode, wl_tx_chains_t tx_chains)
{
	pprpbw_t* bw_pwrs1;
	pprpbw_t* bw_pwrs2;
	int8* powers1;
	int8* powers2;
	uint i;

	bw_pwrs1 = ppr_get_bw_powers(pprptr1, bw);
	bw_pwrs2 = ppr_get_bw_powers(pprptr2, bw);
	if ((bw_pwrs1 != NULL) && (bw_pwrs2 != NULL)) {
		powers1 = (int8*)ppr_get_ofdm_group(bw_pwrs1, mode, tx_chains);
		powers2 = (int8*)ppr_get_ofdm_group(bw_pwrs2, mode, tx_chains);
		if ((powers1 != NULL) && (powers2 != NULL)) {
			for (i = 0; i < WL_RATESET_SZ_OFDM; i++)
				(fn)(context, (uint8*)powers1++, (uint8*)powers2++);
		}
	}
}


/* Map the given function with its context value over the two power vectors */
void
ppr_map_vec_ht_mcs(ppr_mapfn_t fn, void* context, ppr_t* pprptr1,
	ppr_t* pprptr2, wl_tx_bw_t bw, wl_tx_nss_t Nss, wl_tx_mode_t mode,
	wl_tx_chains_t tx_chains)
{
	pprpbw_t* bw_pwrs1;
	pprpbw_t* bw_pwrs2;
	int8* powers1;
	int8* powers2;
	uint i;

	bw_pwrs1 = ppr_get_bw_powers(pprptr1, bw);
	bw_pwrs2 = ppr_get_bw_powers(pprptr2, bw);
	if ((bw_pwrs1 != NULL) && (bw_pwrs2 != NULL)) {
		powers1 = (int8*)ppr_get_mcs_group(bw_pwrs1, Nss, mode, tx_chains);
		powers2 = (int8*)ppr_get_mcs_group(bw_pwrs2, Nss, mode, tx_chains);
		if ((powers1 != NULL) && (powers2 != NULL)) {
			for (i = 0; i < WL_RATESET_SZ_HT_MCS; i++)
				(fn)(context, (uint8*)powers1++, (uint8*)powers2++);
		}
	}
}


/* Map the given function with its context value over the two power vectors */
void
ppr_map_vec_vht_mcs(ppr_mapfn_t fn, void* context, ppr_t* pprptr1,
	ppr_t* pprptr2, wl_tx_bw_t bw, wl_tx_nss_t Nss, wl_tx_mode_t mode, wl_tx_chains_t
	tx_chains)
{
	pprpbw_t* bw_pwrs1;
	pprpbw_t* bw_pwrs2;
	int8* powers1;
	int8* powers2;
	uint i;

	bw_pwrs1 = ppr_get_bw_powers(pprptr1, bw);
	bw_pwrs2 = ppr_get_bw_powers(pprptr2, bw);
	if ((bw_pwrs1 != NULL) && (bw_pwrs2 != NULL)) {
		powers1 = (int8*)ppr_get_mcs_group(bw_pwrs1, Nss, mode, tx_chains);
		powers2 = (int8*)ppr_get_mcs_group(bw_pwrs2, Nss, mode, tx_chains);
		if ((powers1 != NULL) && (powers2 != NULL)) {
			for (i = 0; i < WL_RATESET_SZ_VHT_MCS; i++)
				(fn)(context, (uint8*)powers1++, (uint8*)powers2++);
		}
	}
}


/* Map the given function with its context value over the two power vectors */

void
ppr_map_vec_all(ppr_mapfn_t fn, void* context, ppr_t* pprptr1, ppr_t* pprptr2)
{
	uint i;
	pprpbw_t* bw_pwrs1;
	pprpbw_t* bw_pwrs2;
	int8* rptr1 = (int8*)&pprptr1->ppr_bw;
	int8* rptr2 = (int8*)&pprptr2->ppr_bw;

	bw_pwrs1 = ppr_get_bw_powers(pprptr1, WL_TX_BW_20);
	bw_pwrs2 = ppr_get_bw_powers(pprptr2, WL_TX_BW_20);
	if ((bw_pwrs1 != NULL) && (bw_pwrs2 != NULL)) {
		rptr1 = (int8*)bw_pwrs1;
		rptr2 = (int8*)bw_pwrs2;
		for (i = 0; i < sizeof(pprpbw_t); i++, rptr1++, rptr2++) {
			(fn)(context, (uint8*)rptr1, (uint8*)rptr2);
		}
	}

	bw_pwrs1 = ppr_get_bw_powers(pprptr1, WL_TX_BW_40);
	bw_pwrs2 = ppr_get_bw_powers(pprptr2, WL_TX_BW_40);

	if ((bw_pwrs1 != NULL) && (bw_pwrs2 != NULL)) {
		rptr1 = (int8*)bw_pwrs1;
		rptr2 = (int8*)bw_pwrs2;
		for (i = 0; i < sizeof(pprpbw_t); i++, rptr1++, rptr2++) {
			(fn)(context, (uint8*)rptr1, (uint8*)rptr2);
		}

		bw_pwrs1 = ppr_get_bw_powers(pprptr1, WL_TX_BW_20IN40);
		bw_pwrs2 = ppr_get_bw_powers(pprptr2, WL_TX_BW_20IN40);
		if ((bw_pwrs1 != NULL) && (bw_pwrs2 != NULL)) {
			rptr1 = (int8*)bw_pwrs1;
			rptr2 = (int8*)bw_pwrs2;
			for (i = 0; i < sizeof(pprpbw_t); i++, rptr1++, rptr2++) {
				(fn)(context, (uint8*)rptr1, (uint8*)rptr2);
			}
		}
	}

	bw_pwrs1 = ppr_get_bw_powers(pprptr1, WL_TX_BW_80);
	bw_pwrs2 = ppr_get_bw_powers(pprptr2, WL_TX_BW_80);
	if ((bw_pwrs1 != NULL) && (bw_pwrs2 != NULL)) {
		rptr1 = (int8*)bw_pwrs1;
		rptr2 = (int8*)bw_pwrs2;
		for (i = 0; i < sizeof(pprpbw_t); i++, rptr1++, rptr2++) {
			(fn)(context, (uint8*)rptr1, (uint8*)rptr2);
		}

		bw_pwrs1 = ppr_get_bw_powers(pprptr1, WL_TX_BW_20IN80);
		bw_pwrs2 = ppr_get_bw_powers(pprptr2, WL_TX_BW_20IN80);
		if ((bw_pwrs1 != NULL) && (bw_pwrs2 != NULL)) {
			rptr1 = (int8*)bw_pwrs1;
			rptr2 = (int8*)bw_pwrs2;
			for (i = 0; i < sizeof(pprpbw_t); i++, rptr1++, rptr2++) {
				(fn)(context, (uint8*)rptr1, (uint8*)rptr2);
			}
		}

		bw_pwrs1 = ppr_get_bw_powers(pprptr1, WL_TX_BW_40IN80);
		bw_pwrs2 = ppr_get_bw_powers(pprptr2, WL_TX_BW_40IN80);
		if ((bw_pwrs1 != NULL) && (bw_pwrs2 != NULL)) {
			rptr1 = (int8*)bw_pwrs1;
			rptr2 = (int8*)bw_pwrs2;
			for (i = 0; i < sizeof(pprpbw_t); i++, rptr1++, rptr2++) {
				(fn)(context, (uint8*)rptr1, (uint8*)rptr2);
			}
		}
	}
}


/* Set PPR struct to a certain power level */
void
ppr_set_cmn_val(ppr_t* pprptr, int8 val)
{
	memset((uchar*)&pprptr->ppr_bw, val, ppr_pwrs_size(pprptr->ch_bw));
}


/* Make an identical copy of a ppr structure (for ppr_bw==all case) */
void
ppr_copy_struct(ppr_t* pprptr_s, ppr_t* pprptr_d)
{
	int8* rptr_s = (int8*)&pprptr_s->ppr_bw;
	int8* rptr_d = (int8*)&pprptr_d->ppr_bw;
	/* ASSERT(ppr_pwrs_size(pprptr_d->ch_bw) >= ppr_pwrs_size(pprptr_s->ch_bw)); */

	if (pprptr_s->ch_bw == pprptr_d->ch_bw)
		bcopy(rptr_s, rptr_d, ppr_pwrs_size(pprptr_s->ch_bw));
	else {
		const pprpbw_t* src_bw_pwrs;
		pprpbw_t* dest_bw_pwrs;

		src_bw_pwrs = ppr_get_bw_powers(pprptr_s, WL_TX_BW_20);
		dest_bw_pwrs = ppr_get_bw_powers(pprptr_d, WL_TX_BW_20);
		if (src_bw_pwrs && dest_bw_pwrs)
			bcopy((const uint8*)src_bw_pwrs, (uint8*)dest_bw_pwrs,
				sizeof(*src_bw_pwrs));

		src_bw_pwrs = ppr_get_bw_powers(pprptr_s, WL_TX_BW_40);
		dest_bw_pwrs = ppr_get_bw_powers(pprptr_d, WL_TX_BW_40);
		if (src_bw_pwrs && dest_bw_pwrs)
			bcopy((const uint8*)src_bw_pwrs, (uint8*)dest_bw_pwrs,
				sizeof(*src_bw_pwrs));

		src_bw_pwrs = ppr_get_bw_powers(pprptr_s, WL_TX_BW_20IN40);
		dest_bw_pwrs = ppr_get_bw_powers(pprptr_d, WL_TX_BW_20IN40);
		if (src_bw_pwrs && dest_bw_pwrs)
			bcopy((const uint8*)src_bw_pwrs, (uint8*)dest_bw_pwrs,
				sizeof(*src_bw_pwrs));

		src_bw_pwrs = ppr_get_bw_powers(pprptr_s, WL_TX_BW_80);
		dest_bw_pwrs = ppr_get_bw_powers(pprptr_d, WL_TX_BW_80);
		if (src_bw_pwrs && dest_bw_pwrs)
			bcopy((const uint8*)src_bw_pwrs, (uint8*)dest_bw_pwrs,
				sizeof(*src_bw_pwrs));

		src_bw_pwrs = ppr_get_bw_powers(pprptr_s, WL_TX_BW_20IN80);
		dest_bw_pwrs = ppr_get_bw_powers(pprptr_d, WL_TX_BW_20IN80);
		if (src_bw_pwrs && dest_bw_pwrs)
			bcopy((const uint8*)src_bw_pwrs, (uint8*)dest_bw_pwrs,
				sizeof(*src_bw_pwrs));

		src_bw_pwrs = ppr_get_bw_powers(pprptr_s, WL_TX_BW_40IN80);
		dest_bw_pwrs = ppr_get_bw_powers(pprptr_d, WL_TX_BW_40IN80);
		if (src_bw_pwrs && dest_bw_pwrs)
			bcopy((const uint8*)src_bw_pwrs, (uint8*)dest_bw_pwrs,
				sizeof(*src_bw_pwrs));
	}
}

/* Subtract each power from a common value and re-store */
void
ppr_cmn_val_minus(ppr_t* pprptr, int8 val)
{
	uint i;
	int8* rptr = (int8*)&pprptr->ppr_bw;

	for (i = 0; i < ppr_pwrs_size(pprptr->ch_bw); i++, rptr++) {
		if (*rptr != (int8)WL_RATE_DISABLED)
			*rptr = val - *rptr;
	}

}


/* Subtract a common value from each power and re-store */
void
ppr_minus_cmn_val(ppr_t* pprptr, int8 val)
{
	uint i;
	int8* rptr = (int8*)&pprptr->ppr_bw;

	for (i = 0; i < ppr_pwrs_size(pprptr->ch_bw); i++, rptr++) {
		if (*rptr != (int8)WL_RATE_DISABLED)
			*rptr = (*rptr > val) ? (*rptr - val) : 0;
	}

}

/* Add a common value to each power and re-store */
void
ppr_plus_cmn_val(ppr_t* pprptr, int8 val)
{
	uint i;
	int8* rptr = (int8*)&pprptr->ppr_bw;

	for (i = 0; i < ppr_pwrs_size(pprptr->ch_bw); i++, rptr++) {
		if (*rptr != (int8)WL_RATE_DISABLED)
			*rptr += val;
	}

}

/* Multiply a percentage */
void
ppr_multiply_percentage(ppr_t* pprptr, uint8 val)
{
	uint i;
	int8* rptr = (int8*)&pprptr->ppr_bw;

	for (i = 0; i < ppr_pwrs_size(pprptr->ch_bw); i++, rptr++) {
		if (*rptr != (int8)WL_RATE_DISABLED)
			*rptr = (*rptr * val) / 100;
	}

}

/* Compare two ppr variables p1 and p2, save the min. value of each
 * contents to variable p1
 */
void
ppr_compare_min(ppr_t* p1, ppr_t* p2)
{
	uint i;
	int8* rptr1 = NULL;
	int8* rptr2 = NULL;
	uint32 pprsize = 0;

	if (p1->ch_bw == p2->ch_bw) {
		rptr1 = (int8*)&p1->ppr_bw;
		rptr2 = (int8*)&p2->ppr_bw;
		pprsize = ppr_pwrs_size(p1->ch_bw);
	} else if (p1->ch_bw == WL_TX_BW_ALL) {
		rptr1 = (int8*)ppr_get_bw_powers_all(p1, p2->ch_bw);
		rptr2 = (int8*)&p2->ppr_bw;
		pprsize = ppr_pwrs_size(p2->ch_bw);
	} else if (p2->ch_bw == WL_TX_BW_ALL) {
		rptr1 = (int8*)&p1->ppr_bw;
		rptr2 = (int8*)ppr_get_bw_powers_all(p2, p1->ch_bw);
		pprsize = ppr_pwrs_size(p1->ch_bw);
	} else {
		; /* can't compare in this case */
	}

	for (i = 0; i < pprsize; i++, rptr1++, rptr2++) {
		*rptr1 = MIN(*rptr1, *rptr2);
	}
}


/* Compare two ppr variables p1 and p2, save the max. value of each
 * contents to variable p1
 */
void
ppr_compare_max(ppr_t* p1, ppr_t* p2)
{
	uint i;
	int8* rptr1 = NULL;
	int8* rptr2 = NULL;
	uint32 pprsize = 0;

	if (p1->ch_bw == p2->ch_bw) {
		rptr1 = (int8*)&p1->ppr_bw;
		rptr2 = (int8*)&p2->ppr_bw;
		pprsize = ppr_pwrs_size(p1->ch_bw);
	} else if (p1->ch_bw == WL_TX_BW_ALL) {
		rptr1 = (int8*)ppr_get_bw_powers_all(p1, p2->ch_bw);
		rptr2 = (int8*)&p2->ppr_bw;
		pprsize = ppr_pwrs_size(p2->ch_bw);
	} else if (p2->ch_bw == WL_TX_BW_ALL) {
		rptr1 = (int8*)&p1->ppr_bw;
		rptr2 = (int8*)ppr_get_bw_powers_all(p2, p1->ch_bw);
		pprsize = ppr_pwrs_size(p1->ch_bw);
	} else {
		; /* can't compare in this case */
	}

	for (i = 0; i < pprsize; i++, rptr1++, rptr2++) {
		*rptr1 = MAX(*rptr1, *rptr2);
	}
}

static uint ppr_ser_size_by_flag(uint32 flag, wl_tx_bw_t bw)
{
	uint ret = PPR_CHAIN1_SIZE; /* at least 1 chain rates should be there */
	uint chain   = flag & PPR_MAX_TX_CHAIN_MASK;
	bool bf      = (flag & PPR_BEAMFORMING) != 0;
	BCM_REFERENCE(chain);
	BCM_REFERENCE(bf);
#if (PPR_MAX_TX_CHAINS > 1)
	if (chain > 1) {
		ret += PPR_CHAIN2_SIZE;
	}
#if (PPR_MAX_TX_CHAINS > 2)
	if (chain > 2) {
		ret += PPR_CHAIN3_SIZE;
	}
#endif

#ifdef WL_BEAMFORMING
	if (bf) {
		ret += PPR_BF_CHAIN2_SIZE;
	}
#if (PPR_MAX_TX_CHAINS > 2)
	if (bf && chain > 2) {
		ret += PPR_BF_CHAIN3_SIZE;
	}
#endif
#endif /* WL_BEAMFORMING */
#endif /* PPR_MAX_TX_CHAINS > 1 */
	switch (bw) {
	case WL_TX_BW_20:
		ret *= sizeof(ppr_bw_20_t)/sizeof(pprpbw_t);
		break;
	case WL_TX_BW_40:
		ret *= sizeof(ppr_bw_40_t)/sizeof(pprpbw_t);
		break;
	case WL_TX_BW_80:
		ret *= sizeof(ppr_bw_80_t)/sizeof(pprpbw_t);
		break;
	case WL_TX_BW_ALL:
		ret *= sizeof(ppr_bw_all_t)/sizeof(pprpbw_t);
		break;
	default:
		ASSERT(0);
	}
	return ret;

}

/* Serialize the contents of the opaque ppr struct.
 * Writes number of bytes copied, zero on error.
 * Returns error code, BCME_OK if successful.
 */
int
ppr_serialize(const ppr_t* pprptr, uint8* buf, uint buflen, uint* bytes_copied)
{
	int err = BCME_OK;
	if (buflen <= sizeof(ppr_ser_mem_flag_t)) {
		err = BCME_BUFTOOSHORT;
	} else {
		ppr_ser_mem_flag_t *smem_flag = (ppr_ser_mem_flag_t *)buf;
		uint32 flag = NTOH32(smem_flag->flag);

		/* check if memory contains a valid flag, if not, use current
		 * condition (num of chains, txbf etc.) to serialize data.
		 */
		if (NTOH32(smem_flag->magic_word) != PPR_SER_MEM_WORD) {
			flag = ppr_get_flag();
		}

		if (buflen >= ppr_ser_size_by_flag(flag, pprptr->ch_bw)) {
			*bytes_copied = ppr_serialize_data(pprptr, buf, flag);
		} else {
			err = BCME_BUFTOOSHORT;
		}
	}
	return err;
}


/* Deserialize the contents of a buffer into an opaque ppr struct.
 * Creates an opaque structure referenced by *pptrptr, NULL on error.
 * Returns error code, BCME_OK if successful.
 */
int
ppr_deserialize_create(osl_t *osh, const uint8* buf, uint buflen, ppr_t** pprptr)
{
	const uint8* bptr = buf;
	int err = BCME_OK;
	ppr_t* lpprptr = NULL;

	if ((buflen > SER_HDR_LEN) && (bptr != NULL) && (*bptr == PPR_SERIALIZATION_VER)) {
		const ppr_deser_header_t * ser_head = (const ppr_deser_header_t *)bptr;
		wl_tx_bw_t ch_bw = ser_head->bw;

		/* struct size plus header */
		uint32 ser_size = ppr_pwrs_size(ch_bw) + SER_HDR_LEN;

		if ((buflen >= ser_size) && ((lpprptr = ppr_create(osh, ch_bw)) != NULL)) {
			uint32 flags = NTOH32(ser_head->flags);
			/* set the data with default value before deserialize */
			ppr_set_cmn_val(lpprptr, WL_RATE_DISABLED);

			ppr_deser_cpy(lpprptr, bptr + sizeof(*ser_head), flags, ch_bw);
		} else if (buflen < ser_size) {
			err = BCME_BUFTOOSHORT;
		} else {
			err = BCME_NOMEM;
		}
	} else if (buflen <= SER_HDR_LEN) {
		err = BCME_BUFTOOSHORT;
	} else if (bptr == NULL) {
		err = BCME_BADARG;
	} else {
		err = BCME_VERSION;
	}
	*pprptr = lpprptr;
	return err;
}


/* Deserialize the contents of a buffer into an opaque ppr struct.
 * Creates an opaque structure referenced by *pptrptr, NULL on error.
 * Returns error code, BCME_OK if successful.
 */
int
ppr_deserialize(ppr_t* pprptr, const uint8* buf, uint buflen)
{
	const uint8* bptr = buf;
	int err = BCME_OK;
	ASSERT(pprptr);
	if ((buflen > SER_HDR_LEN) && (bptr != NULL) && (*bptr == PPR_SERIALIZATION_VER)) {
		const ppr_deser_header_t * ser_head = (const ppr_deser_header_t *)bptr;
		wl_tx_bw_t ch_bw = ser_head->bw;

		/* struct size plus header */
		uint32 ser_size = ppr_pwrs_size(ch_bw) + SER_HDR_LEN;

		if ((buflen >= ser_size) && (ch_bw == pprptr->ch_bw)) {
			uint32 flags = NTOH32(ser_head->flags);
			ppr_set_cmn_val(pprptr, WL_RATE_DISABLED);
			ppr_deser_cpy(pprptr, bptr + sizeof(*ser_head), flags, ch_bw);
		} else if (buflen < ser_size) {
			err = BCME_BUFTOOSHORT;
		} else {
			err = BCME_BADARG;
		}
	} else if (buflen <= SER_HDR_LEN) {
		err = BCME_BUFTOOSHORT;
	} else if (bptr == NULL) {
		err = BCME_BADARG;
	} else {
		err = BCME_VERSION;
	}
	return err;
}

#endif /* PPR_API */
