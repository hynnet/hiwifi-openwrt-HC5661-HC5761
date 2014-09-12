/*******************************************************************
 *
 * File:            debug.c
 *
 * Description:     Routines for sending debug messages
 *
 * Date:            03 March 2006
 *
 * Author:          B.H.Clarke
 *
 * Copyright:       Oxford Semiconductor Ltd, 2006
 *
 *******************************************************************/
#include <debug.h>

NS16550_t debug_uart;

void putstr(NS16550_t debug_uart, const char *s)
{
    while (*s) {
        putc_NS16550(debug_uart, *s++);
    }
}

char* ultohex(unsigned long num) {
    static char string[11];
    int i;

    string[0] = 'O';
    string[1] = 'x';
    for (i=9; i>=2; i--) {
        int digit = num & 0xf;
        if (digit <= 9) {
            string[i] = '0' + digit;
        } else {
            string[i] = 'a' + (digit-10);
        }
        num >>= 4;
    }
    string[10] = '\0';
    return string;
}

char* ultodec(unsigned long num) {
    static char string[11];
    int i;

    for (i=9; i>0; i--) {
        int digit = num % 10;
        string[i] = '0' + digit;
        num /= 10;
        if (num==0) break;
    }
    string[10] = '\0';
    return &string[i];
}

char* ustohex(unsigned short num) {
    static char string[7];
    int i;

    string[0] = 'O';
    string[1] = 'x';
    for (i=5; i>=2; i--) {
        int digit = num & 0xf;
        if (digit <= 9) {
            string[i] = '0' + digit;
        } else {
            string[i] = 'a' + (digit-10);
        }
        num >>= 4;
    }
    string[6] = '\0';
    return string;
}

char* uctohex(unsigned char num) {
    static char string[5];
    int i;

    string[0] = 'O';
    string[1] = 'x';
    for (i=3; i>=2; i--) {
        int digit = num & 0xf;
        if (digit <= 9) {
            string[i] = '0' + digit;
        } else {
            string[i] = 'a' + (digit-10);
        }
        num >>= 4;
    }
    string[4] = '\0';
    return string;
}

void puthex32(NS16550_t uart, unsigned long val)
{
    const char *C_HEX = "0123456789ABCDEF";
    char str[11];
    int i;
    
    str[0] = '0';
    str[1] = 'x';
    
    for (i=0;i<8;i++)
        str[i+2] = C_HEX[(val & (0xF << (28 - (i*4)))) >> (28 - (i*4))];
    
    str[10] = 0;
    
    putstr(uart, str);
}

#ifdef SDK_BUILD_DEBUG
void puthex8(NS16550_t uart, unsigned char val)
{
    const char *C_HEX = "0123456789ABCDEF";
    char str[3];
    int i;

    for (i=0;i<2;i++)
        str[i] = C_HEX[(val & (0xF << (4 - (i*4)))) >> (4 - (i*4))];
    
    str[2] = 0;
    
    putstr(uart, str);
}
#endif

