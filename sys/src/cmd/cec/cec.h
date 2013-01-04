enum {
	Eaddrlen	= 6,
};

typedef struct {
	uchar	dst[Eaddrlen];
	uchar	src[Eaddrlen];
	ushort	etype;
	uchar	type;
	uchar	conn;
	uchar	seq;
	uchar	len;
	uchar	data[1500];
} Pkt;

enum {
	Fkbd,
	Fcec,
	Ftimeout,
	Ftimedout,
	Ffatal,
};

typedef struct Mux Mux;
#pragma incomplete Mux;

enum{
	Iowait		= 2000,
	Etype 		= 0xbcbc,
};
int debug;

Mux	*mux(int fd[2]);
void	muxfree(Mux*);
int	muxread(Mux*, Pkt*);
void	muxtimeout(Mux*, int);

int	netget(void *, int);
int	netopen(char *name);
int	netsend(void *, int);

void	dump(uchar*, int);
void	exits0(char*);
void	rawoff(void);
void	rawon(void);
