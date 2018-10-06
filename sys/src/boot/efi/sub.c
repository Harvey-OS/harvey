#include <u.h>
#include <a.out.h>
#include "fns.h"
#include "mem.h"

char hex[] = "0123456789abcdef";

void
print(char *s)
{
	while(*s != 0){
		if(*s == '\n')
			putc('\r');
		putc(*s++);
	}
}

int
readn(void *f, void *data, int len)
{
	uchar *p, *e;

	p = data;
	e = p + len;
	while(p < e){
		if((len = read(f, p, e - p)) <= 0)
			break;
		p += len;
	}

	return p - (uchar*)data;
}

void
memmove(void *dst, void *src, int n)
{
	uchar *d = dst;
	uchar *s = src;

	if(d < s){
		while(n-- > 0)
			*d++ = *s++;
	} else if(d > s){
		s += n;
		d += n;
		while(n-- > 0)
			*--d = *--s;
	}
}

int
memcmp(void *src, void *dst, int n)
{
	uchar *d = dst;
	uchar *s = src;
	int r = 0;

	while(n-- > 0){
		r = *d++ - *s++;
		if(r != 0)
			break;
	}

	return r;
}

int
strlen(char *s)
{
	char *p = s;

	while(*p != '\0')
		p++;

	return p - s;
}

char*
strchr(char *s, int c)
{
	for(; *s != 0; s++)
		if(*s == c)
			return s;

	return nil;
}

void
memset(void *dst, int v, int n)
{
	uchar *d = dst;

	while(n > 0){
		*d++ = v;
		n--;
	}
}

static int
readline(void *f, char *buf)
{
	static char white[] = "\t ";
	char *p;

	p = buf;
	do{
		if(f == nil)
			putc('>');
		for(;;){
			if(f == nil){
				while((*p = getc()) == 0)
					;
				if(p == buf && (*p == '\b' || strchr(white, *p) != nil))
					continue;
				putc(*p);
				if(*p == '\r')
					putc('\n');
				else if(*p == '\b'){
					p--;
					putc(' ');
					putc('\b');
					continue;
				}
			}else if(read(f, p, 1) <= 0)
				return 0;
			if(strchr("\r\n", *p) != nil)
				break;
			if(p == buf && strchr(white, *p) != nil)
				continue;	/* whitespace on start of line */
			p++;
		}
		while(p > buf && strchr(white, p[-1]))
			p--;
	}while(p == buf);
	*p = 0;

	return p - buf;
}

static int
timeout(int ms)
{
	while(ms > 0){
		if(getc() != 0)
			return 1;
		usleep(100000);
		ms -= 100;
	}
	return 0;
}

#define BOOTLINE	((char*)CONFADDR)
#define BOOTLINELEN	64
#define BOOTARGS	((char*)(CONFADDR+BOOTLINELEN))
#define	BOOTARGSLEN	(4096-0x200-BOOTLINELEN)

char *confend;

static char*
getconf(char *s, char *buf)
{
	char *p, *e;
	int n;

	n = strlen(s);
	for(p = BOOTARGS; p < confend; p = e+1){
		for(e = p+1; e < confend; e++)
			if(*e == '\n')
				break;
		if(memcmp(p, s, n) == 0){
			p += n;
			n = e - p;
			buf[n] = 0;
			memmove(buf, p, n);
			return buf;
		}
	}
	return nil;
}

static int
delconf(char *s)
{
	char *p, *e;

	for(p = BOOTARGS; p < confend; p = e){
		for(e = p+1; e < confend; e++){
			if(*e == '\n'){
				e++;
				break;
			}
		}
		if(memcmp(p, s, strlen(s)) == 0){
			memmove(p, e, confend - e);
			confend -= e - p;
			*confend = 0;
			return 1;
		}
	}
	return 0;
}

char*
configure(void *f, char *path)
{
	char *line, *kern, *s, *p;
	int inblock, nowait, n;
	static int once = 1;

	if(once){
		once = 0;
Clear:
		memset(BOOTLINE, 0, BOOTLINELEN);

		confend = BOOTARGS;
		memset(confend, 0, BOOTARGSLEN);
		eficonfig(&confend);
	}
	nowait = 1;
	inblock = 0;
Loop:
	while(readline(f, line = confend+1) > 0){
		if(*line == 0 || strchr("#;=", *line) != nil)
			continue;
		if(*line == '['){
			inblock = memcmp("[common]", line, 8) != 0;
			continue;
		}
		if(memcmp("boot", line, 5) == 0){
			nowait=1;
			break;
		}
		if(memcmp("wait", line, 5) == 0){
			nowait=0;
			continue;
		}
		if(memcmp("show", line, 5) == 0){
			print(BOOTARGS);
			continue;
		}
		if(memcmp("clear", line, 5) == 0){
			if(line[5] == '\0'){
				print("ok\n");
				goto Clear;
			} else if(line[5] == ' ' && delconf(line+6))
				print("ok\n");
			continue;
		}
		if(inblock != 0 || (p = strchr(line, '=')) == nil)
			continue;
		*p++ = 0;
		delconf(line);
		s = confend;
		memmove(confend, line, n = strlen(line)); confend += n;
		*confend++ = '=';
		memmove(confend, p, n = strlen(p)); confend += n;
		*confend++ = '\n';
		*confend = 0;
		print(s);
	}
	kern = getconf("bootfile=", path);

	if(f != nil){
		close(f);
		f = nil;

		if(kern != nil && (nowait==0 || timeout(1000)))
			goto Loop;
	}

	if(kern == nil){
		print("no bootfile\n");
		goto Loop;
	}
	while((p = strchr(kern, '!')) != nil)
		kern = p+1;

	return kern;
}

static char*
numfmt(char *s, ulong b, ulong i, ulong a)
{
	char *r;

	if(i == 0){
		ulong v = a;
		while(v != 0){
			v /= b;
			i++;
		}
		if(i == 0)
			i = 1;
	}

	s += i;
	r = s;
	while(i > 0){
		*--s = hex[a % b];
		a /= b;
		i--;
	}
	return r;
}

char*
hexfmt(char *s, int i, uvlong a)
{
	if(i > 8){
		s = numfmt(s, 16, i-8, a>>32);
		i = 8;
	}
	return numfmt(s, 16, i, a);
}

char*
decfmt(char *s, int i, ulong a)
{
	return numfmt(s, 10, i, a);
}

static ulong
beswal(ulong l)
{
	uchar *p = (uchar*)&l;
	return (p[0]<<24) | (p[1]<<16) | (p[2]<<8) | p[3];
}

char*
bootkern(void *f)
{
	uchar *e, *d, *t;
	ulong n;
	Exec ex;

	if(readn(f, &ex, sizeof(ex)) != sizeof(ex))
		return "bad header";

	e = (uchar*)(beswal(ex.entry) & ~0xF0000000UL);
	switch(beswal(ex.magic)){
	case S_MAGIC:
		if(readn(f, e, 8) != 8)
			goto Error;
	case I_MAGIC:
		break;
	default:
		return "bad magic";
	}

	t = e;
	n = beswal(ex.text);
	if(readn(f, t, n) != n)
		goto Error;
	t += n;
	d = (uchar*)PGROUND((uintptr)t);
	memset(t, 0, d - t);
	n = beswal(ex.data);
	if(readn(f, d, n) != n)
		goto Error;
	d += n;
	t = (uchar*)PGROUND((uintptr)d);
	t += PGROUND(beswal(ex.bss));
	memset(d, 0, t - d);

	close(f);
	print("boot\n");
	unload();

	jump(e);

Error:		
	return "i/o error";
}
