/*
 * Memory and machine-specific definitions.  Used in C and assembler.
 */

#define	BI2BY		8			/* bits per byte */
#define	BI2WD		32			/* bits per word */
#define	BY2WD		4			/* bytes per word */
#define 	BY2V		8			/* bytes per vlong */

#define	KZERO		0x80000000

#define	PTEVALID		0xff01
#define	PTEKVALID	0x1101
#define	PTEASM		0x0010
#define	PTEGH(s)		((s)<<5)

/*
 * VMS Palcode instructions (incomplete and possibly incorrect)
 */
#define	PALimb		0x86
#define	PALhalt		0x00
#define	PALdraina	0x02
#define	PALcserve	0x09

#define	PALmfpr_pcbb	0x12
#define	PALmfpr_ptbr	0x15
#define	PALmfpr_vptb	0x29
#define	PALldqp		0x03
#define	PALstqp		0x04
#define	PALswppal	0x0a

#define	PALmtpr_tbia	0x1b
#define	PALmtpr_mces	0x17
#define	PALmfpr_mces	0x16
#define	PALmtpr_ipl	0x15
#define	PALmfpr_ipl	0x14
