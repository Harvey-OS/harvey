typedef struct Tube Tube;
typedef struct KTube KTube;
typedef struct KTalt KTalt;

/*
 * [K]Talt.op
 */
enum
{
	TSND = 0x6666,	/* weird numbers, for debugging */
	TRCV,
	TNBSND,
	TNBRCV,
	TNOP
};


struct Tube
{
	int	msz;
	int	tsz;
	int	nmsg;
	int	nhole;
	int	hd;
	int	tl;
};

/*
 * Tubes as seen by the kernel
 */
struct KTube
{
	Tube *t;
	Sem *hole;
	Sem *msg;
};

struct KTalt
{
	KTube *kt;
	void *m;
	int op;
};

extern void	freetube(KTube *t);
extern int	nbtrecv(KTube *t, void *p);
extern int	nbtsend(KTube *t, void *p);
extern int	talt(KTalt a[], int na);
extern void	trecv(KTube *t, void *p);
extern void	tsend(KTube *t, void *p);
extern KTube*	u2ktube(Tube *t);
extern void	upsem(Sem*);
extern int	downsem(Sem*, int);
extern int	altsems(Sem*[], int);
