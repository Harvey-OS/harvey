typedef	struct	Exec	Exec;
struct	Exec
{
	long	magic;		/* magic number */
	long	text;	 	/* size of text segment */
	long	data;	 	/* size of initialized data */
	long	bss;	  	/* size of uninitialized data */
	long	syms;	 	/* size of symbol table */
	long	entry;	 	/* entry point */
	long	spsz;		/* size of pc/sp offset table */
	long	pcsz;		/* size of pc/line number table */
};

#define	_MAGIC(b)	((((4*b)+0)*b)+7)
#define	A_MAGIC		_MAGIC(8)	/* 68020 */
#define	I_MAGIC		_MAGIC(11)	/* intel 386 */
#define	J_MAGIC		_MAGIC(12)	/* intel 960 */
#define	K_MAGIC		_MAGIC(13)	/* sparc */
#define	V_MAGIC		_MAGIC(16)	/* mips 3000 */
#define	X_MAGIC		_MAGIC(17)	/* att dsp 3210 */
#define	M_MAGIC		_MAGIC(18)	/* mips 4000 */
#define	D_MAGIC		_MAGIC(19)	/* amd 29000 */
#define	E_MAGIC		_MAGIC(20)	/* arm 7-something */
#define	Q_MAGIC		_MAGIC(21)	/* powerpc */
#define	N_MAGIC		_MAGIC(22)	/* mips 4000 LE */
#define	L_MAGIC		_MAGIC(23)	/* dec alpha */

#define	DYN_MAGIC	0x80000000	/* or'd in for dynamically loaded modules */

typedef	struct	Sym	Sym;
struct	Sym
{
	long	value;
	char	type;
	char	*name;
};
