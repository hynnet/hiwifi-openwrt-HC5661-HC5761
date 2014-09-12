/*******************************************************************
*
* File:            pll.h
*
* Description:     Include file for pll.c
*
* Author:          Julien Margetts
*
* Copyright:       PLX Technology PLC 2010
*
*/


// Config structure

typedef struct {

	unsigned short mhz;
	unsigned char  refdiv;
	unsigned char  outdiv;
	unsigned int   fbdiv;		
	unsigned short bwadj;
	unsigned short sfreq;
	unsigned int   sslope;

} PLL_CONFIG;


// Fixed point maths routines

typedef int Fixed;

#define FRAC_BITS   18
#define C_HALF		(1<<(FRAC_BITS-1))
#define C_ONE		(1<<FRAC_BITS)

Fixed MUL(Fixed a, Fixed b);
Fixed DIV(Fixed a, Fixed b);
int   ToInt(Fixed a);
Fixed FixedInt(int a);

#define PLL_BYPASS (1<<1)
#define SAT_ENABLE (1<<3)


// PLL functions

int  plla_num_configs();
int  plla_config_mhz(int idx);
int plla_set_config(int idx);
void plla_set_mhz(int mhz);

