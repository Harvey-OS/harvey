/*
 * VMS PALcode instructions, in numerical order.
 */

#define	PALhalt		0x00	/* required per Alpha architecture */
#define	PALcflush	0x01
#define	PALdraina	0x02	/* required per Alpha architecture */
#define	PALldqp		0x03

#define	PALstqp		0x04
#define	PALswpctx	0x05
#define	PALmfpr_asn	0x06
#define	PALmtpr_asten	0x07
#define	PALmtpr_astsr	0x08
#define	PALcserve	0x09
#define	PALswppal	0x0a
#define	PALmfpr_fen	0x0b
#define	PALmtpr_fen	0x0c
#define	PALmtpr_ipir	0x0d
#define	PALmfpr_ipl	0x0e
#define	PALmtpr_ipl	0x0f
#define	PALmfpr_mces	0x10
#define	PALmtpr_mces	0x11
#define	PALmfpr_pcbb	0x12
#define	PALmfpr_prbr	0x13
#define	PALmtpr_prbr	0x14
#define	PALmfpr_ptbr	0x15
#define	PALmfpr_scbb	0x16
#define	PALmtpr_scbb	0x17
#define	PALmtpr_sirr	0x18
#define	PALmfpr_sisr	0x19
#define	PALmfpr_tbchk	0x1a
#define	PALmtpr_tbia	0x1b
#define	PALmtpr_tbiap	0x1c
#define	PALmtpr_tbis	0x1d
#define	PALmfpr_esp	0x1e
#define	PALmtpr_esp	0x1f
#define	PALmfpr_ssp	0x20
#define	PALmtpr_ssp	0x21
#define	PALmfpr_usp	0x22
#define	PALmtpr_usp	0x23
#define	PALmtpr_tbisd	0x24
#define	PALmtpr_tbisi	0x25
#define	PALmfpr_asten	0x26
#define	PALmfpr_astsr	0x27
				/* where is instruction 0x28 ? */
#define	PALmfpr_vptb	0x29
#define	PALmtpr_vptb	0x2a
#define	PALmtpr_perfmon	0x2b
				/* where is instruction 0x2c ? */
				/* where is instruction 0x2d ? */
#define	PALmtpr_datfx	0x2e
/*
 * ... 0x2f to 0x3e ??
 */
#define	PALmfpr_whami	0x3f
/*
 * ... 0x40 to 0x7f ??
 */
#define	PALbpt		0x80
#define	PALbugchk	0x81
#define	PALchime	0x82
#define	PALchmk		0x83
#define	PALchms		0x84
#define	PALchmu		0x85
#define	PALimb		0x86	/* required per Alpha architecture */
#define	PALinsqhil	0x87
#define	PALinsqtil	0x88
#define	PALinsqhiq	0x89
#define	PALinsqtiq	0x8a
#define	PALinsquel	0x8b
#define	PALinsqueq	0x8c
#define	PALinsqueld	0x8d	/* INSQUEL/D */
#define	PALinsqueqd	0x8e	/* INSQUEQ/D */
#define	PALprober	0x8f
#define	PALprobew	0x90
#define	PALrd_ps	0x91
#define	PALrei		0x92
#define	PALremqhil	0x93
#define	PALremqtil	0x94
#define	PALremqhiq	0x95
#define	PALremqtiq	0x96
#define	PALremquel	0x97
#define	PALremqueq	0x98
#define	PALremqueld	0x99	/* REMQUEL/D */
#define	PALremqueqd	0x9a	/* REMQUEQ/D */
#define	PALswasten	0x9b
#define	PALwr_ps_sw	0x9c
#define	PALrscc		0x9d
#define	PALread_unq	0x9e
#define	PALwrite_unq	0x9f
#define	PALamovrr	0xa0
#define	PALamovrm	0xa1
#define	PALinsqhilr	0xa2
#define	PALinsqtilr	0xa3

#define	PALinsqhiqr	0xa4
#define	PALinsqtiqr	0xa5
#define	PALremqhilr	0xa6

#define	PALremqtilr	0xa7
#define	PALremqhiqr	0xa8
#define	PALremqtiqr	0xa9
#define	PALgentrap	0xaa

