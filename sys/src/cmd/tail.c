/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#include	<u.h>
#include	<libc.h>
#include	<ctype.h>
#include	<bio.h>

/*
 * tail command, posix plus v10 option -r.
 * the simple command tail -c, legal in v10, is illegal
 */

int32_t	count;
int	anycount;
int	follow;
int	file	= 0;
char*	umsg	= "usage: tail [-n N] [-c N] [-f] [-r] [+-N[bc][fr]] [file]";

Biobuf	bout;
enum
{
	BEG,
	END
} origin = END;
enum
{
	CHARS,
	LINES
} units = LINES;
enum
{
	FWD,
	REV
} dir = FWD;

extern	void	copy(void);
extern	void	fatal(char*);
extern	int	getnumber(char*);
extern	void	keep(void);
extern	void	reverse(void);
extern	void	skip(void);
extern	void	suffix(char*);
extern	int32_t	tread(char*, int32_t);
extern	void	trunc(Dir*, Dir**);
extern	int64_t	tseek(int64_t, int);
extern	void	twrite(char*, int32_t);
extern	void	usage(void);
static	int	isseekable(int fd);

#define JUMP(o,p) tseek(o,p), copy()

void
main(int argc, char **argv)
{
	Binit(&bout, 1, OWRITE);
	for(int c; argc > 1 && ((c=*argv[1])=='-'||c=='+'); argc--,argv++ ) {
		if(getnumber(argv[1])) {
			suffix(argv[1]);
			continue;
		} else
		if(c == '-')
			switch(argv[1][1]) {
			case 'c':
				units = CHARS;
			case 'n':
				if(getnumber(argv[1]+2))
					continue;
				else
				if(argc > 2 && getnumber(argv[2])) {
					argc--, argv++;
					continue;
				} else
					usage();
			case 'r':
				dir = REV;
				continue;
			case 'f':
				follow++;
				continue;
			case '-':
				argc--, argv++;
			}
		break;
	}

	if(dir==REV && (units==CHARS || follow || origin==BEG))
		fatal("incompatible options");
	if(!anycount)
		count = dir==REV? -1 : 10;
	if(origin==BEG && units==LINES && count>0)
		count--;
	if(argc > 2)
		usage();
	if(argc > 1 && (file=open(argv[1],0)) < 0)
		fatal(argv[1]);

	int seekable = isseekable(file);

	if(!seekable && origin==END)
		keep();
	else
	if(!seekable && origin==BEG)
		skip();
	else
	if(units==CHARS && origin==END)
		JUMP(-count, 2);
	else
	if(units==CHARS && origin==BEG)
		JUMP(count, 0);
	else
	if(units==LINES && origin==END)
		reverse();
	else
	if(units==LINES && origin==BEG)
		skip();
	if(follow && seekable)
		for(;;) {
			static Dir *sb0, *sb1;
			trunc(sb1, &sb0);
			copy();
			trunc(sb0, &sb1);
			sleep(5000);
		}
	exits(0);
}

void
trunc(Dir *old, Dir **new)
{
	Dir *d;
	int64_t olength;

	d = dirfstat(file);
	if(d == nil)
		return;
	olength = 0;
	if(old)
		olength = old->length;
	if(d->length < olength)
		d->length = tseek(0LL, 0);
	free(*new);
	*new = d;
}

void
suffix(char *s)
{
	while(*s && strchr("0123456789+-", *s))
		s++;
	switch(*s) {
	case 'b':
		if((count *= 1024) < 0)
			fatal("too big");
	case 'c':
		units = CHARS;
	case 'l':
		s++;
	}
	switch(*s) {
	case 'r':
		dir = REV;
		return;
	case 'f':
		follow++;
		return;
	case 0:
		return;
	}
	usage();
}

/*
 * read past head of the file to find tail
 */
void
skip(void)
{
	int i;
	int32_t n;
	char buf[Bsize];
	if(units == CHARS) {
		for( ; count>0; count -=n) {
			n = count<Bsize? count: Bsize;
			if(!(n = tread(buf, n)))
				return;
		}
	} else /*units == LINES*/ {
		n = i = 0;
		while(count > 0) {
			if(!(n = tread(buf, Bsize)))
				return;
			for(i=0; i<n && count>0; i++)
				if(buf[i]=='\n')
					count--;
		}
		twrite(buf+i, n-i);
	}
	copy();
}

void
copy(void)
{
	int32_t n;
	char buf[Bsize];
	while((n=tread(buf, Bsize)) > 0) {
		twrite(buf, n);
		Bflush(&bout);	/* for FWD on pipe; else harmless */
	}
}

/*
 * read whole file, keeping the tail
 *	complexity is length(file)*length(tail).
 *	could be linear.
 */
void
keep(void)
{
	int len = 0;
	int32_t bufsiz = 0;
	char *buf = 0;
	int j, k, n;

	for(n=1; n;) {
		if(len+Bsize > bufsiz) {
			bufsiz += 2*Bsize;
			if(!(buf = realloc(buf, bufsiz+1)))
				fatal("out of space");
		}
		for(; n && len<bufsiz; len+=n)
			n = tread(buf+len, bufsiz-len);
		if(count >= len)
			continue;
		if(units == CHARS)
			j = len - count;
		else {
			/* units == LINES */
			j = buf[len-1]=='\n'? len-1: len;
			for(k=0; j>0; j--)
				if(buf[j-1] == '\n')
					if(++k >= count)
						break;
		}
		memmove(buf, buf+j, len-=j);
	}
	if(dir == REV) {
		if(len>0 && buf[len-1]!='\n')
			buf[len++] = '\n';
		for(j=len-1 ; j>0; j--)
			if(buf[j-1] == '\n') {
				twrite(buf+j, len-j);
				if(--count <= 0)
					return;
				len = j;
			}
	}
	if(count > 0)
		twrite(buf, len);
}

/*
 * count backward and print tail of file
 */
void
reverse(void)
{
	int first;
	int32_t len = 0;
	int32_t n = 0;
	int32_t bufsiz = 0;
	char *buf = 0;
	int64_t pos = tseek(0LL, 2);

	for(first=1; pos>0 && count>0; first=0) {
		n = pos>Bsize? Bsize: (int32_t)pos;
		pos -= n;
		if(len+n > bufsiz) {
			bufsiz += 2*Bsize;
			if(!(buf = realloc(buf, bufsiz+1)))
				fatal("out of space");
		}
		memmove(buf+n, buf, len);
		len += n;
		tseek(pos, 0);
		if(tread(buf, n) != n)
			fatal("length error");
		if(first && buf[len-1]!='\n')
			buf[len++] = '\n';
		for(n=len-1 ; n>0 && count>0; n--)
			if(buf[n-1] == '\n') {
				count--;
				if(dir == REV)
					twrite(buf+n, len-n);
				len = n;
			}
	}
	if(dir == FWD) {
		if(n)
			tseek(pos+n+1, 0);
		else
			tseek(0, 0);
		copy();
	} else
	if(count > 0)
		twrite(buf, len);
}

int64_t
tseek(int64_t o, int p)
{
	o = seek(file, o, p);
	if(o == -1)
		fatal("");
	return o;
}

int32_t
tread(char *buf, int32_t n)
{
	int r = read(file, buf, n);
	if(r == -1)
		fatal("");
	return r;
}

void
twrite(char *s, int32_t n)
{
	if(Bwrite(&bout, s, n) != n)
		fatal("");
}

int
getnumber(char *s)
{
	if(*s=='-' || *s=='+')
		s++;
	if(!isdigit(*s))
		return 0;
	if(s[-1] == '+')
		origin = BEG;
	if(anycount++)
		fatal("excess option");
	count = atol(s);

	/* check range of count */
	if(count < 0 ||	(int)count != count)
		fatal("too big");
	return 1;
}	

void		
fatal(char *s)
{
	char buf[ERRMAX];

	errstr(buf, sizeof buf);
	fprint(2, "tail: %s: %s\n", s, buf);
	exits(s);
}

void
usage(void)
{
	fprint(2, "%s\n", umsg);
	exits("usage");
}

/* return true if seeks work and if the file is > 0 length.
 * this will eventually bite me in the ass if seeking a file
 * is not conservative. - presotto
 */
static int
isseekable(int fd)
{	
	int64_t m;

	m = seek(fd, 0, 1);
	if(m < 0)
		return 0;
	return 1;
}
