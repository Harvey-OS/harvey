#include <u.h>
#include <libc.h>
#include <bio.h>
#include <mach.h>
#include <ctype.h>
#define Extern extern
#include "sim.h"

enum
{
	Cmdlen = 4096,
};

char	fmt = 'X';
int	width = 60;
int	inc;
ulong	lastaddr;

ulong	expr(char*);

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
getreg(char *addr, ulong *val)
{
	Namedreg *r;
	int rn;

	addr = nextc(addr);
	for(r = namedreg; r->name; r++)
		if(strncmp(r->name, addr, strlen(r->name)) == 0){
			*val = *r->reg;
			return addr + strlen(r->name);
		}
	if(*addr == 'r') {
		addr++;
		rn = strtoul(addr, &addr, 10);
		if(rn > 0 && rn < 32) {
			*val = reg.r[rn];
			return addr;
		}
	}
	error("bad register %s", addr);
	return 0;
}

char *
numsym(char *addr, ulong *val)
{
	char tsym[128], *t;
	static char *delim = "()`'%<>/\\@*|#&-~+-/=?\n";
	Symbol s;
	Rune c;

	addr = nextc(addr);
	t = tsym;
	*t++ = c = *addr++;
	if(c)
		while(c = *addr){
			if(strchr(delim, c))
				break;
			*t++ = c;
			addr++;
		}
	t[0] = '\0';

	if(lookup(0, tsym, &s))
		*val = s.value;
	else if(tsym[0] == '#')
		*val = strtoul(tsym+1, 0, 16);
	else
		*val = strtoul(tsym, 0, 0);
	return addr;
}

char *
unop(char *addr, ulong *val)
{
	char *p;

	addr = nextc(addr);
	switch(*addr){
	case '@':
	case '*':
		addr = unop(addr + 1, val);
		*val = getmem_4(*val);
		return addr;
	case '-':
		addr = unop(addr + 1, val);
		*val = -*val;
		return addr;
	case '~':
		addr = unop(addr + 1, val);
		*val = ~*val;
		return addr;
	case '.':
		*val = dot;
		return addr + 1;
	case '+':
		*val = dot + inc;
		return addr + 1;
	case '^':
		*val = dot - inc;
		return addr + 1;
	case '"':
		*val = lastaddr;
		return addr + 1;
	case '<':
		return getreg(addr+1, val);
	case '(':
		addr++;
		p = strchr(addr, ')');
		if(!p)
			error("expr syntax");
		*val = expr(addr);
		return p + 1;
	}
	return numsym(addr, val);
}

ulong
expr(char *addr)
{
	ulong t, t2;
	char op;

	if(*addr == '\0')
		return dot;

	addr = unop(addr, &t);
	if(*addr == '\0')
		return t;

	addr = nextc(addr);
	op = *addr++;
	unop(addr, &t2);
	switch(op) {
	default:
		error("expr syntax");
	case '+':
		t += t2;
		break;
	case '*':
		t *= t2;
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
	case '#':
		t = (t + t2 - 1) / t2 * t2;
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
colon(char *cp)
{
	int argc;
	char *argv[100], buf[128];

	cp = nextc(cp);
	switch(*cp) {
	default:
		Bprint(bioout, "?\n");
		return;
	case 'b':
		breakpoint(cp+1);
		return;
	case 'd':
		delbpt();
		return;

	/* These fall through to print the stopped address */
	case 'r':
		reset();
		argc = buildargv(cp+1, argv, 100);
		initstk(argc, argv);
		count = ~0;
		atbpt = 0;
		dot = run();
		break;
	case 'c':
		count = ~0;
		atbpt = 0;
		dot = run();
		break;
	case 's':
		cp = nextc(cp+1);
		count = 0;
		if(*cp)
			count = strtoul(cp, 0, 0);
		if(count == 0)
			count = 1;
		atbpt = 0;
		dot = run();
		break;
	}

	inc = 0;
	symoff(buf, sizeof buf, dot, CANY);
	Bprint(bioout, "%s at #%lux %s", atbpt ? "breakpoint" : "stopped", dot, buf);
	if(fmt == 'z')
		printsource(dot);

	Bprint(bioout, "\n");
}

void
dollar(char *cp)
{
	Symbol s;
	long v;
	int i;

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

	case 'e':
		for(i = 0; globalsym(&s, i); i++)
			Bprint(bioout, "%-15s #%lux\n", s.name,	getmem_4(s.value));
		break;

	case 'v':
		for(i = 0; globalsym(&s, i); i++){
			v = getmem_4(s.value);
			if(v)
				Bprint(bioout, "%-15s #%lux\n", s.name,	v);
		}
		break;

	case 't':
		cp++;
		switch(*cp) {
		default:
			Bprint(bioout, "$t[0sic]\n");
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
		}
		break;

	case 'i':
		cp++;
		switch(*cp) {
		default:
			Bprint(bioout, "$i[0ictspa]\n");
			break;
		case 'c':
			cachesum();
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
			cachesum();
			tlbsum();
			segsum();
			iprofile();
			break;
		case 'p':
			iprofile();
			break;
		case '0':
			clearprof();
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
		c = Bprint(bioout, "%-4luo ", mem ? getmem_2(dot) : val);
		inc = 2;
		break;

	case 'O':
		c = Bprint(bioout, "%-8luo ", mem ? getmem_4(dot) : val);
		inc = 4;
		break;

	case 'q':
		c = Bprint(bioout, "%-4lo ", mem ? getmem_2(dot) : val);
		inc = 2;
		break;

	case 'Q':
		c = Bprint(bioout, "%-8lo ", mem ? getmem_4(dot) : val);
		inc = 4;
		break;

	case 'd':
		c = Bprint(bioout, "%-5ld ", mem ? getmem_2(dot) : val);
		inc = 2;
		break;

	case 'D':
		c = Bprint(bioout, "%-8ld ", mem ? getmem_4(dot) : val);
		inc = 4;
		break;

	case 'x':
		c = Bprint(bioout, "#%-4lux ", mem ? getmem_2(dot) : val);
		inc = 4;
		break;

	case 'X':
		c = Bprint(bioout, "#%-8lux ", mem ? getmem_4(dot) : val);
		inc = 4;
		break;

	case 'u':
		c = Bprint(bioout, "%-5lud ", mem ? getmem_2(dot) : val);
		inc = 2;
		break;

	case 'U':
		c = Bprint(bioout, "%-8lud ", mem ? getmem_4(dot) : val);
		inc = 4;
		break;

	case 'f':
		inc = ffmt(str, sizeof(str), mem, dot, val);
		c = Bprint(bioout, "%s", str);
		break;

	case 'F':
		inc = Ffmt(str, sizeof(str), mem, dot, val);
		c = Bprint(bioout, "%s", str);
		break;

	case 'b':
		c = Bprint(bioout, "#%-2lx ", mem ? getmem_1(dot) : val);
		inc = 1;
		break;

	case 'c':
		c = Bprint(bioout, "%c ", (int)(mem ? getmem_1(dot) : val));
		inc = 1;
		break;

	case 'C':
		ch = mem ? getmem_1(dot) : val;
		if(isprint(ch))
			c = Bprint(bioout, "%c ", ch);
		else
			c = Bprint(bioout, "\\x%.2x ", ch);
		inc = 1;
		break;

	case 's':
		i = 0;
		while(ch = getmem_1(dot+i))
			str[i++] = ch;
		str[i] = '\0';
		dot += i;
		c = Bprint(bioout, "%s", str);
		inc = 0;
		break;

	case 'S':
		i = 0;
		while(ch = getmem_1(dot+i))
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
		p = ctime(mem ? getmem_1(dot) : val);
		p[30] = '\0';
		c = Bprint(bioout, "%s", p);
		inc = 4;
		break;

	case 'a':
		symoff(str, sizeof str, dot, CANY);
		c = Bprint(bioout, "%s/\t", str);
		inc = 0;
		break;

	case 'i':
	case 'I':
		inc = fmtins(str, sizeof str, mem, dot, val);
		Bprint(bioout, "%s", str);
		c = width+1;
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
		dot -= 2 * inc;
		break;

	case 'z':
		if(findsym(dot, CTEXT, &s))
			Bprint(bioout, "  %s() ", s.name);
		printsource(dot);
		Bprint(bioout, " ");
		inc = 0;
		c = 10;
		break;
	}
	return c;
}

void
eval(char *p)
{
	p = nextc(p);
	if(*p == '\0') {
		p[0] = fmt;
		p[1] = '\0';
	}
	pfmt(*p, 0, dot);
	inc = 0;
	Bprint(bioout, "\n");
}

void
writemem(char *p)
{
	ulong val;
	int fmt;

	fmt = 'x';
	if(*p == 'W')
		fmt = 'X';
	val = expr(p + 1);
	pfmt(fmt, 1, 0);
	if(fmt == 'x')
		putmem_2(dot, val);
	else
		putmem_4(dot, val);
	Bprint(bioout, "\t");
	pfmt(fmt, 1, 0);
}

void
quesie(char *p)
{
	int c, count, i;

	p = nextc(p);
	switch(*p){
	case 'w':
	case 'W':
		writemem(p);
		Bprint(bioout, "\n");
		return;
	case 'l':
	case 'L':
	case 'm':
		Bprint(bioout, "%c command not implemented\n", *p);
		return;
	}
	c = 0;
	while(*p) {
		p = nextc(p);
		if(*p == '"') {
			if(c >= width) {
				Bprint(bioout, "\n\t");
				c = 0;
			}
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
			if(c >= width) {
				Bprint(bioout, "\n\t");
				c = 0;
			}
			c += pfmt(*p, 1, 0);
			dot += inc;
		}
		fmt = *p++;
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
	Bflush(bioout);
	print("%s\n", argv0);
	noted(NCONT);
}

void
setreg(char *addr)
{
	Namedreg *r;
	int rn;

	inc = 0;
	addr = nextc(addr);
	for(r = namedreg; r->name; r++)
		if(strncmp(r->name, addr, strlen(r->name)) == 0){
			*r->reg = dot;
			return;
		}
	if(*addr == 'r') {
		addr++;
		rn = strtoul(addr, 0, 10);
		if(rn > 0 && rn < 32) {
			reg.r[rn] = dot;
			return;
		}
	}
	error("bad register %s", addr);
}

void
cmd(void)
{
	char *p, *a, *cp;
	char addr[Cmdlen], lastcmd[Cmdlen], buf[128];
	static char *cmdlet = ":$?/=>";
	int i, c;

	notify(catcher);
	lastaddr = 0;
	dot = reg.pc;
	setjmp(errjmp);

	for(;;) {
		Bflush(bioout);
		p = Brdline(bin, '\n');
		if(!p)
			exits(0);
		a = addr;

		if(p[0] == '\n'){
			dot += inc;
			p = lastcmd;
			cmdcount = 1;
		}else{
			p[Blinelen(bin)-1] = '\0';
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
				*cp++ = '\0';
				cmdcount = expr(cp);
			}
			if(*addr)
				lastaddr = dot = expr(addr);
			strcpy(lastcmd, p);
		}

		switch(c = *p++) {
		case '$':
			dollar(p);
			break;
		case ':':
			colon(p);
			break;
		case '?':
		case '/':
			symoff(buf, sizeof buf, dot, CANY);
			Bprint(bioout, "%s%c\t", buf, c);
			for(i = 0; i < cmdcount; i++)
				quesie(p);
			dot -= inc;
			break;
		case '=':
			eval(p);
			break;
		case '>':
			setreg(p);
			break;
		default:
			Bprint(bioout, "?\n");
			break;
		}
	}
}
