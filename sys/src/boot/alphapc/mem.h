/*
 * Memory and machine-specific definitions.  Used in C and assembler.
 */

#define	BI2BY		8			/* bits per byte */
#define	BI2WD		32			/* bits per word */
#define	BY2WD		4			/* bytes per word */
#define BY2V		8			/* bytes per vlong */

#define	KZERO		0x80000000

#define	PTEVALID	0xff01
#define	PTEKVALID	0x1101
#define	PTEASM		0x0010
#define	PTEGH(s)	((s)<<5)
