#define	EOF	(-1)
#define	pchr(b, c) ((b)->bufp==(b)->ebuf?fullbuf((b), (c)):(*(b)->bufp++=(c)))
#define	rchr(b) ((b)->bufp==(b)->ebuf?emptybuf(b):(*(b)->bufp++&0xff))
#define	NBUF	512
typedef struct io io;
struct io{
	int fd;
	char *bufp, *ebuf, *strp, buf[NBUF];
};
io *err;
io *openfd(int), *openstr(void), *opencore(char *, int);
int emptybuf(io*);
void closeio(io*);
void flush(io*);
int fullbuf(io*, int);
void pdec(io*, long);
void poct(io*, ulong);
void phex(io*, long);
void pquo(io*, char*);
void pwrd(io*, char*);
void pstr(io*, char*);
void pcmd(io*, tree*);
void pval(io*, word*);
void pfnc(io*, thread*);
void pfmt(io*, char*, ...);
