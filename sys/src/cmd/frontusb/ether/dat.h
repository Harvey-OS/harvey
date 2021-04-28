typedef struct Block Block;
struct Block
{
	Block	*next;

	u8	*rp;
	u8	*wp;
	u8	*lim;

	u8	base[];
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
	u8	ea[Eaddrlen];
	u16	port;
};

typedef struct Etherpkt Etherpkt;
struct Etherpkt
{
	u8	d[Eaddrlen];
	u8	s[Eaddrlen];
	u8	type[2];
	u8	data[1500];
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
u8 multiaddr[32][Eaddrlen];

/* to be filled in by *init() */
u8 macaddr[Eaddrlen];

Macent mactab[127];

void	etheriq(Block*);

int	(*epreceive)(Dev*);
void	(*eptransmit)(Dev*, Block*);
int 	(*eppromiscuous)(Dev*, int);
int	(*epmulticast)(Dev*, u8*, int);
