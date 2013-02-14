/*
 * Memory and machine-specific definitions.  Used in C and assembler.
 */
#define KiB		1024u			/* Kibi 0x0000000000000400 */
#define MiB		1048576u		/* Mebi 0x0000000000100000 */
#define GiB		1073741824u		/* Gibi 000000000040000000 */

/*
 * Not sure where these macros should go.
 * This probably isn't right but will do for now.
 * The macro names are problematic too.
 */
/*
 * In BITN(o), 'o' is the bit offset in the register.
 * For multi-bit fields use F(v, o, w) where 'v' is the value
 * of the bit-field of width 'w' with LSb at bit offset 'o'.
 */
#define BITN(o)		(1<<(o))
#define F(v, o, w)	(((v) & ((1<<(w))-1))<<(o))

/*
 * Sizes
 */
#define	BY2PG		(4*KiB)			/* bytes per page */
#define	PGSHIFT		12			/* log(BY2PG) */

#define	MAXMACH		1			/* max # cpus system can run */
#define	MACHSIZE	BY2PG

#define KSTKSIZE	(16*KiB)			/* was 8K */
#define STACKALIGN(sp)	((sp) & ~3)		/* bug: assure with alloc */

/*
 * Address spaces.
 * KTZERO is used by kprof and dumpstack (if any).
 *
 * KZERO (0xc0000000) is mapped to physical 0x80000000 (start of dram).
 * u-boot claims to occupy the first 3 MB of dram, but we're willing to
 * step on it once we're loaded.  Expect plan9.ini in the first 64K past 3MB.
 *
 * L2 PTEs are stored in 1K before Mach (11K to 12K above KZERO).
 * cpu0's Mach struct is at L1 - MACHSIZE(4K) to L1 (12K to 16K above KZERO).
 * L1 PTEs are stored from L1 to L1+32K (16K to 48K above KZERO).
 * KTZERO may be anywhere after that (but probably shouldn't collide with
 * u-boot).
 * This should leave over 8K from KZERO to L2 PTEs.
 */
#define	KSEG0		0xC0000000		/* kernel segment */
/* mask to check segment; good for 512MB dram */
#define	KSEGM		0xE0000000
#define	KZERO		KSEG0			/* kernel address space */
#define L1		(KZERO+16*KiB)		/* tt ptes: 16KiB aligned */
#define CONFADDR	(KZERO+0x300000)	/* unparsed plan9.ini */
/* KTZERO must match loadaddr in mkfile */
#define	KTZERO		(KZERO+0x310000)	/* kernel text start */

#define	UZERO		0			/* user segment */
#define	UTZERO		(UZERO+BY2PG)		/* user text start */
#define UTROUND(t)	ROUNDUP((t), BY2PG)
/* moved USTKTOP down to 512MB to keep MMIO space out of user space. */
#define	USTKTOP		0x20000000		/* user segment end +1 */
#define	USTKSIZE	(8*1024*1024)		/* user stack size */
#define	TSTKTOP		(USTKTOP-USTKSIZE)	/* sysexec temporary stack */
#define	TSTKSIZ	 	256

/* address at which to copy and execute rebootcode */
#define	REBOOTADDR	KADDR(0x100)

/*
 * Legacy...
 */
#define BLOCKALIGN	32			/* only used in allocb.c */
#define KSTACK		KSTKSIZE

/*
 * Sizes
 */
#define BI2BY		8			/* bits per byte */
#define BY2SE		4
#define BY2WD		4
#define BY2V		8			/* only used in xalloc.c */

#define CACHELINESZ	64			/* bytes per cache line */
#define	PTEMAPMEM	(1024*1024)
#define	PTEPERTAB	(PTEMAPMEM/BY2PG)
#define	SEGMAPSIZE	1984			/* magic 16*124 */
#define	SSEGMAPSIZE	16			/* magic */
#define	PPN(x)		((x)&~(BY2PG-1))	/* pure page number? */

/*
 * With a little work these move to port.
 */
#define	PTEVALID	(1<<0)
#define	PTERONLY	0
#define	PTEWRITE	(1<<1)
#define	PTEUNCACHED	(1<<2)
#define PTEKERNEL	(1<<3)

/*
 * Physical machine information from here on.
 */

/* gpmc-controlled address space 0—1G */
#define PHYSNAND	1		/* cs0 is onenand flash */
#define PHYSETHER	0x2c000000

#define PHYSIO		0x48000000	/* L4 ctl */

#define PHYSSCM		0x48002000	/* system control module */

/* core control pad cfg		0x48002030—0x480021e4, */
/* core control d2d pad cfg	0x480021e4—0x48002264 */
#define PHYSSCMPCONF	0x48002270	/* general device config */
#define PHYSOMAPSTS	0x4800244c	/* standalone short: has l2 size */
/* core control pad cfg (2)	0x480025d8—0x480025fc */
#define PHYSSWBOOTCFG	0x48002910	/* sw booting config */
/* wakeup control pad cfg	0x48002a00—0x48002a54 */

#define PHYSSCMMPU	0x48004900	/* actually CPU */
#define PHYSSCMCORE	0x48004a00
#define PHYSSCMWKUP	0x48004c00
#define PHYSSCMPLL	0x48004d00	/* clock ctl for dpll[3-5] */
#define PHYSSCMDSS	0x48004e00
#define PHYSSCMPER	0x48005000
#define PHYSSCMUSB	0x48005400

#define PHYSL4CORE	0x48040100	/* l4 ap */
#define PHYSDSS		0x48050000	/* start of dss registers */
#define PHYSDISPC	0x48050400
#define PHYSGFX		0x48050480	/* part of dispc */

#define PHYSSDMA	0x48056000	/* system dma */
#define PHYSDMA		0x48060000

#define PHYSUSBTLL	0x48062000	/* usb: transceiver-less link */
#define PHYSUHH		0x48064000	/* usb: `high-speed usb host' ctlr or subsys */
#define PHYSOHCI	0x48064400	/* usb 1.0: slow */
#define PHYSEHCI	0x48064800	/* usb 2.0: medium */
#define PHYSUART0	0x4806a000
#define PHYSUART1	0x4806c000
#define PHYSMMCHS1	0x4809c000	/* mmc/sdio */
#define PHYSUSBOTG	0x480ab000	/* on-the-go usb */
#define PHYSMMCHS3	0x480ad000
#define PHYSMMCHS2	0x480b4000

#define PHYSINTC	0x48200000	/* interrupt controller */

#define PHYSPRMIVA2	0x48206000	/* prm iva2 regs */
/* 48306d40 sys_clkin_sel */
#define PHYSPRMGLBL	0x48307200	/* prm global regs */
#define PHYSPRMWKUSB	0x48307400

#define PHYSCNTRL	0x4830a200	/* SoC id, etc. */
#define PHYSWDT1	0x4830c000	/* wdt1, not on GP omaps */

#define PHYSGPIO1	0x48310000	/* contains dss gpio */

#define PHYSWDOG	0x48314000	/* watchdog timer, wdt2 */
#define PHYSWDT2	0x48314000	/* watchdog timer, wdt2 */
#define PHYSTIMER1	0x48318000

#define PHYSL4WKUP	0x48328100	/* l4 wkup */
#define PHYSL4PER	0x49000100	/* l4 per */

#define PHYSCONS	0x49020000	/* uart console (third one) */

#define PHYSWDT3	0x49030000	/* wdt3 */
#define PHYSTIMER2	0x49032000
#define PHYSTIMER3	0x49034000
#define PHYSGPIO5	0x49056000
#define PHYSGPIO6	0x49058000	/* contains igep ether gpio */

#define PHYSIOEND	0x49100000	/* end of PHYSIO identity map */

#define PHYSL4EMU	0x54006100	/* l4 emu */
#define PHYSL4PROT	0x54728000	/* l4 protection regs */

#define PHYSL3		0x68000000	/* l3 interconnect control */
#define PHYSL3GPMCCFG	0x68002000	/* l3 gpmc target port agent cfg */
#define PHYSL3USB	0x68004000	/* l3 regs for usb */
#define PHYSL3USBOTG	0x68004400	/* l3 regs for usb otg */
/* (target port) protection registers */
#define PHYSL3PMRT	0x68010000	/* l3 PM register target prot. */
#define PHYSL3GPMCPM	0x68012400	/* l3 gpmc target port protection */
#define PHYSL3OCTRAM	0x68012800	/* l3 ocm ram */
#define PHYSL3OCTROM	0x68012c00	/* l3 ocm rom */
#define PHYSL3MAD2D	0x68013000	/* l3 die-to-die */
#define PHYSL3IVA	0x68014000	/* l3 die-to-die */

#define PHYSSMS		0x6c000000	/* cfg regs: sms addr space 2 */
#define PHYSDRC		0x6d000000	/* sdram ctlr, addr space 3 */
#define PHYSGPMC	0x6e000000	/* flash, non-dram memory ctlr */

#define PHYSDRAM	0x80000000

#define VIRTNAND	0x20000000	/* fixed by u-boot */
#define VIRTIO		PHYSIO
