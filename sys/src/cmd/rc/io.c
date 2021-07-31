#include "rc.h"
#include "exec.h"
#include "io.h"
#include "fns.h"

enum { Stralloc = 100, };

int pfmtnest = 0;

void
pfmt(io *f, char *fmt, ...)
{
	va_list ap;
	char err[ERRMAX];

	va_start(ap, fmt);
	pfmtnest++;
	for(;*fmt;fmt++) {
		if(*fmt!='%') {
			pchr(f, *fmt);
			continue;
		}
		if(*++fmt == '\0')		/* "blah%"? */
			break;
		switch(*fmt){
		case 'c':
			pchr(f, va_arg(ap, int));
			break;
		case 'd':
			pdec(f, va_arg(ap, int));
			break;
		case 'o':
			poct(f, va_arg(ap, unsigned));
			break;
		case 'p':
			pptr(f, va_arg(ap, void*));
			break;
		case 'Q':
			pquo(f, va_arg(ap, char *));
			break;
		case 'q':
			pwrd(f, va_arg(ap, char *));
			break;
		case 'r':
			errstr(err, sizeof err); pstr(f, err);
			break;
		case 's':
			pstr(f, va_arg(ap, char *));
			break;
		case 't':
			pcmd(f, va_arg(ap, struct tree *));
			break;
		case 'v':
			pval(f, va_arg(ap, struct word *));
			break;
		default:
			pchr(f, *fmt);
			break;
		}
	}
	va_end(ap);
	if(--pfmtnest==0)
		flush(f);
}

void
pchr(io *b, int c)
{
	if(b->bufp==b->ebuf)
		fullbuf(b, c);
	else *b->bufp++=c;
}

int
rchr(io *b)
{
	if(b->bufp==b->ebuf)
		return emptybuf(b);
	return *b->bufp++;
}

int
rutf(io *b, char *buf, Rune *r)
{
	int n, i, c;

	c = rchr(b);
	if(c == EOF)
		return EOF;
	*buf = c;
	if(c < Runesync){
		*r = c;
		return 1;
	}
	for(i = 1; (c = rchr(b)) != EOF; ){
		buf[i++] = c;
		buf[i] = 0;
		if(fullrune(buf, i)){
			n = chartorune(r, buf);
			b->bufp -= i - n;	/* push back unconsumed bytes */
			assert(b->fd == -1 || b->bufp > b->buf);
			return n;
		}
	}
	/* at eof */
	b->bufp -= i - 1;			/* consume 1 byte */
	*r = Runeerror;
	return runetochar(buf, r);
}

void
pquo(io *f, char *s)
{
	pchr(f, '\'');
	for(;*s;s++)
		if(*s=='\'')
			pfmt(f, "''");
		else pchr(f, *s);
	pchr(f, '\'');
}

void
pwrd(io *f, char *s)
{
	char *t;
	for(t = s;*t;t++) if(*t >= 0 && needsrcquote(*t)) break;
	if(t==s || *t)
		pquo(f, s);
	else pstr(f, s);
}

void
pptr(io *f, void *v)
{
	int n;
	uintptr p;

	p = (uintptr)v;
	if(sizeof(uintptr) == sizeof(uvlong) && p>>32)
		for(n = 60;n>=32;n-=4) pchr(f, "0123456789ABCDEF"[(p>>n)&0xF]);

	for(n = 28;n>=0;n-=4) pchr(f, "0123456789ABCDEF"[(p>>n)&0xF]);
}

void
pstr(io *f, char *s)
{
	if(s==0)
		s="(null)";
	while(*s) pchr(f, *s++);
}

void
pdec(io *f, int n)
{
	if(n<0){
		n=-n;
		if(n>=0){
			pchr(f, '-');
			pdec(f, n);
			return;
		}
		/* n is two's complement minimum integer */
		n = 1-n;
		pchr(f, '-');
		pdec(f, n/10);
		pchr(f, n%10+'1');
		return;
	}
	if(n>9)
		pdec(f, n/10);
	pchr(f, n%10+'0');
}

void
poct(io *f, unsigned n)
{
	if(n>7)
		poct(f, n>>3);
	pchr(f, (n&7)+'0');
}

void
pval(io *f, word *a)
{
	if(a){
		while(a->next && a->next->word){
			pwrd(f, (char *)a->word);
			pchr(f, ' ');
			a = a->next;
		}
		pwrd(f, (char *)a->word);
	}
}

int
fullbuf(io *f, int c)
{
	flush(f);
	return *f->bufp++=c;
}

void
flush(io *f)
{
	int n;

	if(f->strp){
		n = f->ebuf - f->strp;
		f->strp = realloc(f->strp, n+Stralloc+1);
		if(f->strp==0)
			panic("Can't realloc %d bytes in flush!", n+Stralloc+1);
		f->bufp = f->strp + n;
		f->ebuf = f->bufp + Stralloc;
		memset(f->bufp, '\0', Stralloc+1);
	}
	else{
		n = f->bufp-f->buf;
		if(n && Write(f->fd, f->buf, n) != n){
			Write(2, "Write error\n", 12);
			if(ntrap)
				dotrap();
		}
		f->bufp = f->buf;
		f->ebuf = f->buf+NBUF;
	}
}

io*
openfd(int fd)
{
	io *f = new(struct io);
	f->fd = fd;
	f->bufp = f->ebuf = f->buf;
	f->strp = 0;
	return f;
}

io*
openstr(void)
{
	io *f = new(struct io);

	f->fd = -1;
	f->bufp = f->strp = emalloc(Stralloc+1);
	f->ebuf = f->bufp + Stralloc;
	memset(f->bufp, '\0', Stralloc+1);
	return f;
}
/*
 * Open a corebuffer to read.  EOF occurs after reading len
 * characters from buf.
 */

io*
opencore(char *s, int len)
{
	io *f = new(struct io);
	uchar *buf = emalloc(len);

	f->fd = -1 /*open("/dev/null", 0)*/;
	f->bufp = f->strp = buf;
	f->ebuf = buf+len;
	Memcpy(buf, s, len);
	return f;
}

void
rewind(io *io)
{
	if(io->fd==-1)
		io->bufp = io->strp;
	else{
		io->bufp = io->ebuf = io->buf;
		Seek(io->fd, 0L, 0);
	}
}

void
closeio(io *io)
{
	if(io->fd>=0)
		close(io->fd);
	if(io->strp)
		efree(io->strp);
	efree(io);
}

int
emptybuf(io *f)
{
	int n;
	if(f->fd==-1 || (n = Read(f->fd, f->buf, NBUF))<=0) return EOF;
	f->bufp = f->buf;
	f->ebuf = f->buf + n;
	return *f->bufp++;
}
