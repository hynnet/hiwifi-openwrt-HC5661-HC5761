#ifndef _oxnas_ehci_regs_h
#define _oxnas_ehci_regs_h

struct oxnas_ehci_regs {
	u32 		command;
	u32		status;
	u32		intr_enable;
	u32		frame_index;
	u32		segment;
	u32		frame_list;
	u32		async_next;
	/* OXNAS specific { */
	u32		ttctrl;
#define OXNAS_TT_CONTROL_TTAC  (1<<1)   /* clear transaction controller */
#define OXNAS_TT_CONTROL_TTAS  (1<<0)   /* transaction controller active state */
	u32 burstsize;
	u32 txfilltuning;
	u32 txttfilltuning;
	u32 reserved_1;
	u32 ulpi_viewport;
	u32 reserved_2;
	u32 endpknack;
	u32 endptnalek;
	/* } */

	u32		configured_flag;
	u32		port_status[8];
	u32		otgsc;
	u32		usbmode;

	/* OXNAS specific { */
	u32 endptsetupstack;
	u32 endptprime;
	u32 endptflush;
	u32 endptstat;
	u32 endptcomplete;
	u32 endptctrl[8];
	/* } */
};

/* TDI transaction translator status register and busy bit */
#define ONXAS_TT_BUSY 0x1
#define OXNAS_TT_STATUS  (0x15c-0x140)

#endif
