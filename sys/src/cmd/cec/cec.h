/* cec.h: definitions for cec */

typedef struct {
	uchar	dst[6];
	uchar	src[6];
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
	Floc,
};

typedef struct Mux Mux;
#pragma incomplete Mux;

enum {
	Iowait		= 2000,
	Etype 		= 0xbcbc,
};
int debug;

Mux *mux(int fd[2]);
int muxread(Mux*, Pkt*);
void muxfree(Mux*);

int netopen(char *name);
int netsend(void *, int);
int netget(void *, int);

void rawon(void);
void rawoff(void);
void dump(uchar*, int);
void exits0(char*);
