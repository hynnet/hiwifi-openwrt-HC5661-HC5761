/*******************************************************************
*
* File:            pll.c
*
* Description:     Routine for calculating values and setting up the
*                  PLL for a given ARM clock frequency                  
*
* Author:          Julien Margetts
*
* Copyright:       PLX Technology PLC 2010
*
*/

#include "oxnas.h"
#include "debug.h"
#include "pll.h"

PLL_CONFIG C_PLL_CONFIG[] = 
{
	{ 500,1,2,3932160,119,208,189}, //  500 MHz
	{ 525,2,1,4128768,125,139,297}, //  525 MHz
	{ 550,2,1,4325376,131,139,311}, //  550 MHz
	{ 575,2,1,4521984,137,139,326}, //  575 MHz
	{ 600,2,1,4718592,143,138,339}, //  600 MHz
	{ 625,1,1,3276800, 99,208,157}, //  625 MHz
	{ 650,1,1,3407872,103,208,164}, //  650 MHz
	{ 675,1,1,3538944,107,208,170}, //  675 MHz
	{ 700,0,0, 917504, 27,416, 22}, //  700 MHz
	{ 725,1,1,3801088,115,208,182}, //  725 MHz
	{ 750,0,0, 983040, 29,416, 23}, //  750 MHz
	{ 775,3,0,4063232,123,104,390}, //  775 MHz
	{ 800,3,0,4194304,127,104,403}, //  800 MHz
	{ 825,3,0,4325376,131,104,415}, //  825 MHz
	{ 850,2,0,3342336,101,139,241}, //  850 MHz
	{ 875,2,0,3440640,104,139,248}, //  875 MHz
	{ 900,2,0,3538944,107,139,255}, //  900 MHz
	{ 925,2,0,3637248,110,139,262}, //  925 MHz
	{ 950,2,0,3735552,113,139,269}, //  950 MHz
	{ 975,2,0,3833856,116,139,276}, //  975 MHz
	{1000,2,0,3932160,119,139,283}, // 1000 MHz
};


extern void udelay(u32 time);


// PLL functions

static void plla_configure(int outdiv, int refdiv, int fbdiv, int bwadj, int sfreq, int sslope)
{
    volatile unsigned int *plla_ctrl = (volatile unsigned int *) 0x44e001f0;    
    
    plla_ctrl[0] |= PLL_BYPASS; // Bypass PLL
    
    udelay(10); // No real reason
    
    writel((1 << SYS_CTRL_RSTEN_PLLA_BIT), SYS_CTRL_RSTEN_SET_CTRL);
    
    udelay(10); // No real reason

	plla_ctrl[0] = (refdiv << 8) | (outdiv << 4) | SAT_ENABLE | PLL_BYPASS;
	plla_ctrl[1] = fbdiv;
	plla_ctrl[2] = (bwadj << 16) | sfreq;
	plla_ctrl[3] = sslope;

    udelay(10); // 5us delay required (from TCI datasheet), use 10us
    
    writel((1 << SYS_CTRL_RSTEN_PLLA_BIT), SYS_CTRL_RSTEN_CLR_CTRL);

    udelay(100); // Delay for PLL to lock    

//     putstr(debug_uart, "\r\n  plla_ctrl0 : "); puthex32(debug_uart, plla_ctrl[0]);
//     putstr(debug_uart, "\r\n  plla_ctrl1 : "); puthex32(debug_uart, plla_ctrl[1]);
//     putstr(debug_uart, "\r\n  plla_ctrl2 : "); puthex32(debug_uart, plla_ctrl[2]);
//     putstr(debug_uart, "\r\n  plla_ctrl3 : "); puthex32(debug_uart, plla_ctrl[3]);
    
    plla_ctrl[0] &= ~PLL_BYPASS; // Take PLL out of bypass

/*    putstr(debug_uart, "\r\nPLLA Set\r\n\r\n");*/ 
}


int plla_num_configs()
{
    return sizeof(C_PLL_CONFIG) / sizeof(C_PLL_CONFIG[0]);
}


int plla_config_mhz(int idx)
{
    return C_PLL_CONFIG[idx].mhz;
}


int plla_set_config(int idx)
{
    PLL_CONFIG *cfg = &C_PLL_CONFIG[idx];
    
    putstr(debug_uart, "CPU Clock set to "); 
    putstr(debug_uart, ultodec((unsigned)cfg->mhz));
    putstr(debug_uart, "MHz ..."); 	
	
	plla_configure(cfg->outdiv, cfg->refdiv, cfg->fbdiv, cfg->bwadj, cfg->sfreq, cfg->sslope);
	
	return cfg->mhz;
}


#if 0 // This isn't included so as not to include integer division code bloat

// Fixed point maths routines

Fixed MUL(Fixed a, Fixed b) 
{ 
	long long tmp = (long long)a * b;
	tmp = (tmp + C_HALF) >> FRAC_BITS;
	return (Fixed)tmp;
}


Fixed DIV(Fixed a, Fixed b)
{ 	
	long long tmp;
	tmp = (((long long)a<<FRAC_BITS) + (b>>1)) / b;
	return (Fixed)tmp; 
}


int ToInt(Fixed a)
{ 
    return (a + C_HALF) >> FRAC_BITS;
}


Fixed FixedInt(int a)
{ 
    return a << FRAC_BITS;
}


void plla_set_mhz(int mhz)
{
	const int   outdiv	      = 1500 / mhz;
	const Fixed fref_MHz	  = FixedInt(25);
	const Fixed fs_Hz		  = FixedInt(30);
	const Fixed fout_MHz      = FixedInt(mhz*outdiv);
	const Fixed fs_KHz		  = DIV(fs_Hz, FixedInt(1000));
	const Fixed spread_depth  = DIV(FixedInt(5),FixedInt(1000));
		
	Fixed internal_ref, bwadj, unknown, spread_attn, sslope, srate, spread_freq;
	int prescale_div, pll_bwadj, pll_refdiv, pll_fbdiv, pll_sfreq, pll_sslope, pll_outdiv;

	spread_freq  = mhz*outdiv*spread_depth*2;
	
	prescale_div = fref_MHz / spread_freq;

	if (fref_MHz % spread_freq) prescale_div++;

	internal_ref = DIV(fref_MHz, FixedInt(prescale_div));
	bwadj        = DIV(MUL(fout_MHz, FixedInt(prescale_div)), fref_MHz);
	unknown      = DIV(internal_ref, FixedInt(35));
	spread_attn  = (fs_KHz > unknown) ? DIV(unknown, fs_KHz) : C_ONE;
	sslope       = DIV( MUL(MUL(spread_depth*4, bwadj),MUL(fs_KHz,bwadj)), MUL(fout_MHz, spread_attn) );	
	srate        = DIV(internal_ref, fs_KHz*2);

	pll_refdiv   = prescale_div - 1;
	pll_outdiv   = outdiv - 1;
	pll_fbdiv    = bwadj>>(FRAC_BITS-15);
	pll_bwadj    = ToInt(bwadj-C_ONE);
	pll_sfreq    = ToInt(srate);
	pll_sslope   = sslope>>(FRAC_BITS-15);
	
//    putstr(debug_uart, "Attempting to set PLLA to "); 
    putstr(debug_uart, "CPU Clock set to "); 
    putstr(debug_uart, ultodec((unsigned)mhz));
    putstr(debug_uart, "MHz ..."); 	
	
	plla_configure(pll_refdiv, pll_outdiv, pll_bwadj, pll_fbdiv, pll_bwadj, pll_sslope);
}

#endif
