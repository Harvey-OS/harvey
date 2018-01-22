/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#include 	<u.h>
#include 	<libc.h>

typedef struct PCB	/* Control block controlling specification parse */
{
	char	*base;		/* start of specification */
	char	*current;	/* current parse point */
	int32_t	last;		/* last Rune returned */
	int32_t	final;		/* final Rune in a span */
} Pcb;

uint8_t	bits[] = { 1, 2, 4, 8, 16, 32, 64, 128 };

#define	SETBIT(a, c)		((a)[(c)/8] |= bits[(c)&07])
#define	CLEARBIT(a,c)		((a)[(c)/8] &= ~bits[(c)&07])
#define	BITSET(a,c)		((a)[(c)/8] & bits[(c)&07])

uint8_t	f[(Runemax+1)/8];
uint8_t	t[(Runemax+1)/8];
char 	wbuf[4096];
char	*wptr;

Pcb pfrom, pto;

int cflag;
int dflag;
int sflag;

void	complement(void);
void	delete(void);
void	squeeze(void);
void	translit(void);
int32_t	canon(Pcb*);
char	*getrune(char*, Rune*);
void	Pinit(Pcb*, char*);
void	Prewind(Pcb *p);
int	readrune(int, int32_t*);
void	wflush(int);
void	writerune(int, Rune);

static void
usage(void)
{
	fprint(2, "usage: %s [-cds] [string1 [string2]]\n", argv0);
	exits("usage");
}

void
main(int argc, char **argv)
{
	ARGBEGIN{
	case 's':	sflag++; break;
	case 'd':	dflag++; break;
	case 'c':	cflag++; break;
	default:	usage();
	}ARGEND
	if(argc>0)
		Pinit(&pfrom, argv[0]);
	if(argc>1)
		Pinit(&pto, argv[1]);
	if(argc>2)
		usage();
	if(dflag) {
		if ((sflag && argc != 2) || (!sflag && argc != 1))
			usage();
		delete();
	} else {
		if (argc != 2)
			usage();
		if (cflag)
			complement();
		else translit();
	}
	exits(0);
}

void
delete(void)
{
	int32_t c, last;

	if (cflag) {
		memset((char *) f, 0xff, sizeof f);
		while ((c = canon(&pfrom)) >= 0)
			CLEARBIT(f, c);
	} else {
		while ((c = canon(&pfrom)) >= 0)
			SETBIT(f, c);
	}
	if (sflag) {
		while ((c = canon(&pto)) >= 0)
			SETBIT(t, c);
	}

	last = 0x10000;
	while (readrune(0, &c) > 0) {
		if(!BITSET(f, c) && (c != last || !BITSET(t,c))) {
			last = c;
			writerune(1, (Rune) c);
		}
	}
	wflush(1);
}

void
complement(void)
{
	Rune *p;
	int i;
	int32_t from, to, lastc, high;

	lastc = 0;
	high = 0;
	while ((from = canon(&pfrom)) >= 0) {
		if (from > high) high = from;
		SETBIT(f, from);
	}
	while ((to = canon(&pto)) > 0) {
		if (to > high) high = to;
		SETBIT(t,to);
	}
	Prewind(&pto);
	if ((p = (Rune *) malloc((high+1)*sizeof(Rune))) == 0)
		sysfatal("no memory");
	for (i = 0; i <= high; i++){
		if (!BITSET(f,i)) {
			if ((to = canon(&pto)) < 0)
				to = lastc;
			else lastc = to;
			p[i] = to;
		}
		else p[i] = i;
	}
	if (sflag){
		lastc = 0x10000;
		while (readrune(0, &from) > 0) {
			if (from > high)
				from = to;
			else
				from = p[from];
			if (from != lastc || !BITSET(t,from)) {
				lastc = from;
				writerune(1, (Rune) from);
			}
		}

	} else {
		while (readrune(0, &from) > 0){
			if (from > high)
				from = to;
			else
				from = p[from];
			writerune(1, (Rune) from);
		}
	}
	wflush(1);
}

void
translit(void)
{
	Rune *p;
	int i;
	int32_t from, to, lastc, high;

	lastc = 0;
	high = 0;
	while ((from = canon(&pfrom)) >= 0)
		if (from > high) high = from;
	Prewind(&pfrom);
	if ((p = (Rune *) malloc((high+1)*sizeof(Rune))) == 0)
		sysfatal("no memory");
	for (i = 0; i <= high; i++)
		p[i] = i;
	while ((from = canon(&pfrom)) >= 0) {
		if ((to = canon(&pto)) < 0)
			to = lastc;
		else lastc = to;
		if (BITSET(f,from) && p[from] != to)
			sysfatal("ambiguous translation");
		SETBIT(f,from);
		p[from] = to;
		SETBIT(t,to);
	}
	while ((to = canon(&pto)) >= 0) {
		SETBIT(t,to);
	}
	if (sflag){
		lastc = 0x10000;
		while (readrune(0, &from) > 0) {
			if (from <= high)
				from = p[from];
			if (from != lastc || !BITSET(t,from)) {
				lastc = from;
				writerune(1, (Rune) from);
			}
		}

	} else {
		while (readrune(0, &from) > 0) {
			if (from <= high)
				from = p[from];
			writerune(1, (Rune) from);
		}
	}
	wflush(1);
}

int
readrune(int fd, int32_t *rp)
{
	Rune r;
	int j;
	static int i, n;
	static char buf[4096];

	j = i;
	for (;;) {
		if (i >= n) {
			wflush(1);
			if (j != i)
				memcpy(buf, buf+j, n-j);
			i = n-j;
			n = read(fd, &buf[i], sizeof(buf)-i);
			if (n < 0)
				sysfatal("read error: %r");
			if (n == 0)
				return 0;
			j = 0;
			n += i;
		}
		i++;
		if (fullrune(&buf[j], i-j))
			break;
	}
	chartorune(&r, &buf[j]);
	*rp = r;
	return 1;
}

void
writerune(int fd, Rune r)
{
	char buf[UTFmax];
	int n;

	if (!wptr)
		wptr = wbuf;
	n = runetochar(buf, (Rune*)&r);
	if (wptr+n >= wbuf+sizeof(wbuf))
		wflush(fd);
	memcpy(wptr, buf, n);
	wptr += n;
}

void
wflush(int fd)
{
	if (wptr && wptr > wbuf)
		if (write(fd, wbuf, wptr-wbuf) != wptr-wbuf)
			sysfatal("write error: %r");
	wptr = wbuf;
}

char *
getrune(char *s, Rune *rp)
{
	Rune r;
	char *save;
	int i, n;

	s += chartorune(rp, s);
	if((r = *rp) == '\\' && *s){
		n = 0;
		if (*s == 'x') {
			s++;
			for (i = 0; i < 4; i++) {
				save = s;
				s += chartorune(&r, s);
				if ('0' <= r && r <= '9')
					n = 16*n + r - '0';
				else if ('a' <= r && r <= 'f')
					n = 16*n + r - 'a' + 10;
				else if ('A' <= r && r <= 'F')
					n = 16*n + r - 'A' + 10;
				else {
					if (i == 0)
						*rp = 'x';
					else *rp = n;
					return save;
				}
			}
		} else {
			for(i = 0; i < 3; i++) {
				save = s;
				s += chartorune(&r, s);
				if('0' <= r && r <= '7')
					n = 8*n + r - '0';
				else {
					if (i == 0)
					{
						*rp = r;
						return s;
					}
					*rp = n;
					return save;
				}
			}
			if(n > 0377)
				sysfatal("character > 0377");
		}
		*rp = n;
	}
	return s;
}

int32_t
canon(Pcb *p)
{
	Rune r;

	if (p->final >= 0) {
		if (p->last < p->final)
			return ++p->last;
		p->final = -1;
	}
	if (*p->current == '\0')
		return -1;
	if(*p->current == '-' && p->last >= 0 && p->current[1]){
		p->current = getrune(p->current+1, &r);
		if (r < p->last)
			sysfatal("invalid range specification");
		if (r > p->last) {
			p->final = r;
			return ++p->last;
		}
	}
	p->current = getrune(p->current, &r);
	p->last = r;
	return p->last;
}

void
Pinit(Pcb *p, char *cp)
{
	p->current = p->base = cp;
	p->last = p->final = -1;
}
void
Prewind(Pcb *p)
{
	p->current = p->base;
	p->last = p->final = -1;
}
