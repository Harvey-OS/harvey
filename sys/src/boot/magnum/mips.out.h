struct mipshdr {
	ushort	f_magic;	/* magic number */
	ushort	f_nscns;	/* number of sections */
	long	f_timdat;	/* time & date stamp */
	long	f_symptr;	/* file pointer to symbolic header */
	long	f_nsyms;	/* sizeof(symbolic hdr) */
	ushort	f_opthdr;	/* sizeof(optional hdr) */
	ushort	f_flags;	/* flags */

	short	magic;		/* another one				*/
	short	vstamp;		/* version stamp			*/
	long	tsize;		/* text size in bytes, padded to DW bdry*/
	long	dsize;		/* initialized data "  "		*/
	long	bsize;		/* uninitialized data "   "		*/
	long	entry;		/* entry pt.				*/
	long	text_start;	/* base of text used for this file	*/
	long	data_start;	/* base of data used for this file	*/
	long	bss_start;	/* base of bss used for this file	*/
	long	gprmask;	/* general purpose register mask	*/
	long	cprmask[4];	/* co-processor register masks		*/
	long	gp_value;	/* the gp value used for this object	*/

	long	dummy;		/* pad to 0x50 bytes */
};

#define  F_RELFLG	001	/* relocation info stripped from file */
#define  F_EXEC		002	/* file is executable */
#define  F_LNNO		004	/* line nunbers stripped from file */
#define  F_LSYMS	010	/* local symbols stripped from file */
