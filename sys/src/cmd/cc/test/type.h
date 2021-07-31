
typedef	struct	Type	Type;

struct	Type
{
	schar		sc;
	uchar		uc;
	short		sh;
	ushort		uh;
	long		sl;
	ulong		ul;
#ifndef	NOFLOAT
	float		ff;
	double		df;
#else
	int		ff;
	int		df;
#endif
#ifndef	NOBIT
	long		sb:15;
	ulong		ub:15;
#else
	int		sb;
	int		ub;
#endif
};

void	settype(Type*, int);
void	tsttype(Type*, Type*, char*, char*);
void	tsttype1(Type*, Type*, char*, char*);

void	assign(void);
void	preinc(void);
void	postinc(void);
void	predec(void);
void	postdec(void);
void	add(void);
void	sub(void);
void	mul(void);
void	div(void);
void	mod(void);
void	and(void);
void	or(void);
void	xor(void);
void	lsh(void);
void	rsh(void);
