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
#define	A_MAGIC		_MAGIC(8)	/* vax */
#define	Z_MAGIC		_MAGIC(10)	/* hobbit */
#define	I_MAGIC		_MAGIC(11)	/* intel 386 */
#define	J_MAGIC		_MAGIC(12)	/* intel 960 */
#define	K_MAGIC		_MAGIC(13)	/* sparc */
#define	P_MAGIC		_MAGIC(14)	/* hp-pa */
#define	V_MAGIC		_MAGIC(16)	/* mips 3000 */
#define	NNAME	20

typedef	struct	Sym	Sym;
struct	Sym
{
	long	value;
	char	type;
	char	name[NNAME];	/* NUL-terminated */
	char	pad[3];
};
