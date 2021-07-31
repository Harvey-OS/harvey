/*****************************************************************************
*									     *
*  p_def.h							2/1/89	     *
*									     *
*  Author:		John A. Feaver					     *
*									     *
*  Description:		This file, with f_def.h and fax_glb.h, has all the   *
*			global definitions used by the Fax Platform Library. *
*									     *
*  Modifications: 7/89 bns	- add p_stat_t structure definition	     *
*				- add p_mhshdrs_t structure definition	     *
*		7/27/89 jaf	- add fax_dmainfo_t struc to p_sent_parms    *
*									     *
*****************************************************************************/
/* Width of fax line */

#define M_FAXWIDTH	1728		/* 1728 pels (216 bytes) */
#define PIXLINESZ	216		/* 216 byte line size */
#define COMPBUFF	2000		/* Compressed-data buffer */
#define P_PATHLEN	100		/* Length of Font File Path */
#define P_FONTPATH	"/usr/lib"	/* Path for Font Files */

/* X.409/X.209 ("MHS format") Constants */

#define P_DS_HDR0	0x30		/* Data Sequence Header */
#define P_DS_HDR1	0x80
#define P_DS_TLR0	0x00		/* Data Sequence Trailer */
#define P_DS_TLR1	0x00
#define P_PAGE_HDR0	0x23		/* Page Header (Constructive) */
#define P_PAGE_HDR1	0x80		/* Indefinite length code */
#define	P_PAGE_HDR2	0x03		/* primitive form */
#define P_PAGE_TLR0	0x00		/* Page Trailer */
#define P_PAGE_TLR1	0x00
#define P_PBS_HDR	0x03		/* Primitive Bit String Header */
#define P_UNUSED	0x00		/* Unused-Bits Field */


/* Enumerated Types */
enum file_mode_enu {WRITE_MODE, READ_MODE};
enum file_type_enu {MHS_TYPE, ATTFAX_TYPE};
enum mhs_form_enu  {CONSTRUCTIVE, PRIMITIVE};
enum mhs_len_enu   {DEFINITE, INDEFINITE};
