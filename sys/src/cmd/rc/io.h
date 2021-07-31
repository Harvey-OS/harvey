#define	EOF	(-1)
#define	NBUF	512

struct io{
	int	fd;
	uchar	*bufp, *ebuf, *strp;
	uchar	buf[NBUF];
};
io *err;

io *openfd(int), *openstr(void), *opencore(char *, int);
int emptybuf(io*);
void pchr(io*, int);
int rchr(io*);
int rutf(io*, char*, Rune*);
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
