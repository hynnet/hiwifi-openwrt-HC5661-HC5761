/*******************************************************************
*
* File:            debug.h
*
* Description:     Headers for debug text ouput
*
* Author:          Unknown
*
* Copyright:       Oxford Semiconductor Ltd, 2009
*
*
*/

#if !defined(__DEBUG_H__)
#define __DEBUG_H__

#include <ns16550.h>

extern NS16550_t debug_uart;
extern void putstr(NS16550_t debug_uart, const char *s);
extern char* ultohex(unsigned long num);
extern char* ultodec(unsigned long num);
extern char* ustohex(unsigned short num);
extern char* uctohex(unsigned char num);
extern void puthex32(NS16550_t uart, unsigned long val);
#ifdef SDK_BUILD_DEBUG
extern void puthex8(NS16550_t uart, unsigned char val);
#endif

#endif        //  #if !defined(__DEBUG_H__)

