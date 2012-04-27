uvlong	allocate(int);
int		bootp(char*);
void		consinit(void);
int		devopen(char*);
int		devclose(int);
int		devread(int, uchar*, int, int);
int		devwrite(int, uchar*, int, int);
uvlong	dispatch(uvlong, uvlong, uvlong, uvlong, uvlong);
void		dumpenv(void);
void		firmware(void);
uvlong	gendispatch(uvlong, uvlong, uvlong, uvlong, uvlong, uvlong);
int		getcfields(char*, char**, int, char*);
char*	getconf(char*);
char*	getenv(char*);
uvlong	getptbr(void);
void		kexec(ulong);
uvlong	ldqp(uvlong);
void		meminit(void);
void		mmuinit(void);
ulong	msec(void);
uvlong	rdv(uvlong);
uvlong	paddr(uvlong);
void		panic(char *, ...);
ulong	pcc_cnt(void);
uvlong	pground(uvlong);
void		putstrn(char *, int);
void		setconf(char*);
void		stqp(uvlong, uvlong);
int		swppal(uvlong, uvlong, uvlong, uvlong, uvlong);
void		tlbflush(void);
int		validrgn(ulong, ulong);
void		wrv(uvlong, uvlong);

#define	GSHORT(p)	(((p)[1]<<8)|(p)[0])
#define	GLONG(p)	((GSHORT(p+2)<<16)|GSHORT(p))
#define	GLSHORT(p)	(((p)[0]<<8)|(p)[1])
#define	GLLONG(p)	((GLSHORT(p)<<16)|GLSHORT(p+2))
