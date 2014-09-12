#if (NAS_VERSION == 820)
#include <common.h>

#define SATA_PHY_ASIC_STAT  (0x44900000)
#define SATA_PHY_ASIC_DATA  (0x44900004)
#define SATA_PHY_ASIC_JTAG  (0x44900008)
#define SATA_PHY_FPGA_JTAG  (0x44900018)

/**
 * initialise functions and macros for ASIC implementation 
 */
#define PH_GAIN         2
#define FR_GAIN         3
#define PH_GAIN_OFFSET  6
#define FR_GAIN_OFFSET  8
#define PH_GAIN_MASK  (0x3 << PH_GAIN_OFFSET)
#define FR_GAIN_MASK  (0x3 << FR_GAIN_OFFSET)

#define CR_READ_ENABLE  (1<<16)
#define CR_WRITE_ENABLE (1<<17)
#define CR_CAP_DATA     (1<<18)

static void wait_cr_ack(void){
	while ((readl(SATA_PHY_ASIC_STAT) >> 16) & 0x1f)
		/* wait for an ack bit to be set */ ;
}

static unsigned short read_cr(unsigned short address) {
	writel(address, SATA_PHY_ASIC_STAT);
	wait_cr_ack();
	writel(CR_READ_ENABLE, SATA_PHY_ASIC_DATA);
	wait_cr_ack();
	return readl(SATA_PHY_ASIC_STAT);
}

static void write_cr(unsigned short data, unsigned short address) {
	writel(address, SATA_PHY_ASIC_STAT);
	wait_cr_ack();
	writel((data | CR_CAP_DATA), SATA_PHY_ASIC_DATA);
	wait_cr_ack();
	writel(CR_WRITE_ENABLE, SATA_PHY_ASIC_DATA);
	wait_cr_ack();
	return ;
}

void workaround5458(void){
	unsigned i;
	
	for (i=0; i<2;i++){
		unsigned short rx_control = read_cr( 0x201d + (i<<8));
		rx_control &= ~(PH_GAIN_MASK | FR_GAIN_MASK);
		rx_control |= PH_GAIN << PH_GAIN_OFFSET;
		rx_control |= FR_GAIN << FR_GAIN_OFFSET;
		write_cr( rx_control, 0x201d+(i<<8));
	}
}

/**
 * initialise functions and macros for FPGA implementation 
 */

#define TMS     (1 << 1) 
#define TDI     (1 << 2)
#define TCK     (1 << 0)
#define nTRST    (1 << 4)
#define PAUSE   64


/* Synopsys phy setup */
struct bitpattern {
    signed char tms;
    signed char tdi;
} __attribute__((packed));

/** Right Port */
static const struct bitpattern SATA_RIGHT_65[] =
{
    //
    // Synopsys JTAG Test Vector
    //
    // 13-Aug-2008 17:22:15
    //
    // JTAG Chain: ANI_PART only part in chain
    //
    // Vector: TMS_TDI_TDO
    // TCK pulse up/down implied
    // TMS & TDI sampled by part on rising TCK edge
    // TDO changes on falling
    //
    // JTAG RUN VECT
    {1,1},
    {1,1},
    {1,1},
    {1,1},
    {1,1},
    {1,1},
    {1,1},
    {0,0},
    // JTAG RUN VECT - done
    // writing 45 bit register with reset high
    // writing the write_dr register
    // // Enter ovrdreg in IR register value is 10100001
    // //Move from rti to shift-ir
    // JTAG RUN VECT
    {1,0},
    {1,0},
    {0,0},
    {0,0},
    // JTAG RUN VECT - done
    // //Enter in IR command
    // JTAG RUN VECT
    // core UP3, location 2 in chain
    {0,1},
    {0,0},
    {0,0},
    {0,0},
    {0,0},
    {0,1},
    {0,0},
    {0,1},
    // Adding instruction to put core OTHER chain location 1 in bypass
    {0,1},
    {0,1},
    {0,1},
    {0,1},
    {0,1},
    {0,1},
    {0,1},
    {1,1},
    // JTAG RUN VECT - done
    // //Move from exit-ir to rti
    // JTAG RUN VECT
    {1,0},
    {0,0},
    // JTAG RUN VECT - done
    // JTAG RUN VECT
    {1,0},
    {0,0},
    {0,0},
    // JTAG RUN VECT - done
    // JTAG RUN VECT
    // core UP3, location 2 in chain
    {0,0},
    {0,1},
    {0,1},
    {0,0},
#ifdef PHILS_FIX
    // 130nm Oxsemi: Phil's fix: LOS LVL LSB First
    {0,1},
    {0,1},
    {0,1},
    {0,0},
    {0,0},
#else
    {0,1},
    {0,1},
    {0,0},
    {0,0},
    {0,1},
#endif
    {0,0},
    {0,0},
    {0,1},
    {0,1},
    {0,0},
    {0,0},
    {0,0},
    {0,0},
    {0,1},
    {0,0},
    {0,0},
    {0,1},
#ifdef PHILS_FIX
    // 130nm Oxsemi: Phil's fix: LOS LVL End
    {0,1},
    {0,1},
    {0,0},
    {0,1},
    {0,1},
#else
    {0,0},
    {0,0},
    {0,1},
    {0,1},
    {0,1},
#endif
    {0,0},
    {0,1},
    {0,1},
    {0,1},
    {0,0},
    {0,0},
    {0,1},
    {0,0},
    {0,0},
    {0,1},
    {0,0},
    {0,1},
    {0,1},
    {0,0},
    {0,0},
    {0,1},
    {0,0},
#ifdef SYNOPSYS_JTAG_TEST_VECTOR_SS
    {0,1},
#else
    {0,0},
#endif
    {0,0},
    // Adding bypass vectors for core OTHER chain location 1
    {1,0},
    {1,0},
    {0,0},
    // JTAG RUN VECT - done
    // **Pause 0.100000 ms
    {64,64},
    // writing 45 bit register with reset low
    // writing the write_dr register
    // // Enter ovrdreg in IR register value is 10100001
    // //Move from rti to shift-ir
    // JTAG RUN VECT
    {1,0},
    {1,0},
    {0,0},
    {0,0},
    // JTAG RUN VECT - done
    // //Enter in IR command
    // JTAG RUN VECT
    // core UP3, location 2 in chain
    {0,1},
    {0,0},
    {0,0},
    {0,0},
    {0,0},
    {0,1},
    {0,0},
    {0,1},
    // Adding instruction to put core OTHER chain location 1 in bypass
    {0,1},
    {0,1},
    {0,1},
    {0,1},
    {0,1},
    {0,1},
    {0,1},
    {1,1},
    // JTAG RUN VECT - done
    // //Move from exit-ir to rti
    // JTAG RUN VECT
    {1,0},
    {0,0},
    // JTAG RUN VECT - done
    // JTAG RUN VECT
    {1,0},
    {0,0},
    {0,0},
    // JTAG RUN VECT - done
    // JTAG RUN VECT
    // core UP3, location 2 in chain
    {0,0},
    {0,1},
    {0,1},
    {0,0},
#ifdef PHILS_FIX
    // 130nm Oxsemi: Phil's fix: LOS LVL LSB First
    {0,1},
    {0,1},
    {0,1},
    {0,0},
    {0,0},
#else
    {0,1},
    {0,1},
    {0,0},
    {0,0},
    {0,1},
#endif
    {0,0},
    {0,0},
    {0,1},
    {0,1},
    {0,0},
    {0,0},
    {0,0},
    {0,0},
    {0,1},
    {0,0},
    {0,0},
    {0,1},
#ifdef PHILS_FIX
    // 130nm Oxsemi: Phil's fix: LOS LVL End
    {0,1},
    {0,1},
    {0,0},
    {0,1},
    {0,1},
#else
    {0,0},
    {0,0},
    {0,1},
    {0,1},
    {0,1},
#endif
    {0,0},
    {0,1},
    {0,0},
    {0,1},
    {0,0},
    {0,0},
    {0,1},
    {0,0},
    {0,0},
    {0,1},
    {0,0},
    {0,1},
    {0,1},
    {0,0},
    {0,0},
    {0,1},
    {0,0},
#ifdef SYNOPSYS_JTAG_TEST_VECTOR_SS
    {0,1},
#else
    {0,0},
#endif
    {0,0},
    // Adding bypass vectors for core OTHER chain location 1
    {1,0},
    {1,0},
    {0,0},
    // JTAG RUN VECT - done
    // **Pause 0.100000 ms
    {64,64},
    // // Enter apucrsel in IR register value is 111101
    // //Move from rti to shift-ir
    // JTAG RUN VECT
    {1,0},
    {1,0},
    {0,0},
    {0,0},
    // JTAG RUN VECT - done
    // //Enter in IR command
    // JTAG RUN VECT
    // core UP3, location 2 in chain
    {0,1},
    {0,0},
    {0,1},
    {0,1},
    {0,1},
    {0,1},
    {0,0},
    {0,0},
    // Adding instruction to put core OTHER chain location 1 in bypass
    {0,1},
    {0,1},
    {0,1},
    {0,1},
    {0,1},
    {0,1},
    {0,1},
    {1,1},
    // JTAG RUN VECT - done
    // //Move from exit-ir to rti
    // JTAG RUN VECT
    {1,0},
    {0,0},
    // JTAG RUN VECT - done
    // Setting up insel and outsel registers
    // Write val 4 to insel.cfg_insel0 (addr 0x0108), bottom=0 width=3, old=0x7676 new=0x7674
    // REG SET: addr=0x0108  val=0x7674
    // ADDR OP (0x0108)
    {1,0},
    {0,0},
    {0,0},
    // Shift-DR
    // core UP3, location 2 in chain
    {0,0},
    {0,0},
    {0,0},
    {0,1},
    {0,0},
    {0,0},
    {0,0},
    {0,0},
    {0,1},
    {0,0},
    {0,0},
    {0,0},
    {0,0},
    {0,0},
    {0,0},
    {0,0},
    {0,0},
    {0,0},
    // Adding bypass vectors for core OTHER chain location 1
    {1,0},
    // Shift-DR - done
    {1,0},
    {0,0},
    // ADDR OP (0x0108) - done
    // WRITE OP (0x7674)
    {1,0},
    {0,0},
    {0,0},
    // Shift-DR
    // core UP3, location 2 in chain
    {0,0},
    {0,0},
    {0,1},
    {0,0},
    {0,1},
    {0,1},
    {0,1},
    {0,0},
    {0,0},
    {0,1},
    {0,1},
    {0,0},
    {0,1},
    {0,1},
    {0,1},
    {0,0},
    {0,1},
    {0,0},
    // Adding bypass vectors for core OTHER chain location 1
    {1,0},
    // Shift-DR - done
    {1,0},
    {0,0},
    // WRITE OP (0x0108) - done
    // Write val 4 to insel.cfg_insel1 (addr 0x0108), bottom=4 width=3, old=0x7674 new=0x7644
    // REG SET: addr=0x0108  val=0x7644
    // ADDR OP (0x0108)
    {1,0},
    {0,0},
    {0,0},
    // Shift-DR
    // core UP3, location 2 in chain
    {0,0},
    {0,0},
    {0,0},
    {0,1},
    {0,0},
    {0,0},
    {0,0},
    {0,0},
    {0,1},
    {0,0},
    {0,0},
    {0,0},
    {0,0},
    {0,0},
    {0,0},
    {0,0},
    {0,0},
    {0,0},
    // Adding bypass vectors for core OTHER chain location 1
    {1,0},
    // Shift-DR - done
    {1,0},
    {0,0},
    // ADDR OP (0x0108) - done
    // WRITE OP (0x7644)
    {1,0},
    {0,0},
    {0,0},
    // Shift-DR
    // core UP3, location 2 in chain
    {0,0},
    {0,0},
    {0,1},
    {0,0},
    {0,0},
    {0,0},
    {0,1},
    {0,0},
    {0,0},
    {0,1},
    {0,1},
    {0,0},
    {0,1},
    {0,1},
    {0,1},
    {0,0},
    {0,1},
    {0,0},
    // Adding bypass vectors for core OTHER chain location 1
    {1,0},
    // Shift-DR - done
    {1,0},
    {0,0},
    // WRITE OP (0x0108) - done
    // Write val 6 to insel.cfg_insel2 (addr 0x0108), bottom=8 width=3, old=0x7644 new=0x7644
    // REG SET: addr=0x0108  val=0x7644
    // ADDR OP (0x0108)
    {1,0},
    {0,0},
    {0,0},
    // Shift-DR
    // core UP3, location 2 in chain
    {0,0},
    {0,0},
    {0,0},
    {0,1},
    {0,0},
    {0,0},
    {0,0},
    {0,0},
    {0,1},
    {0,0},
    {0,0},
    {0,0},
    {0,0},
    {0,0},
    {0,0},
    {0,0},
    {0,0},
    {0,0},
    // Adding bypass vectors for core OTHER chain location 1
    {1,0},
    // Shift-DR - done
    {1,0},
    {0,0},
    // ADDR OP (0x0108) - done
    // WRITE OP (0x7644)
    {1,0},
    {0,0},
    {0,0},
    // Shift-DR
    // core UP3, location 2 in chain
    {0,0},
    {0,0},
    {0,1},
    {0,0},
    {0,0},
    {0,0},
    {0,1},
    {0,0},
    {0,0},
    {0,1},
    {0,1},
    {0,0},
    {0,1},
    {0,1},
    {0,1},
    {0,0},
    {0,1},
    {0,0},
    // Adding bypass vectors for core OTHER chain location 1
    {1,0},
    // Shift-DR - done
    {1,0},
    {0,0},
    // WRITE OP (0x0108) - done
    // Write val 7 to insel.cfg_insel3 (addr 0x0108), bottom=12 width=3, old=0x7644 new=0x7644
    // REG SET: addr=0x0108  val=0x7644
    // ADDR OP (0x0108)
    {1,0},
    {0,0},
    {0,0},
    // Shift-DR
    // core UP3, location 2 in chain
    {0,0},
    {0,0},
    {0,0},
    {0,1},
    {0,0},
    {0,0},
    {0,0},
    {0,0},
    {0,1},
    {0,0},
    {0,0},
    {0,0},
    {0,0},
    {0,0},
    {0,0},
    {0,0},
    {0,0},
    {0,0},
    // Adding bypass vectors for core OTHER chain location 1
    {1,0},
    // Shift-DR - done
    {1,0},
    {0,0},
    // ADDR OP (0x0108) - done
    // WRITE OP (0x7644)
    {1,0},
    {0,0},
    {0,0},
    // Shift-DR
    // core UP3, location 2 in chain
    {0,0},
    {0,0},
    {0,1},
    {0,0},
    {0,0},
    {0,0},
    {0,1},
    {0,0},
    {0,0},
    {0,1},
    {0,1},
    {0,0},
    {0,1},
    {0,1},
    {0,1},
    {0,0},
    {0,1},
    {0,0},
    // Adding bypass vectors for core OTHER chain location 1
    {1,0},
    // Shift-DR - done
    {1,0},
    {0,0},
    // WRITE OP (0x0108) - done
    // Write val 2 to outsel.cfg_outsela (addr 0x0109), bottom=0 width=3, old=0x0010 new=0x0012
    // REG SET: addr=0x0109  val=0x0012
    // ADDR OP (0x0109)
    {1,0},
    {0,0},
    {0,0},
    // Shift-DR
    // core UP3, location 2 in chain
    {0,1},
    {0,0},
    {0,0},
    {0,1},
    {0,0},
    {0,0},
    {0,0},
    {0,0},
    {0,1},
    {0,0},
    {0,0},
    {0,0},
    {0,0},
    {0,0},
    {0,0},
    {0,0},
    {0,0},
    {0,0},
    // Adding bypass vectors for core OTHER chain location 1
    {1,0},
    // Shift-DR - done
    {1,0},
    {0,0},
    // ADDR OP (0x0109) - done
    // WRITE OP (0x0012)
    {1,0},
    {0,0},
    {0,0},
    // Shift-DR
    // core UP3, location 2 in chain
    {0,0},
    {0,1},
    {0,0},
    {0,0},
    {0,1},
    {0,0},
    {0,0},
    {0,0},
    {0,0},
    {0,0},
    {0,0},
    {0,0},
    {0,0},
    {0,0},
    {0,0},
    {0,0},
    {0,1},
    {0,0},
    // Adding bypass vectors for core OTHER chain location 1
    {1,0},
    // Shift-DR - done
    {1,0},
    {0,0},
    // WRITE OP (0x0109) - done
    // Write val 3 to outsel.cfg_outselb (addr 0x0109), bottom=4 width=3, old=0x0012 new=0x0032
    // REG SET: addr=0x0109  val=0x0032
    // ADDR OP (0x0109)
    {1,0},
    {0,0},
    {0,0},
    // Shift-DR
    // core UP3, location 2 in chain
    {0,1},
    {0,0},
    {0,0},
    {0,1},
    {0,0},
    {0,0},
    {0,0},
    {0,0},
    {0,1},
    {0,0},
    {0,0},
    {0,0},
    {0,0},
    {0,0},
    {0,0},
    {0,0},
    {0,0},
    {0,0},
    // Adding bypass vectors for core OTHER chain location 1
    {1,0},
    // Shift-DR - done
    {1,0},
    {0,0},
    // ADDR OP (0x0109) - done
    // WRITE OP (0x0032)
    {1,0},
    {0,0},
    {0,0},
    // Shift-DR
    // core UP3, location 2 in chain
    {0,0},
    {0,1},
    {0,0},
    {0,0},
    {0,1},
    {0,1},
    {0,0},
    {0,0},
    {0,0},
    {0,0},
    {0,0},
    {0,0},
    {0,0},
    {0,0},
    {0,0},
    {0,0},
    {0,1},
    {0,0},
    // Adding bypass vectors for core OTHER chain location 1
    {1,0},
    // Shift-DR - done
    {1,0},
    {0,0},
    // WRITE OP (0x0109) - done
    // added ip reset
    // // Enter crsel in IR register value is 110001
    // //Move from rti to shift-ir
    // JTAG RUN VECT
    {1,0},
    {1,0},
    {0,0},
    {0,0},
    // JTAG RUN VECT - done
    // //Enter in IR command
    // JTAG RUN VECT
    // core UP3, location 2 in chain
    {0,1},
    {0,0},
    {0,0},
    {0,0},
    {0,1},
    {0,1},
    {0,0},
    {0,0},
    // Adding instruction to put core OTHER chain location 1 in bypass
    {0,1},
    {0,1},
    {0,1},
    {0,1},
    {0,1},
    {0,1},
    {0,1},
    {1,1},
    // JTAG RUN VECT - done
    // //Move from exit-ir to rti
    // JTAG RUN VECT
    {1,0},
    {0,0},
    // JTAG RUN VECT - done
    // Write val 1 to reset (addr 0x7f3f), bottom=0 width=16, old=0x0000 new=0x0001
    // REG SET: addr=0x7f3f  val=0x0001
    // ADDR OP (0x7f3f)
    {1,0},
    {0,0},
    {0,0},
    // Shift-DR
    // core UP3, location 2 in chain
    {0,1},
    {0,1},
    {0,1},
    {0,1},
    {0,1},
    {0,1},
    {0,0},
    {0,0},
    {0,1},
    {0,1},
    {0,1},
    {0,1},
    {0,1},
    {0,1},
    {0,1},
    {0,0},
    {0,0},
    {0,0},
    // Adding bypass vectors for core OTHER chain location 1
    {1,0},
    // Shift-DR - done
    {1,0},
    {0,0},
    // ADDR OP (0x7f3f) - done
    // WRITE OP (0x0001)
    {1,0},
    {0,0},
    {0,0},
    // Shift-DR
    // core UP3, location 2 in chain
    {0,1},
    {0,0},
    {0,0},
    {0,0},
    {0,0},
    {0,0},
    {0,0},
    {0,0},
    {0,0},
    {0,0},
    {0,0},
    {0,0},
    {0,0},
    {0,0},
    {0,0},
    {0,0},
    {0,1},
    {0,0},
    // Adding bypass vectors for core OTHER chain location 1
    {1,0},
    // Shift-DR - done
    {1,0},
    {0,0},
    // WRITE OP (0x7f3f) - done
    // Write val 0 to reset (addr 0x7f3f), bottom=0 width=16, old=0x0001 new=0x0000
    // REG SET: addr=0x7f3f  val=0x0000
    // ADDR OP (0x7f3f)
    {1,0},
    {0,0},
    {0,0},
    // Shift-DR
    // core UP3, location 2 in chain
    {0,1},
    {0,1},
    {0,1},
    {0,1},
    {0,1},
    {0,1},
    {0,0},
    {0,0},
    {0,1},
    {0,1},
    {0,1},
    {0,1},
    {0,1},
    {0,1},
    {0,1},
    {0,0},
    {0,0},
    {0,0},
    // Adding bypass vectors for core OTHER chain location 1
    {1,0},
    // Shift-DR - done
    {1,0},
    {0,0},
    // ADDR OP (0x7f3f) - done
    // WRITE OP (0x0000)
    {1,0},
    {0,0},
    {0,0},
    // Shift-DR
    // core UP3, location 2 in chain
    {0,0},
    {0,0},
    {0,0},
    {0,0},
    {0,0},
    {0,0},
    {0,0},
    {0,0},
    {0,0},
    {0,0},
    {0,0},
    {0,0},
    {0,0},
    {0,0},
    {0,0},
    {0,0},
    {0,1},
    {0,0},
    // Adding bypass vectors for core OTHER chain location 1
    {1,0},
    // Shift-DR - done
    {1,0},
    {0,0},
    // WRITE OP (0x7f3f) - done
    // end of added ip reset
    // **************************************************
    // BEGIN PHY INIT SECTION.
    // BEFORE THIS POINT, THE PHY SHOULD HAVE BEEN RESET.
    // **************************************************
    // ***************************
    // BEGIN TEST SECTION
    // ***************************
    //
    //
    // 13-Aug-2008 17:20:33
    {-1,-1}   // Oxsemi: end of stream
};

static void SetTMSTDI(unsigned long tms, unsigned long tdi) {
    unsigned long reg;
    
    /* set bits */
    reg = readl(SATA_PHY_FPGA_JTAG);
    reg &= ~(TMS|TDI);
    reg |= tms ? TMS : 0;
    reg |= tdi ? TDI : 0;
    writel( reg, SATA_PHY_FPGA_JTAG);

    /* toggle clock with 100us delay */
    reg |= TCK;
    writel( reg, SATA_PHY_FPGA_JTAG);

 //   udelay(100);

    reg &= ~TCK;
    writel( reg, SATA_PHY_FPGA_JTAG);
	/* wait 100us before next bit */
//    udelay(100);
}

static void Stream(void) {
    const struct bitpattern* bp = &(SATA_RIGHT_65[0]);
    
    do {
        if (bp->tms == PAUSE) {
            udelay(100);
        } else {
            SetTMSTDI(bp->tms, bp->tdi);
        }
        ++bp;
    } while (bp->tms != -1);
}

static void ResetDll(void) {
    unsigned long reg;
    /* hold reset dawn for 1 ms */
    reg = readl(SATA_PHY_FPGA_JTAG);
    reg &= ~nTRST;
    writel( reg, SATA_PHY_FPGA_JTAG);
    
    udelay(1000);

    reg |= nTRST;
    writel( reg, SATA_PHY_FPGA_JTAG);
}

/**
 * initialise ASIC and FPGA
 */

void EnableSATAPhy(void) {
    Stream();
    ResetDll();
	workaround5458();
}

#endif // NAS_VERSION == 820
