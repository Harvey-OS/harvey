#include <u.h>
#include <libc.h>
#include <bio.h>
#include <mach.h>
#include <ctype.h>

static	int	rtrace(ulong, ulong, ulong);
static	int	ctrace(ulong, ulong, ulong);
static	int	i386trace(ulong, ulong, ulong);
static	ulong	getval(ulong);
static	void	inithdr(int);
static	void	fatal(char*, ...);
static	void	readstack(void);

static	Fhdr	fhdr;
static	int	interactive;

#define	FRAMENAME	".frame"

static void
usage(void)
{
	fprint(2, "usage: ktrace [-i] kernel pc sp [link]\n");
	exits("usage");
}

static void
printaddr(char *addr, ulong pc)
{
	int i;
	char *p;

	/*
	 * reformat the following.
	 *
	 * foo+1a1 -> src(foo+0x1a1);
	 * 10101010 -> src(0x10101010);
	 */

	if(strlen(addr) == 8 && strchr(addr, '+') == nil){
		for(i=0; i<8; i++)
			if(!isxdigit(addr[i]))
				break;
		if(i == 8){
			print("src(0x%.8lux); // 0x%s\n", pc, addr);
			return;
		}
	}

	if(p=strchr(addr, '+')){
		*p++ = 0;
		print("src(0x%.8lux); // %s+0x%s\n", pc, addr, p);
	}else
		print("src(0x%.8lux); // %s\n", pc, addr);
}

static void (*fmt)(char*, ulong) = printaddr;

void
main(int argc, char *argv[])
{
	int (*t)(ulong, ulong, ulong);
	ulong pc, sp, link;
	int fd;

	ARGBEGIN{
	case 'i':
		interactive++;
		break;
	default:
		usage();
	}ARGEND

	link = 0;
	t = ctrace;
	switch(argc){
	case 4:
		t = rtrace;
		link = strtoul(argv[3], 0, 16);
		break;
	case 3:
		break;
	default:
		usage();
	}
	pc = strtoul(argv[1], 0, 16);
	sp = strtoul(argv[2], 0, 16);
	if(!interactive)
		readstack();

	fd = open(argv[0], OREAD);
	if(fd < 0)
		fatal("can't open %s: %r", argv[0]);
	inithdr(fd);
	switch(fhdr.magic){
	case I_MAGIC:	/* intel 386 */
		t = i386trace;
		break;
	case A_MAGIC:	/* 68020 */
	case J_MAGIC:	/* intel 960 */
		t = ctrace;
		break;
	case K_MAGIC:	/* sparc */
	case D_MAGIC:	/* amd 29000 */
	case V_MAGIC:	/* mips 3000 */
	case M_MAGIC:	/* mips 4000 */
	case E_MAGIC:	/* arm 7-something */
	case Q_MAGIC:	/* powerpc */
	case N_MAGIC:	/* mips 4000 LE */
	case L_MAGIC:	/* dec alpha */
		t = rtrace;
		break;
	case X_MAGIC:	/* att dsp 3210 */
		sysfatal("can't ktrace %s\n", argv[0]);
		break;
	default:
		fprint(2, "%s: warning: can't tell what type of stack %s uses; assuming it's %s\n",
			argv0, argv[0], argc == 4 ? "risc" : "cisc");
		break;
	}
	(*t)(pc, sp, link);
	exits(0);
}

static void
inithdr(int fd)
{
	seek(fd, 0, 0);
	if(!crackhdr(fd, &fhdr))
		fatal("read text header");

	if(syminit(fd, &fhdr) < 0)
		fatal("%r\n");
}

static int
rtrace(ulong pc, ulong sp, ulong link)
{
	Symbol s, f;
	char buf[128];
	ulong oldpc;
	int i;

	i = 0;
	while(findsym(pc, CTEXT, &s)) {
		if(pc == s.value)	/* at first instruction */
			f.value = 0;
		else if(findlocal(&s, FRAMENAME, &f) == 0)
			break;

		symoff(buf, sizeof buf, pc, CANY);
		fmt(buf, pc);

		oldpc = pc;
		if(s.type == 'L' || s.type == 'l' || pc <= s.value+mach->pcquant){
			if(link == 0)
				fprint(2, "%s: need to supply a valid link register\n", argv0);
			pc = link;
		}else{
			pc = getval(sp);
			if(pc == 0)
				break;
		}

		if(pc == 0 || (pc == oldpc && f.value == 0))
			break;

		sp += f.value;

		if(++i > 40)
			break;
	}
	return i;
}

static int
ctrace(ulong pc, ulong sp, ulong link)
{
	Symbol s;
	char buf[128];
	int found;
	ulong opc;
	long moved, j;

	USED(link);
	j = 0;
	opc = 0;
	while(pc && opc != pc) {
		moved = pc2sp(pc);
		if (moved == -1){
			print("pc2sp(%.8lux) = -1 %r\n", pc);
			break;
		}
		found = findsym(pc, CTEXT, &s);
		if (!found){
			print("findsym fails\n");
			break;
		}
		symoff(buf, sizeof buf, pc, CANY);
		fmt(buf, pc);

		sp += moved;
		opc = pc;
		pc = getval(sp);
		if(pc == 0)
			break;
		sp += mach->szaddr;	/*assumes address size = stack width*/
		if(++j > 40)
			break;
	}
	return j;
}

static int
i386trace(ulong pc, ulong sp, ulong link)
{
	int i;
	ulong osp;
	Symbol s, f;
	char buf[128];

	USED(link);
	i = 0;
	osp = 0;
	while(findsym(pc, CTEXT, &s)) {

		symoff(buf, sizeof buf, pc, CANY);
		fmt(buf, pc);

		if(pc != s.value) {	/* not at first instruction */
			if(findlocal(&s, FRAMENAME, &f) == 0)
				break;
			sp += f.value-mach->szaddr;
		}else if(strcmp(s.name, "forkret") == 0){
			print("//passing interrupt frame; last pc found at sp=%lux\n", osp);
			sp +=  15 * mach->szaddr;		/* pop interrupt frame */
		}

		pc = getval(sp);
		if(pc == 0 && strcmp(s.name, "forkret") == 0){
			sp += 3 * mach->szaddr;			/* pop iret eip, cs, eflags */
			print("//guessing call through invalid pointer, try again at sp=%lux\n", sp);
			s.name = "";
			pc = getval(sp);
		}
		if(pc == 0) {
			print("//didn't find pc at sp=%lux, last pc found at sp=%lux\n", sp, osp);
			break;
		}
		osp = sp;

		sp += mach->szaddr;
		if(strcmp(s.name, "forkret") == 0)
			sp += 2 * mach->szaddr;			/* pop iret cs, eflags */

		if(++i > 40)
			break;
	}
	return i;
}

int naddr;
ulong addr[1024];
ulong val[1024];

static void
putval(ulong a, ulong v)
{
	if(naddr < nelem(addr)){
		addr[naddr] = a;
		val[naddr] = v;
		naddr++;
	}
}

static void
readstack(void)
{
	Biobuf b;
	char *p;
	char *f[64];
	int nf, i;

	Binit(&b, 0, OREAD);
	while(p=Brdline(&b, '\n')){
		p[Blinelen(&b)-1] = 0;
		nf = tokenize(p, f, nelem(f));
		for(i=0; i<nf; i++){
			if(p=strchr(f[i], '=')){
				*p++ = 0;
				putval(strtoul(f[i], 0, 16), strtoul(p, 0, 16));
			}
		}
	}
}

static ulong
getval(ulong a)
{
	char buf[256];
	int i, n;

	if(interactive){
		print("// data at 0x%8.8lux? ", a);
		n = read(0, buf, sizeof(buf)-1);
		if(n <= 0)
			return 0;
		buf[n] = '\0';
		return strtoul(buf, 0, 16);
	}else{
		for(i=0; i<naddr; i++)
			if(addr[i] == a)
				return val[i];
		return 0;
	}
}

static void
fatal(char *fmt, ...)
{
	char buf[4096];
	va_list arg;

	va_start(arg, fmt);
	vseprint(buf, buf+sizeof(buf), fmt, arg);
	va_end(arg);
	fprint(2, "ktrace: %s\n", buf);
	exits(buf);
}
