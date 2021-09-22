#define	EOF	(-1)
#define	NBUF	512

struct io{
	int	fd;
	uchar	*bufp, *ebuf, *strp;
	uchar	buf[NBUF];
	uchar	output;		/* flag */
};
io *err;

io *openfd(int), *openstr(void), *opencore(char *, int);
int emptybuf(io*);
void pchr(io*, int);
int rchr(io*);
int rutf(io*, char*, Rune*);
void rewind(io*);
void closeio(io*);
void flush(io*);
int fullbuf(io*, int);
void pdec(io*, int);
void poct(io*, unsigned);
void pptr(io*, void*);
void pquo(io*, char*);
void pwrd(io*, char*);
void pstr(io*, char*);
void pcmd(io*, tree*);
void pval(io*, word*);
void pfnc(io*, thread*);
void pfmt(io*, char*, ...);

#pragma	varargck	argpos	pfmt		2

#pragma	varargck	type	"%"	void
#pragma	varargck	type	"c"	int
#pragma	varargck	type	"c"	uint
#pragma	varargck	type	"d"	int
#pragma	varargck	type	"d"	uint
#pragma	varargck	type	"o"	int
#pragma	varargck	type	"p"	uintptr
#pragma	varargck	type	"p"	void*
#pragma	varargck	type	"q"	char*
#pragma	varargck	type	"Q"	char*
#pragma	varargck	type	"r"	void
#pragma	varargck	type	"s"	char*
#pragma	varargck	type	"t"	tree*
#pragma	varargck	type	"v"	word*
