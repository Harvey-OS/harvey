/*
 * definitions for machine-language startup
 */
#undef DEBUG

#define SPLHI	CSRRC	CSR(SSTATUS), $Sie, R0

/* dedicated registers during start-up */
#define SYSRB	20
#define MACHMODE 21
#define RKSEG0	22
#define TMP2	23
#define LOCK	24
#define UART0	25
#define PRTMP	26
#define CSRZERO	27
#define TMP	29
#define MACHNO	30
#define HARTID	31

#define INITSTKSIZE	PGSZ

/* spinlock from unpriv. isa spec., figure a.7 */
#define ACQLOCK	\
	MOV	$1, R(TMP2); \
	FENCE; \
	/* after: old printlck in R(TMP), updated printlck in memory */ \
	AMOW(Amoswap, AQ|RL, TMP2, LOCK, TMP); \
	BNE	R(TMP), -3(PC);		/* was non-0? lock held by another */ \
	FENCE
#define RELLOCK \
	FENCE; \
	MOVW	R0, (R(LOCK)); \
	FENCE

#ifdef DEBUG
/* assumes R(UART0) is set. */
#ifdef SIFIVEUART
#define CONSPUT(c) \
	FENCE; \
	MOVWU	(0*4)(R(UART0)), R(TMP); /* 0 is txdata */ \
	MOVW	$(1ul<<31), R(TMP2); \
	AND	R(TMP2), R(TMP);	/* notready bit */ \
	BNE	R(TMP), -4(PC); \
	MOV	c, R(TMP); \
	MOVW	R(TMP), (R(UART0))
#else					/* SIFIVEUART */
/* i8250 */
#define CONSPUT(c) \
	FENCE; \
	MOVWU	(5*4)(R(UART0)), R(TMP); /* Lsr is 5 */ \
	AND	$0x20, R(TMP);		/* Thre (Thr empty) is 0x20 */ \
	BEQ	R(TMP), -3(PC); \
	MOV	c, R(TMP); \
	MOVW	R(TMP), (R(UART0))	/* reg 0 is Thr */
#endif					/* SIFIVEUART */

#define CONSPUTLCK(c) \
	ACQLOCK; \
	CONSPUT(c); \
	RELLOCK
#define PRIDMACHNO(id) \
	ACQLOCK; \
	CONSPUT(id); \
	ADD	$'0', R(MACHNO), R(PRTMP); \
	CONSPUT(R(PRTMP)); \
	RELLOCK
#else					/* DEBUG */
#define CONSPUT(c)
#define CONSPUTLCK(c)
#define PRIDMACHNO(id)
#endif					/* DEBUG */

#define LOADSATP(reg); \
	FENCE; \
	FENCE_I; \
	SFENCE_VMA(0, 0)			/* flush TLB */; \
	MOV	reg, CSR(SATP); \
	SFENCE_VMA(0, 0)			/* flush TLB */; \
	FENCE; \
	FENCE_I
