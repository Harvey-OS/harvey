aggr Exec
{
	int	magic;		/* magic number */
	int	text;	 	/* size of text segment */
	int	data;	 	/* size of initialized data */
	int	bss;	  	/* size of uninitialized data */
	int	syms;	 	/* size of symbol table */
	int	entry;	 	/* entry point */
	int	spsz;		/* size of pc/sp offset table */
	int	pcsz;		/* size of pc/line number table */
};

#define	_MAGIC(b)	((((4*b)+0)*b)+7)
#define	A_MAGIC		_MAGIC(8)	/* vax */
#define	Z_MAGIC		_MAGIC(10)	/* hobbit */
#define	I_MAGIC		_MAGIC(11)	/* intel 386 */
#define	J_MAGIC		_MAGIC(12)	/* intel 960 */
#define	K_MAGIC		_MAGIC(13)	/* sparc */
#define	P_MAGIC		_MAGIC(14)	/* hp-pa */
#define	V_MAGIC		_MAGIC(16)	/* mips 3000 */
#define X_MAGIC		_MAGIC(17)	/* att dsp 3210 */
