#include <u.h>
#include <libc.h>
#include <bio.h>
#include <mach.h>
#include <ctype.h>
#define Extern extern
#include "mips.h"

char	buf[128], lastcmd[128];
char	fmt = 'X';
int	width = 60;
int	inc;

ulong	expr(char*);
ulong	expr1(char*);
char*	term(char*, ulong*);

char *
nextc(char *p)
{
	while(*p && (*p == ' ' || *p == '\t') && *p != '\n')
		p++;

	if(*p == '\n')
		*p = '\0';

	return p;
}

char *
numsym(char *addr, ulong *val)
{
	char tsym[128], *t;
	static char *delim = "`'<>/\\@*|-~+-/=?\n";
	Symbol s;
	char c;

	t = tsym;
	while(c = *addr) {
		if(strchr(delim, c))
			break;
		*t++ = c;
		addr++;
	}
	t[0] = '\0';

	if(strcmp(tsym, ".") == 0) {
		*val = dot;
		return addr;
	}

	if(lookup(0, tsym, &s))
		*val = s.value;
	else {
		if(tsym[0] == '#')
			*val = strtoul(tsym+1, 0, 16);
		else
			*val = strtoul(tsym, 0, 0);
	}
	return addr;
}

ulong
expr(char *addr)
{
	ulong t, t2;
	char op;

	if(*addr == '\0')
		return dot;

	addr = numsym(addr, &t);

	if(*addr == '\0')
		return t;

	addr = nextc(addr);
	op = *addr++;
	numsym(addr, &t2);
	switch(op) {
	default:
		Bprint(bioout, "expr syntax\n");
		return 0;
	case '+':
		t += t2;
		break;
	case '-':
		t -= t2;
		break;
	case '%':
		t /= t2;
		break;
	case '&':
		t &= t2;
		break;
	case '|':
		t |= t2;
		break;
	}

	return t;
}

int
buildargv(char *str, char **args, int max)
{
	int na = 0;

	while (na < max) {
		while((*str == ' ' || *str == '\t' || *str == '\n') && *str != '\0')
			str++;

		if(*str == '\0')
			return na;

		args[na++] = str;
		while(!(*str == ' ' || *str == '\t'|| *str == '\n') && *str != '\0')
			str++;

		if(*str == '\n')
			*str = '\0';

		if(*str == '\0')
			break;

		*str++ = '\0';
	}
	return na;
}

void
colon(char *addr, char *cp)
{
	int argc;
	char *argv[100];
	char tbuf[512];

	cp = nextc(cp);
	switch(*cp) {
	default:
		Bprint(bioout, "?\n");
		return;
	case 'b':
		breakpoint(addr, cp+1);
		return;

	case 'd':
		delbpt(addr);
		return;

	/* These fall through to print the stopped address */
	case 'r':
		reset();
		argc = buildargv(cp+1, argv, 100);
		initstk(argc, argv);
		count = 0;
		atbpt = 0;
		run();
		break;
	case 'c':
		count = 0;
		atbpt = 0;
		run();
		break;
	case 's':
		cp = nextc(cp+1);
		count = 0;
		if(*cp)
			count = strtoul(cp, 0, 0);
		if(count == 0)
			count = 1;
		atbpt = 0;
		run();
		break;
	}

	dot = reg.pc;
	Bprint(bioout, "%s at #%lux ", atbpt ? "breakpoint" : "stopped", dot);
	symoff(tbuf, sizeof(tbuf), dot, CTEXT);
	Bprint(bioout, tbuf);
	if(fmt == 'z')
		printsource(dot);

	Bprint(bioout, "\n");
}


void
dollar(char *cp)
{
	int nr;

	cp = nextc(cp);
	switch(*cp) {
	default:
		Bprint(bioout, "?\n");
		break;

	case 'c':
		stktrace(*cp);
		break;

	case 'C':
		stktrace(*cp);
		break;
		
	case 'b':
		dobplist();
		break;

	case 'r':
		dumpreg();
		break;

	case 'R':
		dumpreg();

	case 'f':
		dumpfreg();
		break;

	case 'F':
		dumpdreg();
		break;

	case 'q':
		exits(0);
		break;

	case 'Q':
		isum();
		tlbsum();
		segsum();
		break;

	case 't':
		cp++;
		switch(*cp) {
		default:
			Bprint(bioout, "$t[0sicr#]\n");
			break;
		case '\0':
			trace = 1;
			break;
		case '0':
			trace = 0;
			sysdbg = 0;
			calltree = 0;
			break;
		case 's':
			sysdbg = 1;
			break;
		case 'i':
			trace = 1;
			break;
		case 'c':
			calltree = 1;
			break;
		case 'r':
			nr = atoi(cp+1);
			if(nr < 0 || nr > 31) {
				print("bad register\n");
				break;
			}
			rtrace ^= (1<<nr);
			print("%.8ux\n", rtrace);
			break;
		}
		break;

	case 'i':
		cp++;
		switch(*cp) {
		default:
			Bprint(bioout, "$i[itsa]\n");
			break;
		case 'i':
			isum();
			break;
		case 't':
			tlbsum();
			break;
		case 's':
			segsum();
			break;
		case 'a':
			isum();
			tlbsum();
			segsum();
			iprofile();
			break;
		case 'p':
			iprofile();
			break;
		}
	}
}

int
pfmt(char fmt, int mem, ulong val)
{
	int c, i;
	Symbol s;
	char *p, ch, str[1024];

	c = 0;
	switch(fmt) {
	default:
		Bprint(bioout, "bad modifier\n");
		return 0;
	case 'o':
		c = Bprint(bioout, "%-4lo ", mem ? (ushort)getmem_2(dot) : val);
		inc = 2;
		break;

	case 'O':
		c = Bprint(bioout, "%-8lo ", mem ? getmem_4(dot) : val);
		inc = 4;
		break;

	case 'q':
		c = Bprint(bioout, "%-4lo ", mem ? (short)getmem_2(dot) : val);
		inc = 2;
		break;

	case 'Q':
		c = Bprint(bioout, "%-8lo ", mem ? (long)getmem_4(dot) : val);
		inc = 4;
		break;

	case 'd':
		c = Bprint(bioout, "%-5ld ", mem ? (short)getmem_2(dot) : val);
		inc = 2;
		break;


	case 'D':
		c = Bprint(bioout, "%-8ld ", mem ? (long)getmem_4(dot) : val);
		inc = 4;
		break;

	case 'x':
		c = Bprint(bioout, "#%-4lux ", mem ? (long)getmem_2(dot) : val);
		inc = 2;
		break;

	case 'X':
		c = Bprint(bioout, "#%-8lux ", mem ? (long)getmem_4(dot) : val);
		inc = 4;
		break;

	case 'u':
		c = Bprint(bioout, "%-5ld ", mem ? (ushort)getmem_2(dot) : val);
		inc = 2;
		break;

	case 'U':
		c = Bprint(bioout, "%-8ld ", mem ? (ulong)getmem_4(dot) : val);
		inc = 4;
		break;

	case 'b':
		c = Bprint(bioout, "%-3ld ", mem ? getmem_b(dot) : val);
		inc = 1;
		break;

	case 'c':
		c = Bprint(bioout, "%c ", (int)(mem ? getmem_b(dot) : val));
		inc = 1;
		break;

	case 'C':
		ch = mem ? getmem_b(dot) : val;
		if(isprint(ch))
			c = Bprint(bioout, "%c ", ch);
		else
			c = Bprint(bioout, "\\x%.2x ", ch);
		inc = 1;
		break;

	case 's':
		i = 0;
		while(ch = getmem_b(dot+i))
			str[i++] = ch;
		str[i] = '\0';
		dot += i;
		c = Bprint(bioout, "%s", str);
		inc = 0;
		break;

	case 'S':
		i = 0;
		while(ch = getmem_b(dot+i))
			str[i++] = ch;
		str[i] = '\0';
		dot += i;
		for(p = str; *p; p++)
			if(isprint(*p))
				c += Bprint(bioout, "%c", *p);
			else
				c += Bprint(bioout, "\\x%.2ux", *p);
		inc = 0;
		break;

	case 'Y':
		p = ctime(mem ? getmem_b(dot) : val);
		p[30] = '\0';
		c = Bprint(bioout, "%s", p);
		inc = 4;
		break;

	case 'a':
		symoff(str, sizeof(str), dot, CTEXT);
		c = Bprint(bioout, str);
		inc = 0;
		break;

	case 'e':
		for (i = 0; globalsym(&s, i); i++)
			Bprint(bioout, "%-15s #%lux\n", s.name,	getmem_4(s.value));
		inc = 0;
		break;

	case 'i':
		inc = _mipscoinst(symmap, dot, str, sizeof(str));
		if (inc < 0) {
			Bprint(bioout, "vi: %r\n");
			return 0;
		}
		c = Bprint(bioout, str);
		break;

	case 'n':
		c = width+1;
		inc = 0;
		break;

	case '-':
		c = 0;
		inc = -1;
		break;

	case '+':
		c = 0;
		inc = 1;
		break;

	case '^':
		c = 0;
		if(inc > 0)
			inc = -inc;
		break;

	case 'z':
		if (findsym(dot, CTEXT, &s))
			Bprint(bioout, "  %s() ", s.name);
		printsource(dot);
		inc = 0;
		break;
	}
	return c;
}

void
eval(char *addr, char *p)
{
	ulong val;

	val = expr(addr);
	p = nextc(p);
	if(*p == '\0') {
		p[0] = fmt;
		p[1] = '\0';
	}
	pfmt(*p, 0, val);
	Bprint(bioout, "\n");
}

void
quesie(char *p)
{
	int c, count, i;
	char tbuf[512];

	c = 0;
	symoff(tbuf, sizeof(tbuf), dot, CTEXT);
	Bprint(bioout, "%s?\t", tbuf);

	while(*p) {
		p = nextc(p);
		if(*p == '"') {
			for(p++; *p && *p != '"'; p++) {
				Bputc(bioout, *p);
				c++;
			}
			if(*p)
				p++;
			continue;
		}
		count = 0;
		while(*p >= '0' && *p <= '9')
			count = count*10 + (*p++ - '0');
		if(count == 0)
			count = 1;
		p = nextc(p);
		if(*p == '\0') {
			p[0] = fmt;
			p[1] = '\0';
		}
		for(i = 0; i < count; i++) {
			c += pfmt(*p, 1, 0);
			dot += inc;
			if(c > width) {
				Bprint(bioout, "\n");
				symoff(tbuf, sizeof(tbuf), dot, CTEXT);
				Bprint(bioout, "%s?\t", tbuf);
				c = 0;
			}
		}
		fmt = *p++;
		p = nextc(p);
	}
	Bprint(bioout, "\n");
}

void
catcher(void *a, char *msg)
{
	USED(a);
	if(strcmp(msg, "interrupt") != 0)
		noted(NDFLT);

	count = 1;
	print("vi\n");
	noted(NCONT);
}

void
setreg(char *addr, char *cp)
{
	int rn;

	dot = expr(addr);
	cp = nextc(cp);
	if(strcmp(cp, "pc") == 0) {
		reg.pc = dot;
		return;
	}
	if(strcmp(cp, "sp") == 0) {
		reg.r[29] = dot;
		return;
	}
	if(strcmp(cp, "mh") == 0) {
		reg.mhi = dot;
		return;
	}
	if(strcmp(cp, "ml") == 0) {
		reg.mlo = dot;
		return;
	}
	if(*cp++ == 'r') {
		rn = strtoul(cp, 0, 10);
		if(rn > 0 && rn < 32) {
			reg.r[rn] = dot;
			return;
		}
	}
	Bprint(bioout, "bad register\n");
}

void
cmd(void)
{
	char *p, *a, *cp, *gotint;
	char addr[128];
	static char *cmdlet = ":$?/=>";
	int n, i;

	notify(catcher);

	dot = reg.pc;
	setjmp(errjmp);

	for(;;) {
		Bflush(bioout);
		p = buf;
		n = 0;
		for(;;) {
			i = Bgetc(bin);
			if(i < 0)
				exits(0);
			*p++ = i;
			n++;
			if(i == '\n')
				break;
		}

		if(buf[0] == '\n')
			strcpy(buf, lastcmd);
		else {
			buf[n-1] = '\0';
			strcpy(lastcmd, buf);
		}
		p = buf;
		a = addr;

		for(;;) {
			p = nextc(p);
			if(*p == 0 || strchr(cmdlet, *p))
				break;
			*a++ = *p++;
		}

		*a = '\0';
		cmdcount = 1;
		cp = strchr(addr, ',');
		if(cp != 0) {
			if(cp[1] == '#')
				cmdcount = strtoul(cp+2, &gotint, 16);
			else
				cmdcount = strtoul(cp+1, &gotint, 0);
			*cp = '\0';
		}

		switch(*p) {
		case '$':
			dollar(p+1);
			break;
		case ':':
			colon(addr, p+1);
			break;
		case '/':
		case '?':
			dot = expr(addr);
			for(i = 0; i < cmdcount; i++)
				quesie(p+1);
			break;
		case '=':
			eval(addr, p+1);
			break;
		case '>':
			setreg(addr, p+1);
			break;
		default:
			Bprint(bioout, "?\n");
			break;
		}
	}
}
