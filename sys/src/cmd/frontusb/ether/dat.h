typedef struct Block Block;
struct Block
{
	Block	*next;

	uchar	*rp;
	uchar	*wp;
	uchar	*lim;

	uchar	base[];
};

#define BLEN(s)	((s)->wp - (s)->rp)

Block*	allocb(int size);
Block*	copyblock(Block*, int);
#define	freeb(b) free(b)

enum {
	Eaddrlen=	6,
	ETHERHDRSIZE=	14,		/* size of an ethernet header */
	Maxpkt=		2000,
};

typedef struct Macent Macent;
struct Macent
{
	uchar	ea[Eaddrlen];
	ushort	port;
};

typedef struct Etherpkt Etherpkt;
struct Etherpkt
{
	uchar	d[Eaddrlen];
	uchar	s[Eaddrlen];
	uchar	type[2];
	uchar	data[1500];
};

enum
{
	Cdcunion = 6,
	Scether = 6,
	Fnether = 15,
};

int debug;
int setmac;

int nprom;
int nmulti;
uchar multiaddr[32][Eaddrlen];

/* to be filled in by *init() */
uchar macaddr[Eaddrlen];

Macent mactab[127];

void	etheriq(Block*);

int	(*epreceive)(Dev*);
void	(*eptransmit)(Dev*, Block*);
int 	(*eppromiscuous)(Dev*, int);
int	(*epmulticast)(Dev*, uchar*, int);
