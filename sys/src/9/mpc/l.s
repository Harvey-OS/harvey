#include	"mem.h"

/*
 * common ppc special purpose registers
 */
#define DSISR	18
#define DAR		19	/* Data Address Register */
#define DEC		22	/* Decrementer */
#define SRR0	26	/* Saved Registers (exception) */
#define SRR1	27
#define SPRG0	272	/* Supervisor Private Registers */
#define SPRG1	273
#define SPRG2	274
#define SPRG3	275
#define TBRU	269	/* Time base Upper/Lower (Reading) */
#define TBRL	268
#define TBWU	285	/* Time base Upper/Lower (Writing) */
#define TBWL	284
#define PVR	287	/* Processor Version */

/*
 * mpc8xx-specific special purpose registers of interest here
 */
#define EIE		80
#define EID		81
#define NRI		82
#define IMMR		638
#define IC_CST		560
#define IC_ADR		561
#define IC_DAT		562
#define DC_CST		568
#define DC_ADR		569
#define DC_DAT		570
#define MI_CTR		784
#define MI_AP		786
#define MI_EPN		787
#define MI_TWC		789
#define MI_RPN		790
#define MI_DBCAM	816
#define MI_DBRAM0	817
#define MI_DBRAM1	818
#define MD_CTR		792
#define M_CASID		793
#define MD_AP		794
#define MD_EPN		795
#define M_TWB		796
#define MD_TWC		797
#define MD_RPN		798
#define	M_TW		799
#define	MD_DBCAM	824
#define	MD_DBRAM0	825
#define	MD_DBRAM1	826

/*
 * mpc8xx specific debug-level SPRs
 */ 
#define CMPA		144
#define CMPB		145
#define CMPC		146
#define CMPD		147
#define ICR			148
#define DER			149
#define COUNTA		150
#define COUNTB		151
#define CMPE		152
#define CMPF		153
#define CMPG		154
#define CMPH		155
#define LCTRL1		156
#define LCTRL2		157
#define ICTRL		158
#define BAR			159
#define DPDR		630

/* use of SPRG registers in save/restore */
#define	SAVER0	SPRG0
#define	SAVER1	SPRG1
#define	SAVELR	SPRG2
#define	SAVEXX	SPRG3

/* special instruction definitions */
#define	BDNZ	BC	16,0,
#define	BDNE	BC	0,2,
#define	TLBIA	WORD	$((31<<26)|(370<<1))
#define	MFTB(tbr,d)	WORD	$((31<<26)|((d)<<21)|((tbr&0x1f)<<16)|(((tbr>>5)&0x1f)<<11)|(371<<1))

/* on some models mtmsr doesn't synchronise enough (eg, 603e) */
#define	MSRSYNC	SYNC; ISYNC

/*
 * The following code assume that the MPC8xx processor has been
 * configured to a certain extent.  In particular, we assume the following
 * following registers have been set up.
 *
 * SIU
 *		SIUMCR
 *		SYPCR
 *
 */

#define	UREGSPACE	(UREGSIZE+8)

	TEXT start(SB), $-4

	/*
	 * setup MSR
	 * turn off interrupts
	 * use 0x000 as exception prefix
	 * enable machine check
	 */
	MOVW	MSR, R3
	MOVW	$(MSR_EE|MSR_IP), R4
	ANDN	R4, R3
	OR		$(MSR_ME), R3
	ISYNC
	MOVW	R3, MSR
	MSRSYNC

	/* set internal memory base */
	MOVW	$INTMEM, R4
	MOVW	R4, SPR(IMMR)

	/* except during trap handling, R0 is zero from now on */
	MOVW	$0, R0

	/* setup SB for pre mmu */
	MOVW	$setSB(SB), R2
	MOVW	$KZERO, R3
	ANDN	R3, R2

/*
 * reset the caches and disable them for now
 */
	MOVW	SPR(IC_CST), R4	/* read and clear */
	MOVW	$(5<<25), R4
	MOVW	R4, SPR(IC_CST)	/* unlock all */
	ISYNC
	MOVW	$(6<<25), R4
	MOVW	R4, SPR(IC_CST)	/* invalidate all */
	ISYNC
	MOVW	$(2<<25), R4
	MOVW	R4, SPR(IC_CST)	/* disable i-cache */
	ISYNC

	SYNC
	MOVW	SPR(DC_CST), R4	/* read and clear */
	MOVW	$(10<<24), R4
	SYNC
	MOVW	R4, SPR(DC_CST)	/* unlock all */
	ISYNC
	MOVW	$(12<<24), R4
	SYNC
	MOVW	R4, SPR(DC_CST)	/* invalidate all */
	ISYNC
	MOVW	$(4<<24), R4
	SYNC
	MOVW	R4, SPR(DC_CST)	/* disable d-cache */
	ISYNC

	MOVW	$7, R4
	MOVW	R4, SPR(ICTRL)		/* cancel `show cycle' for normal instruction execution */
	ISYNC
	MOVW	R4, SPR(DER)
	ISYNC
	BL	mmuinit0(SB)

	/* running with MMU on!! */

	/* set R2 to it correct value */
	MOVW	$setSB(SB), R2

	/* enable i-cache */
	MOVW	$(1<<25), R4
	MOVW	R4, SPR(IC_CST)
	ISYNC

 	/* enable d-cache 	*/
	MOVW	$(2<<24), R4
	MOVW	R4, SPR(DC_CST)
	ISYNC


/*
 * set other system configuration values
 */


/*
 * setup mach
 */
	MOVW	$MACHADDR, R(MACH)
	ADD	$(MACHSIZE-8), R(MACH), R1	/* set stack */
	SUB	$4, R(MACH), R3
	ADD	$4, R1, R4
clrmach:
	MOVWU	R0, 4(R3)
	CMP	R3, R4

	MOVW	R0, R(USER)
	MOVW	R0, 0(R(MACH))

	/* zero bss */
	MOVW	$edata(SB), R3
	MOVW	$end(SB), R4
	SUBCC	R3, R4
	BLE	skipz
	SRAW	$2, R4
	MOVW	R4, CTR
	SUB	$4, R3
zero:
	MOVWU	R0, 4(R3)
	BDNZ	zero
skipz:

	/* off to main */
	BL	main(SB)
	BR	0(PC)

/*
 * on return from this function we will be running in virtual mode.
 * We setup two TLB entries:
 * 1) map the first 8Mb of RAM to KZERO
 * 2) map the region that contains the IMMR
 */
TEXT	mmuinit0(SB), $0
	/* reset all the tlbs */
	TLBIA

	/* dont use CASID yet - set to zero for now */
	MOVW	$0, R4
	MOVW	R4, SPR(M_CASID)
	
	/* set Ks = 0 Kp = 1 for all access groups */
	MOVW	$0x55555555, R4
	MOVW	R4, SPR(MI_AP)
	MOVW	R4, SPR(MD_AP)

	/*
	 * set:
	 *  GPM = 0: PowerPC mode
	 *  PPM = 0: Page protection mode - no 1K pages
	 *  CIDEF = 0: cache enable when MMU disabled
	 *  WTDEF = 0: write back when MMU is disbaled
	 *  RSV2 = 0: no reserved entries
	 *	TWAM = 0: don't use table walk assit
	 *  PPCS = 0: not used in PowerPC mode
	 *	INDX = 0: start at first entry
	 */
	MOVW	$0, R4
	MOVW	R4, SPR(MI_CTR)	/* i-mmu control */
	MOVW	$0, R4
	MOVW	R4, SPR(MD_CTR)	/* d-mmu control */

	/*
	 * map 8Mb of ram at 0 -> KZERO, cached, writeback, shared
	 */
	MOVW	$(KZERO|MMUEV), R4
	MOVW	R4, SPR(MD_EPN)
	MOVW	R4, SPR(MI_EPN)
	MOVW	$(MMUPS8M|MMUV), R4		/* |MMUWT */
	MOVW	R4, SPR(MD_TWC)
	MOVW	R4, SPR(MI_TWC)
	MOVW	$(0|MMUPP|MMUSPS|MMUSH|MMUV), R4
	MOVW	R4, SPR(MD_RPN)
	MOVW	R4, SPR(MI_RPN)

	/*
	 * map 8mb IO at INTMEM->INTMEM, cache inhibit, shared
	 */
	MOVW	$(INTMEM|MMUEV), R4
	MOVW	R4, SPR(MD_EPN)
	MOVW	$(MMUPS8M|MMUWT|MMUV), R4
	MOVW	R4, SPR(MD_TWC)
	MOVW	$(INTMEM|MMUPP|MMUSPS|MMUSH|MMUCI|MMUV), R4
	MOVW	R4, SPR(MD_RPN)

	/* enable MMU */
	MOVW	LR, R3
	OR	$KZERO, R3
	MOVW	R3, SPR(SRR0)
	MOVW	MSR, R4
	OR	$(MSR_IR|MSR_DR), R4
	MOVW	R4, SPR(SRR1)
	RFI	/* resume in kernel mode in caller */

	RETURN

TEXT	splhi(SB), $0
	MOVW	MSR, R3
	RLWNM	$0, R3, $~MSR_EE, R4
	SYNC
	MOVW	R4, MSR
	MSRSYNC
	MOVW	LR, R31
	MOVW	R31, 4(R(MACH))	/* save PC in m->splpc */
	RETURN

TEXT	splx(SB), $0
	/* fall though */

TEXT	splxpc(SB), $0
	MOVW	MSR, R4
	RLWMI	$0, R3, $MSR_EE, R4
	RLWNMCC	$0, R3, $MSR_EE, R5
	BNE	splx0
	MOVW	LR, R31
	MOVW	R31, 4(R(MACH))	/* save PC in m->splpc */
splx0:
	SYNC
	MOVW	R4, MSR
	MSRSYNC
	RETURN

TEXT	spllo(SB), $0
	MFTB(TBRL, 3)
	MOVW	R3, spltbl(SB)
	MOVW	MSR, R3
	OR	$MSR_EE, R3, R4
	SYNC
	MOVW	R4, MSR
	MSRSYNC
	RETURN

TEXT	spldone(SB), $0
	RETURN

TEXT	islo(SB), $0
	MOVW	MSR, R3
	RLWNM	$0, R3, $MSR_EE, R3
	RETURN

TEXT	setlabel(SB), $-4
	MOVW	LR, R31
	MOVW	R1, 0(R3)
	MOVW	R31, 4(R3)
	MOVW	$0, R3
	RETURN

TEXT	gotolabel(SB), $-4
	MOVW	4(R3), R31
	MOVW	R31, LR
	MOVW	0(R3), R1
	MOVW	$1, R3
	RETURN

TEXT	touser(SB), $-4
	MOVW	$(UTZERO+32), R5	/* header appears in text */
	MOVW	$(MSR_EE|MSR_PR|MSR_ME|MSR_IR|MSR_DR|MSR_RI), R4
	MOVW	R4, SPR(SRR1)
	MOVW	R3, R1
	MOVW	R5, SPR(SRR0)
	RFI


TEXT	icflush(SB), $-4	/* icflush(virtaddr, count) */
	MOVW	n+4(FP), R4
	RLWNM	$0, R3, $~(CACHELINESZ-1), R5
	SUB	R5, R3
	ADD	R3, R4
	ADD		$(CACHELINESZ-1), R4
	SRAW	$CACHELINELOG, R4
	MOVW	R4, CTR
icf0:	ICBI	(R5)
	ADD	$CACHELINESZ, R5
	BDNZ	icf0
	ISYNC
	RETURN

TEXT	dcflush(SB), $-4	/* dcflush(virtaddr, count) */
	MOVW	n+4(FP), R4
	RLWNM	$0, R3, $~(CACHELINESZ-1), R5
	CMP	R4, $0
	BLE	dcf1
	SUB	R5, R3
	ADD	R3, R4
	ADD		$(CACHELINESZ-1), R4
	SRAW	$CACHELINELOG, R4
	MOVW	R4, CTR
dcf0:	DCBF	(R5)
	ADD	$CACHELINESZ, R5
	BDNZ	dcf0
dcf1:
	RETURN

TEXT	tas(SB), $0
	SYNC
	MOVW	R3, R4
	MOVW	$0xdead,R5
tas1:
	DCBF	(R4)	/* fix for 603x bug */
	LWAR	(R4), R3
	CMP	R3, $0
	BNE	tas0
	STWCCC	R5, (R4)
	BNE	tas1
tas0:
	SYNC
	ISYNC
	RETURN

TEXT	gettbl(SB), $0
	MFTB(TBRL, 3)
	RETURN

TEXT	gettbu(SB), $0
	MOVW	SPR(TBRU), R3
	RETURN

TEXT	getpvr(SB), $0
	MOVW	SPR(PVR), R3
	RETURN

TEXT	getimmr(SB), $0
	MOVW	SPR(IMMR), R3
	RETURN

TEXT	getdec(SB), $0
	MOVW	SPR(DEC), R3
	RETURN

TEXT	putdec(SB), $0
	MOVW	R3, SPR(DEC)
	RETURN

TEXT	getdar(SB), $0
	MOVW	SPR(DAR), R3
	RETURN

TEXT	getdsisr(SB), $0
	MOVW	SPR(DSISR), R3
	RETURN

TEXT	getdepn(SB), $0
	MOVW	SPR(MD_EPN), R3
	RETURN

TEXT	getmsr(SB), $0
	MOVW	MSR, R3
	RETURN

TEXT	putder(SB), $0
	MOVW	R3, SPR(DER)
	RETURN

TEXT	getder(SB), $0
	MOVW	SPR(DER), R3
	RETURN

TEXT	putmsr(SB), $0
	SYNC
	MOVW	R3, MSR
	MSRSYNC
	RETURN

TEXT	eieio(SB), $0
	EIEIO
	RETURN

TEXT	tlbflushall(SB), $0
	TLBIA
	RETURN

TEXT	tlbflush(SB), $0
	TLBIE	R3
	RETURN

TEXT	putcasid(SB), $-4
	MOVW	LR, R4
	MOVW	MSR, R5
	BL	nommu(SB)
	MOVW	R3,	SPR(M_CASID)
	MOVW	R4, SPR(SRR0)
	MOVW	R5, SPR(SRR1)
	RFI

TEXT	nommu(SB), $-4
	MOVW	LR, R6
	RLWNM	$0, R6, $~KZERO, R6
	MOVW	$(MSR_DR|MSR_IR), R8
	MOVW	R5, R7
	ANDN	R8, R7
	MOVW	R6, SPR(SRR0)
	MOVW	R7, SPR(SRR1)
	RFI

TEXT	gotopc(SB), $0
	MOVW	R3, CTR
	MOVW	LR, R31	/* for trace back */
	BR	(CTR)

/*
 * byte swapping of arrays of long and short;
 * could possibly be avoided with more changes to drivers
 */
TEXT	swabl(SB), $0
	MOVW	v+4(FP), R4
	MOVW	n+8(FP), R5
	SRAW	$2, R5, R5
	MOVW	R5, CTR
	SUB	$4, R4
	SUB	$4, R3
swabl1:
	ADD	$4, R3
	MOVWU	4(R4), R7
	MOVWBR	R7, (R3)
	BDNZ	swabl1
	RETURN

TEXT	swabs(SB), $0
	MOVW	v+4(FP), R4
	MOVW	n+8(FP), R5
	SRAW	$1, R5, R5
	MOVW	R5, CTR
	SUB	$2, R4
	SUB	$2, R3
swabs1:
	ADD	$2, R3
	MOVHZU	2(R4), R7
	MOVHBR	R7, (R3)
	BDNZ	swabs1
	RETURN

TEXT	legetl(SB), $0
	MOVWBR	(R3), R3
	RETURN

TEXT	lesetl(SB), $0
	MOVW	v+4(FP), R4
	MOVWBR	R4, (R3)
	RETURN

TEXT	legets(SB), $0
	MOVHBR	(R3), R3
	RETURN

TEXT	lesets(SB), $0
	MOVW	v+4(FP), R4
	MOVHBR	R4, (R3)
	RETURN


TEXT	itlbmiss(SB), $-4
	MOVW	R1, SPR(SAVER1)
	MOVW	R2, SPR(M_TW)

/*
	to enable hardware break points
	MOVW	MSR, R1
	OR		$(MSR_RI), R1
	MOVW	R1, MSR
	ISYNC	
*/

	/* m->tlbfault++ */
	MOVW	$(MACHADDR&~KZERO), R1		/* m-> */
	MOVW	20(R1), R2	
	ADD		$1, R2
	MOVW	R2, 20(R1)

	MOVW	CR, R2
	MOVW	SPR(MI_EPN), R1
	MOVW	R1, CR
	BC	4,0,itlblookup
	/*
	 * map ram
	 */
	MOVW	R2, CR
	MOVW	$(MMUPS8M|MMUV), R2
	MOVW	R2, SPR(MI_TWC)
	RLWNM	$0, R1, $0x7f800000, R1
	OR	$(MMUPP|MMUSPS|MMUSH|MMUV), R1
	MOVW	R1, SPR(MI_RPN)
	MOVW	SPR(M_TW), R2
	MOVW	SPR(SAVER1), R1
	RFI

itlblookup:
	MOVW	R0, SPR(SAVER0)
	MOVW	R3, SPR(SAVEXX)

	/*
	 * R1 = 20 bits of addr | 8 bits of junk | 4 bits of asid
	 * calulate ((x>>9)^(x>>21)^(x<<11)) & (0xfff<<3)
	 * note (x>>21)^(x<<11) = rotate left 11
	 */
	RLWNM	$(32-9), R1, $(0xfff<<3), R3
	RLWNM	$11, R1, $(0xfff<<3), R1
	XOR		R1, R3, R1
	MOVW	$(MACHADDR&~KZERO), R3	/* m-> */
	MOVW	16(R3), R3				/* m->stb */
	RLWNM	$0, R3, $~KZERO, R3		/* PADDR(m->stb) */
	ADD		R1, R3
	MOVW	4(R3), R0
	MOVW	0(R3), R3
	MOVW	SPR(MI_EPN), R1
	RLWNM	$20, R1, $0xffffff, R1
	RLWNM	$12, R1, $~0, R1
	CMP		R3, R1
	BNE		itlbtrap
	MOVW	R2, CR
	MOVW	$(MMUV), R1
	MOVW	R1, SPR(MI_TWC)
	MOVW	R0, SPR(MI_RPN)
	MOVW	SPR(SAVEXX), R3
	MOVW	SPR(M_TW), R2
	MOVW	SPR(SAVER1), R1
	MOVW	SPR(SAVER0), R0
	RFI
itlbtrap:
	MOVW	R2, CR

	MOVW	$(MACHADDR&~KZERO), R2	/* m-> */
	MOVW	R1, 32(R2)
	MOVW	R3, 36(R2)

	MOVW	SPR(SAVEXX), R3
	MOVW	SPR(M_TW), R2
	MOVW	LR, R0
	MOVW	R0, SPR(SAVELR)
	MOVW	$0x1100, R0
	BR	traps


TEXT	dtlbmiss(SB), $-4

	MOVW	R1, SPR(SAVER1)
	MOVW	R2, SPR(M_TW)
/*
	to enable hardware break points
	MOVW	MSR, R1
	OR		$(MSR_RI), R1
	MOVW	R1, MSR
	ISYNC
*/	

	/* m->tlbfault++ */
	MOVW	$(MACHADDR&~KZERO), R1		/* m-> */
	MOVW	20(R1), R2	
	ADD		$1, R2
	MOVW	R2, 20(R1)

	MOVW	CR, R2
	MOVW	SPR(MD_EPN), R1
	MOVW	R1, CR
	BC	4,0,dtlblookup
	BC	12,1,dtlbio
	/*
	 * map ram
	 */
	MOVW	R2, CR
	MOVW	$(MMUPS8M|MMUV), R2
	MOVW	R2, SPR(MD_TWC)
	RLWNM	$0, R1, $0x7f800000, R1
	OR	$(MMUPP|MMUSPS|MMUSH|MMUV), R1
	MOVW	R1, SPR(MD_RPN)
	MOVW	SPR(M_TW), R2
	MOVW	SPR(SAVER1), R1
	RFI
dtlbio:
	/*
	 * map io
	 */
	MOVW	R2, CR
	MOVW	$(MMUPS8M|MMUWT|MMUV), R2
	MOVW	R2, SPR(MD_TWC)
	RLWNM	$0, R1, $0xff800000, R1
	OR	$(MMUPP|MMUSPS|MMUSH|MMUCI|MMUV), R1
	MOVW	R1, SPR(MD_RPN)
	MOVW	SPR(M_TW), R2
	MOVW	SPR(SAVER1), R1
	RFI
dtlblookup:
	MOVW	R0, SPR(SAVER0)
	MOVW	R3, SPR(SAVEXX)

	/*
	 * R1 = 20 bits of addr | 8 bits of junk | 4 bits of asid
	 * calulate ((x>>9)^(x>>21)^(x<<11)) & (0xfff<<3)
	 * note (x>>21)^(x<<11) = rotate left 11
	 */
	RLWNM	$(32-9), R1, $(0xfff<<3), R3
	RLWNM	$11, R1, $(0xfff<<3), R1
	XOR		R1, R3, R1
	MOVW	$(MACHADDR&~KZERO), R3	/* m-> */
	MOVW	16(R3), R3				/* m->stb */
	RLWNM	$0, R3, $~KZERO, R3		/* PADDR(m->stb) */
	ADD	R1,R3,R3
	MOVW	4(R3), R0
	MOVW	0(R3), R3
	MOVW	SPR(MD_EPN), R1
	RLWNM	$20, R1, $0xffffff, R1
	RLWNM	$12, R1, $~0, R1
	CMP		R3, R1
	BNE		dtlbtrap
	MOVW	$(MMUV), R1
	MOVW	R1, SPR(MD_TWC)
	MOVW	R0, SPR(MD_RPN)
	MOVW	R2, CR
	MOVW	SPR(SAVEXX), R3
	MOVW	SPR(M_TW), R2
	MOVW	SPR(SAVER1), R1
	MOVW	SPR(SAVER0), R0
	RFI
dtlbtrap:
	MOVW	R2, CR

	MOVW	$(MACHADDR&~KZERO), R2	/* m-> */
	MOVW	R1, 32(R2)
	MOVW	R3, 36(R2)

	MOVW	SPR(MD_EPN), R1
	MOVW	$(MACHADDR&~KZERO), R3	/* m-> */
	MOVW	R1, 24(R3)				/* save dar */
	MOVW	SPR(SAVEXX), R3
	MOVW	SPR(M_TW), R2
	MOVW	LR, R0
	MOVW	R0, SPR(SAVELR)
	MOVW	$0x1200, R0
	BR	traps

TEXT	dtlberror(SB), $-4
	MOVW	R0, SPR(SAVER0)
	MOVW	R1, SPR(SAVER1)
	MOVW	LR, R0
	MOVW	R0, SPR(SAVELR)
	MOVW	$(MACHADDR&~KZERO), R1		/* m-> */
	MOVW	SPR(DAR), R0
	MOVW	R0, 24(R1)				/* save dar */
	MOVW	SPR(DSISR), R0
	MOVW	R0, 28(R1)				/* save dsisr */
	MOVW	$0x1400, R0
	BR	traps

/*
 * traps force memory mapping off.
 * the following code has been executed at the exception
 * vector location
 *	MOVW R0, SPR(SAVER0)
 *	MOVW LR, R0
 *	MOVW R0, SPR(SAVELR) 
 *	bl	trapvec(SB)
 */
TEXT	trapvec(SB), $-4
	MOVW	LR, R0
	MOVW	R1, SPR(SAVER1)
traps:
	MOVW	R0, SPR(SAVEXX)	/* vector */

/*
	to enable hardware break points
	MOVW	MSR, R1
	OR		$(MSR_RI), R1
	MOVW	R1, MSR
	ISYNC
*/	

	/* did we come from user space */
	MOVW	SPR(SRR1), R0
	MOVW	CR, R1
	MOVW	R0, CR
	BC	4,17,ktrap
	
	/* switch to kernel stack */
	MOVW	R1, CR
	MOVW	$(MACHADDR&~KZERO), R1	/* PADDR(m->) */
	MOVW	12(R1), R1				/* m->proc  */
	RLWNM	$0, R1, $~KZERO, R1		/* PADDR(m->proc) */
	MOVW	8(R1), R1				/* m->proc->kstack */
	RLWNM	$0, R1, $~KZERO, R1		/* PADDR(m->proc->kstack) */
	ADD	$(KSTACK-UREGSIZE), R1
	BL	saveureg(SB)
	BL	trap(SB)
	BR	restoreureg
ktrap:
	MOVW	R1, CR
	MOVW	SPR(SAVER1), R1
	RLWNM	$0, R1, $~KZERO, R1		/* PADDR(m->proc->kstack) */
	SUB	$UREGSPACE, R1
	BL	saveureg(SB)
	BL	trap(SB)
	BR	restoreureg

TEXT	intrvec(SB), $-4
	MOVW	LR, R0

	BR	0(PC)

/*
 * map data virtually and make space to save
 */
	MOVW	R0, SPR(SAVEXX)	/* vector */
	MOVW	R1, SPR(SAVER1)
	SYNC
	ISYNC
	MOVW	MSR, R0
	OR	$MSR_DR, R0		/* make data space usable */
	SYNC
	MOVW	R0, MSR
	MSRSYNC
	SUB	$UREGSPACE, R1

	MFTB(TBRL, 0)
	MOVW	R0, intrtbl(SB)

	MOVW	SPR(SRR0), R0
	MOVW	R0, LR
	MOVW	SPR(SRR1), R0
	MOVW	R0, 12(R1)
	MOVW	LR, R0
	MOVW	R0, 16(R1)
	BL	saveureg(SB)

	MFTB(TBRL, 5)
	MOVW	R5, isavetbl(SB)

	BL	intr(SB)
/*
 * enter with stack set and mapped.
 * on return, SB (R2) has been set, and R3 has the Ureg*,
 * the MMU has been re-enabled, kernel text and PC are in KSEG,
 * R(MACH) has been set, and R0 contains 0.
 *
 */
TEXT	saveureg(SB), $-4
/*
 * save state
 */
	MOVMW	R2, 48(R1)	/* r2:r31 */
	MOVW	$setSB(SB), R2
	MOVW	$(MACHADDR&~KZERO), R(MACH)
	MOVW	12(R(MACH)), R(USER)
	MOVW	$MACHADDR, R(MACH)
	MOVW	SPR(SAVER1), R4
	MOVW	R4, 44(R1)
	MOVW	SPR(SAVER0), R5
	MOVW	R5, 40(R1)
	MOVW	CTR, R6
	MOVW	R6, 36(R1)
	MOVW	XER, R4
	MOVW	R4, 32(R1)
	MOVW	CR, R5
	MOVW	R5, 28(R1)
	MOVW	SPR(SAVELR), R6	/* LR */
	MOVW	R6, 24(R1)
	/* pad at 20(R1) */
	MOVW	SPR(SRR0), R0
	MOVW	R0, 16(R1)				/* old PC */
	MOVW	SPR(SRR1), R0
	MOVW	R0, 12(R1)				/* old status */
	MOVW	SPR(SAVEXX), R0
	MOVW	R0, 8(R1)	/* cause/vector */
	ADD	$8, R1, R3	/* Ureg* */
	OR	$KZERO, R3	/* fix ureg */
	STWCCC	R3, (R1)	/* break any pending reservations */
	MOVW	$0, R0	/* compiler/linker expect R0 to be zero */

	MOVW	MSR, R5
	OR	$(MSR_IR|MSR_DR|MSR_RI), R5	/* enable MMU */
	MOVW	R5, SPR(SRR1)
	MOVW	LR, R31
	OR	$KZERO, R31	/* return PC in KSEG0 */
	MOVW	R31, SPR(SRR0)
	OR	$KZERO, R1	/* fix stack pointer */
	RFI	/* returns to trap handler */

/*
 * restore state from Ureg and return from trap/interrupt
 */
TEXT	forkret(SB), $0
	BR	restoreureg

restoreureg:
	MOVMW	48(R1), R2	/* r2:r31 */
	/* defer R1 */
	MOVW	40(R1), R0
	MOVW	R0, SPR(SAVER0)
	MOVW	36(R1), R0
	MOVW	R0, CTR
	MOVW	32(R1), R0
	MOVW	R0, XER
	MOVW	28(R1), R0
	MOVW	R0, CR	/* CR */
	MOVW	24(R1), R0
	MOVW	R0, LR
	/* pad, skip */
	MOVW	16(R1), R0
	MOVW	R0, SPR(SRR0)	/* old PC */
	MOVW	12(R1), R0
	MOVW	R0, SPR(SRR1)	/* old MSR */
	/* cause, skip */
	MOVW	44(R1), R1	/* old SP */
	MOVW	SPR(SAVER0), R0
	RFI

TEXT	flash(SB), $-4
flash0:
	BL	powerupled(SB);
	MOVW	$0x100000, R5
	MOVW	R5, CTR
delay0:
	BDNZ	delay0
	BL	powerdownled(SB);
	MOVW	$0x100000, R5
	MOVW	R5, CTR
delay1:
	BDNZ	delay1
	BR		flash0

TEXT	powerupled(SB), $0

	MOVW	$INTMEM,R11
	MOVH	0x970(R11), R7
	MOVW	$0x100,R8
	OR	R8,R7
	MOVH	R7,0x970(R11)
	MOVH	0x976(R11), R7
	ANDN	R8,R7
	MOVH	R7,0x976(R11)
	RETURN

TEXT	powerdownled(SB), $0

	MOVW	$INTMEM,R11
	MOVH	0x970(R11), R7
	MOVW	$0x100,R8
	OR	R8,R7
	MOVH	R7,0x970(R11)
	MOVH	0x976(R11), R7
	OR	R8,R7
	MOVH	R7,0x976(R11)
	RETURN
	
GLOBL	spltbl+0(SB), $4
GLOBL	intrtbl+0(SB), $4
GLOBL	isavetbl+0(SB), $4

	RETURN

/*
 * TLB prototype entries, loaded once-for-all at startup,
 * remaining unchanged thereafter.
 * Limit the table to at most 4 entries
 */
#define	TLBE(epn,twc,rpn)	WORD	$(epn);	WORD	$(twc);	WORD	$(rpn)

TEXT	tlbtab(SB), $-4
	/* epn, rpn, twc */
	TLBE(FLASHMEM|MMUEV, MMUPS8M|MMUWT|MMUV, FLASHMEM|MMUPP|MMUSPS|MMUSH|MMUCI|MMUV)	/* FLASH, 8M */
	TLBE(DRAMMEM|MMUEV, MMUPS8M|MMUWT|MMUV, DRAMMEM|MMUPP|MMUSPS|MMUSH|MMUV)	/* DRAM, second 8M */
	TLBE(INTMEM|MMUEV, MMUPS8M|MMUWT|MMUV, INTMEM|MMUPP|MMUSPS|MMUSH|MMUCI|MMUV)	/* IO space 8M */
TEXT	tlbtabe(SB), $-4
	RETURN
