#include <u.h>
#include <a.out.h>
#include "fns.h"
#include "mem.h"

void
putc(int c)
{
	cgaputc(c);
	if(uart != -1)
		uartputc(uart, c);
}

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

	putc(' ');
	p = data;
	e = p + len;
	while(p < e){
		if(((ulong)p & 0xF000) == 0){
			putc('\b');
			putc(hex[((ulong)p>>16)&0xF]);
		}
		if((len = read(f, p, e - p)) <= 0)
			break;
		p += len;
	}
	putc('\b');

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

int
getc(void)
{
	int c;

	c = kbdgetc();
	if(c == 0 && uart != -1)
		c = uartgetc(uart);
	return c;
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

static void apmconf(int);
static void e820conf(void);
static void ramdiskconf(int);
static void uartconf(char*);

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

		e820conf();
		ramdiskconf(0);
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
		if(memcmp("apm", line, 3) == 0){
			apmconf(line[3] - '0');
			continue;
		}
		if(memcmp("console", line, 8) == 0)
			uartconf(p);

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


static void
hexfmt(char *s, int i, ulong a)
{
	s += i;
	while(i > 0){
		*--s = hex[a&15];
		a >>= 4;
		i--;
	}
}

static void
addconfx(char *s, int w, ulong v)
{
	int n;

	n = strlen(s);
	memmove(confend, s, n);
	hexfmt(confend+n, w, v);
	confend += n+w;
	*confend = 0;
}

static void
apmconf(int id)
{
	uchar *a;
	char *s;

	a = (uchar*)CONFADDR;
	memset(a, 0, 20);

	apm(id);
	if(memcmp(a, "APM", 4) != 0)
		return;

	s = confend;

	addconfx("apm", 1, id);
	addconfx("=ax=", 4, *((ushort*)(a+4)));
	addconfx(" ebx=", 8, *((ulong*)(a+12)));
	addconfx(" cx=", 4, *((ushort*)(a+6)));
	addconfx(" dx=", 4, *((ushort*)(a+8)));
	addconfx(" di=", 4, *((ushort*)(a+10)));
	addconfx(" esi=", 8, *((ulong*)(a+16)));

	*confend++ = '\n';
	*confend = 0;

	print(s);
}

static void
e820conf(void)
{
	struct {
		uvlong	base;
		uvlong	len;
		ulong	typ;
		ulong	ext;
	} e;
	uvlong v;
	ulong bx;
	char *s;

	bx=0;
	s = confend;

	do{
		bx = e820(bx, &e);
		if(e.len != 0 && (e.ext & 3) == 1){
			if(confend == s){
				/* single entry <= 1MB is useless */
				if(bx == 0 && e.typ == 1 && e.len <= 0x100000)
					break;
				memmove(confend, "*e820=", 6);
				confend += 6;
			}
			addconfx("", 1, e.typ);
			v = e.base;
			addconfx(" 0x", 8, v>>32);
			addconfx("", 8, v&0xffffffff);
			v += e.len;
			addconfx(" 0x", 8, v>>32);
			addconfx("", 8, v&0xffffffff);
			*confend++ = ' ';
		}
	} while(bx);

	if(confend == s)
		return;

	*confend++ = '\n';
	*confend = 0;

	print(s);
}

static int
checksum(void *v, int n)
{
	uchar *p, s;

	s = 0;
	p = v;
	while(n-- > 0)
		s += *p++;
	return s;
}

static void
ramdiskconf(int id)
{
	struct {
		/* ACPI header */
		char	sig[4];
		u32int	len;
		uchar	revision;
		uchar	csum;
		char	oem_id[6];
		char	oem_table_id[8];
		u32int	oem_revision;
		char	asl_compiler_id[4];
		u32int	asl_compiler_revision;

		u32int	safe_hook;

		/* MDI structure */
		u16int	bytes;
		uchar	version_minor;
		uchar	version_major;
		u32int	diskbuf;
		u32int	disksize;
		u32int	cmdline;
		u32int	oldint13;
		u32int	oldint15;
		u16int	olddosmem;
		uchar	bootloaderid;
		uchar	sector_shift;
		u16int	dpt_ptr;
	} *mbft;
	int shift;
	char *s;

#define BDA	((uchar*)0x400)
	mbft = (void*)((((BDA[0x14]<<8) | BDA[0x13])<<10) - 1024);
	for(; (ulong)&mbft->sector_shift < 0xA0000; mbft = (void*)((ulong)mbft + 16)){
		if(memcmp("mBFT", mbft, 4) == 0
		&& mbft->len < 1024 && (uchar*)mbft + mbft->len > &mbft->sector_shift
		&& checksum(mbft, mbft->len) == 0)
			goto Found;
	}
	return;
Found:
	shift = mbft->sector_shift;
	if(shift == 0)
		shift = 9;

	s = confend;
	addconfx("ramdisk", 1, id);
	addconfx("=0x", 8, mbft->diskbuf);
	addconfx(" 0x", 8, mbft->disksize<<shift);
	addconfx(" 0x", 8, 1UL<<shift);

	*confend++ = '\n';
	*confend = 0;

	print(s);
}

static void
uartconf(char *s)
{
	if(*s >= '0' && *s <= '3'){
		uart = *s - '0';
		uartinit(uart, (7<<5) | 3);	/* b9660 l8 s1 */
	} else
		uart = -1;
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

	while(a20() < 0)
		print("a20 enable failed\n");

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
	d = (uchar*)PGROUND((ulong)t);
	memset(t, 0, d - t);
	n = beswal(ex.data);
	if(readn(f, d, n) != n)
		goto Error;
	d += n;
	t = (uchar*)PGROUND((ulong)d);
	t += PGROUND(beswal(ex.bss));
	memset(d, 0, t - d);

	close(f);
	unload();

	print("boot\n");

	jump(e);

Error:		
	return "i/o error";
}
