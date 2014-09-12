/*******************************************************************
*
* File:            crc32.h
*
* Description:     SATA control.
*
* Author:          Richard Crewe
*
* Copyright:       Oxford Semiconductor Ltd, 2009
*
*
*/

#ifndef CRC32_H
#define CRC32_H

/* returns 32-bit crc */
unsigned int crc32(unsigned int initial_crc, const unsigned char* address, unsigned int bytes);


#endif
