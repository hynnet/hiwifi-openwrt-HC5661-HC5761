/*******************************************************************************

 
  Filename:    new_rs.h
  Description: Global definitions for Reed-Solomon encoder/decoder


   Author: STMicroelectronics

 
   				                
 You have a license to reproduce, display, perform, produce derivative works of, 
 and distribute (in original or modified form) the Program, provided that you 
 explicitly agree to the following disclaimer:
   
   THIS PROGRAM IS PROVIDED "AS IT IS" WITHOUT WARRANTY OF ANY KIND, EITHER 
   EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO, THE IMPLIED WARRANTY 
   OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE. THE ENTIRE RISK 
   AS TO THE QUALITY AND PERFORMANCE OF THE PROGRAM IS WITH YOU. SHOULD THE 
   PROGRAM PROVE DEFECTIVE, YOU ASSUME THE COST OF ALL NECESSARY SERVICING, 
   REPAIR OR CORRECTION.

********************************************************************************/

#ifndef __MTD_NAND_ECC_RS_H__
#define __MTD_NAND_ECC_RS_H__
#include <common.h>

//typedef unsigned int dtype;
typedef u_short dtype;


/* Initialization function */
void init_rs(void);

/* These two functions *must* be called in this order (e.g.,
 * by init_rs()) before any encoding/decoding
 */

void generate_gf(void);	/* Generate Galois Field */
void gen_poly(void);	/* Generate generator polynomial */

/* Reed-Solomon encoding
 * data[] is the input block, parity symbols are placed in bb[]
 * bb[] may lie past the end of the data, e.g., for (255,223):
 *	encode_rs(&data[0],&data[223]);
 */
char encode_rs(dtype data[], dtype bb[]);


/* Reed-Solomon errors decoding
 * The received block goes into data[]
 *
 * The decoder corrects the symbols in place, if possible and returns
 * the number of corrected symbols. If the codeword is illegal or
 * uncorrectible, the data array is unchanged and -1 is returned
 */
int decode_rs(dtype data[]);

/**
 * nand_calculate_ecc - [NAND Interface] Calculate 3 byte ECC code for 256 byte block
 * @mtd:	MTD block structure
 * @dat:	raw data
 * @ecc_code:	buffer for ECC
 */
int nand_calculate_ecc_rs(struct mtd_info *mtd, const u_char *data, u_char *ecc_code);

/**
 * nand_correct_data - [NAND Interface] Detect and correct bit error(s)
 * @mtd:	MTD block structure
 * @dat:	raw data read from the chip
 * @store_ecc:	ECC from the chip
 * @calc_ecc:	the ECC calculated from raw data
 *
 * Detect and correct a 1 bit error for 256 byte block
 */
int nand_correct_data_rs(struct mtd_info *mtd, u_char *data, u_char *store_ecc, u_char *calc_ecc);

#endif
