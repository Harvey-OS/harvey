/*
 * MultiProcessor Specification Version 1.[14].
 */

enum {					/* table entry types */
	PcmpPROCESSOR	= 0x00,		/* one entry per processor */
	PcmpBUS		= 0x01,		/* one entry per bus */
	PcmpIOAPIC	= 0x02,		/* one entry per I/O APIC */
	PcmpIOINTR	= 0x03,		/* one entry per bus interrupt source */
	PcmpLINTR	= 0x04,		/* one entry per system interrupt source */

	PcmpSASM	= 0x80,
	PcmpHIERARCHY	= 0x81,
	PcmpCBASM	= 0x82,

					/* PCMPprocessor and PCMPioapic flags */
	PcmpEN		= 0x01,		/* enabled */
	PcmpBP		= 0x02,		/* bootstrap processor */

					/* PCMPiointr and PCMPlintr flags */
	PcmpPOMASK	= 0x03,		/* polarity conforms to specifications of bus */
	PcmpHIGH	= 0x01,		/* active high */
	PcmpLOW		= 0x03,		/* active low */
	PcmpELMASK	= 0x0C,		/* trigger mode of APIC input signals */
	PcmpEDGE	= 0x04,		/* edge-triggered */
	PcmpLEVEL	= 0x0C,		/* level-triggered */

					/* PCMPiointr and PCMPlintr interrupt type */
	PcmpINT		= 0x00,		/* vectored interrupt from APIC Rdt */
	PcmpNMI		= 0x01,		/* non-maskable interrupt */
	PcmpSMI		= 0x02,		/* system management interrupt */
	PcmpExtINT	= 0x03,		/* vectored interrupt from external PIC */

					/* PCMPsasm addrtype */
	PcmpIOADDR	= 0x00,		/* I/O address */
	PcmpMADDR	= 0x01,		/* memory address */
	PcmpPADDR	= 0x02,		/* prefetch address */

					/* PCMPhierarchy info */
	PcmpSD		= 0x01,		/* subtractive decode bus */

					/* PCMPcbasm modifier */
	PcmpPR		= 0x01,		/* predefined range list */
};

enum {
	MaxAPICNO	= 254,		/* 255 is physical broadcast */
};

enum {					/* I/O APIC registers */
	IoapicID	= 0x00,		/* ID */
	IoapicVER	= 0x01,		/* version */
	IoapicARB	= 0x02,		/* arbitration ID */
	IoapicRDT	= 0x10,		/* redirection table */
};

/*
 * Common bits for
 *	I/O APIC Redirection Table Entry;
 *	Local APIC Local Interrupt Vector Table;
 *	Local APIC Inter-Processor Interrupt;
 *	Local APIC Timer Vector Table.
 */
enum {
	ApicFIXED	= 0x00000000,	/* [10:8] Delivery Mode */
	ApicLOWEST	= 0x00000100,	/* Lowest priority */
	ApicSMI		= 0x00000200,	/* System Management Interrupt */
	ApicRR		= 0x00000300,	/* Remote Read */
	ApicNMI		= 0x00000400,
	ApicINIT	= 0x00000500,	/* INIT/RESET */
	ApicSTARTUP	= 0x00000600,	/* Startup IPI */
	ApicExtINT	= 0x00000700,

	ApicPHYSICAL	= 0x00000000,	/* [11] Destination Mode (RW) */
	ApicLOGICAL	= 0x00000800,

	ApicDELIVS	= 0x00001000,	/* [12] Delivery Status (RO) */
	ApicHIGH	= 0x00000000,	/* [13] Interrupt Input Pin Polarity (RW) */
	ApicLOW		= 0x00002000,
	ApicRemoteIRR	= 0x00004000,	/* [14] Remote IRR (RO) */
	ApicEDGE	= 0x00000000,	/* [15] Trigger Mode (RW) */
	ApicLEVEL	= 0x00008000,
	ApicIMASK	= 0x00010000,	/* [16] Interrupt Mask */
};

