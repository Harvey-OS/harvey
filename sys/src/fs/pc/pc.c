#include	"all.h"
#include	"mem.h"
#include	"io.h"
#include	"ureg.h"

/*
 * Where configuration info is left for the loaded programme.
 * This will turn into a structure as more is done by the boot loader
 * (e.g. why parse the .ini file twice?).
 * There are 1024 bytes available at CONFADDR.
 */
#define BOOTLINE	((char*)CONFADDR)
#define BOOTLINELEN	64
#define BOOTARGS	((char*)(CONFADDR+BOOTLINELEN))
#define	BOOTARGSLEN	(1024-BOOTLINELEN)
#define	MAXCONF		32

#define KADDR(a)	((void*)((ulong)(a)|KZERO))

char bootdisk[NAMELEN];
char *confname[MAXCONF];
char *confval[MAXCONF];
int nconf;

static int isoldbcom;

int
getcfields(char* lp, char** fields, int n, char* sep)
{
	int i;

	for(i = 0; lp && *lp && i < n; i++){
		while(*lp && strchr(sep, *lp) != 0)
			*lp++ = 0;
		if(*lp == 0)
			break;
		fields[i] = lp;
		while(*lp && strchr(sep, *lp) == 0){
			if(*lp == '\\' && *(lp+1) == '\n')
				*lp++ = ' ';
			lp++;
		}
	}

	return i;
}

static void
options(void)
{
	uchar *bda;
	long i, n;
	char *cp, *line[MAXCONF], *p, *q;

	if(strncmp(BOOTARGS, "ZORT 0\r\n", 8)){
		isoldbcom = 1;

		memmove(BOOTARGS, KADDR(1024), BOOTARGSLEN);
		memmove(BOOTLINE, KADDR(0x100), BOOTLINELEN);

		bda = KADDR(0x400);
		bda[0x13] = 639;
		bda[0x14] = 639>>8;
	}

	/*
	 *  parse configuration args from dos file plan9.ini
	 */
	cp = BOOTARGS;	/* where b.com leaves its config */
	cp[BOOTARGSLEN-1] = 0;

	/*
	 * Strip out '\r', change '\t' -> ' '.
	 */
	p = cp;
	for(q = cp; *q; q++){
		if(*q == '\r')
			continue;
		if(*q == '\t')
			*q = ' ';
		*p++ = *q;
	}
	*p = 0;

	n = getcfields(cp, line, MAXCONF, "\n");
	for(i = 0; i < n; i++){
		if(*line[i] == '#')
			continue;
		cp = strchr(line[i], '=');
		if(cp == 0)
			continue;
		*cp++ = 0;
		if(cp - line[i] >= NAMELEN+1)
			*(line[i]+NAMELEN-1) = 0;
		confname[nconf] = line[i];
		confval[nconf] = cp;
		nconf++;
	}
}


/*
 * Vecinit is the first hook we have into configuring the machine,
 * so we do it all here. A pox on special fileserver code.
 * We do more in meminit below.
 */
void
vecinit(void)
{
	options();
}

int
cistrcmp(char* a, char* b)
{
	int ac, bc;

	for(;;){
		ac = *a++;
		bc = *b++;
	
		if(ac >= 'A' && ac <= 'Z')
			ac = 'a' + (ac - 'A');
		if(bc >= 'A' && bc <= 'Z')
			bc = 'a' + (bc - 'A');
		ac -= bc;
		if(ac)
			return ac;
		if(bc == 0)
			break;
	}
	return 0;
}

int
cistrncmp(char *a, char *b, int n)
{
	unsigned ac, bc;

	while(n > 0){
		ac = *a++;
		bc = *b++;
		n--;

		if(ac >= 'A' && ac <= 'Z')
			ac = 'a' + (ac - 'A');
		if(bc >= 'A' && bc <= 'Z')
			bc = 'a' + (bc - 'A');

		ac -= bc;
		if(ac)
			return ac;
		if(bc == 0)
			break;
	}

	return 0;
}

char*
getconf(char *name)
{
	int i;

	for(i = 0; i < nconf; i++)
		if(cistrcmp(confname[i], name) == 0)
			return confval[i];
	return 0;
}

/* memory map */
#define MAXMEG 512
char mmap[MAXMEG+2];
Mconf mconf;

static void
mconfinit(void)
{
	long x, i, j;
	ulong ktop;
	Mbank *mbp;
	uchar *bda;

	/*
	 *  size memory above 1 meg. Kernel sits at 1 meg.  We
	 *  only recognize MB size chunks.
	 */
	memset(mmap, ' ', sizeof(mmap));
	x = 0x12345678;
	for(i = 1; i <= MAXMEG; i++){
		/*
		 *  write the first & last word in a megabyte of memory
		 */
		*mapaddr(KZERO|(i*MB)) = x;
		*mapaddr(KZERO|((i+1)*MB-BY2WD)) = x;

		/*
		 *  write the first and last word in all previous megs to
		 *  handle address wrap around
		 */
		for(j = 1; j < i; j++){
			*mapaddr(KZERO|(j*MB)) = ~x;
			*mapaddr(KZERO|((j+1)*MB-BY2WD)) = ~x;
		}

		/*
		 *  check for correct value
		 */
		if(*mapaddr(KZERO|(i*MB)) == x && *mapaddr(KZERO|((i+1)*MB-BY2WD)) == x)
			mmap[i] = 'x';
		x += 0x3141526;
	}

	/*
	 * bank[0] goes from the end of the bootstrap structures to ~640k.
	 * want to use this up first for ialloc because sparemem
	 * will want a large contiguous chunk.
	 */
	mbp = mconf.bank;
	mbp->base = PADDR(CPU0MACH+BY2PG);
	bda = (uchar*)(KZERO|0x400);
	mbp->limit = ((bda[0x14]<<8)|bda[0x13])*1024;
	mbp++;

	/*
	 *  bank[1] usually goes from the end of kernel bss to the end of memory
	 */
	ktop = PGROUND((ulong)end);
	ktop = PADDR(ktop);
	mbp->base = ktop;
	for(i = 1; mmap[i] == 'x'; i++)
		;
	mbp->limit = i*MB;
	mconf.topofmem = mbp->limit;
	mbp++;

	/*
	 * Look for any other chunks of memory.
	 */
	for(; i <= MAXMEG; i++){
		if(mmap[i] == 'x'){
			mbp->base = i*MB;
			for(j = i+1; mmap[j] == 'x'; j++)
				;
			mbp->limit = j*MB;
			mconf.topofmem = j*MB;
			mbp++;

			if((mbp - mconf.bank) >= MAXBANK)
				break;
		}
	}

	mconf.nbank = mbp - mconf.bank;
}

ulong
meminit(void)
{
	conf.nmach = 1;
	mconfinit();
	mmuinit();
	trapinit();

	/*
	 * This is not really right, but the port code assumes
	 * a linear memory array and this is as close as we can
	 * get to satisfying that.
	 * Dancing around the 'port' code is all just an ugly hack
	 * anyway.
	 */
	return mconf.topofmem;
}

void
userinit(void (*f)(void), void *arg, char *text)
{
	User *p;

	p = newproc();

	/*
	 * Kernel Stack.
	 * The -4 is because the path sched()->gotolabel()->init0()->f()
	 * uses a stack location without creating any local space.
	 */
	p->sched.pc = (ulong)init0;
	p->sched.sp = (ulong)p->stack + sizeof(p->stack) - 4;
	p->start = f;
	p->text = text;
	p->arg = arg;
	dofilter(p->time+0, C0a, C0b, 1);
	dofilter(p->time+1, C1a, C1b, 1);
	dofilter(p->time+2, C2a, C2b, 1);
	ready(p);
}

static int useuart;
static void (*intrputs)(char*, int);

static int
pcgetc(void)
{
	int c;

	if(c = kbdgetc())
		return c;
	if(useuart)
		return uartgetc();
}

static void
pcputc(int c)
{
	if(predawn)
		cgaputc(c);
	if(useuart)
		uartputc(c);
}

static void
pcputs(char* s, int n)
{
	if(!predawn)
		cgaputs(s, n);
	if(intrputs)
		(*intrputs)(s, n);
}

void
consinit(void (*puts)(char*, int))
{
	char *p;
	int baud, port;

	kbdinit();

	consgetc = pcgetc;
	consputc = pcputc;
	consputs = pcputs;
	intrputs = puts;

	if((p = getconf("console")) == 0 || cistrcmp(p, "cga") == 0)
		return;

	port = strtoul(p, 0, 0);
	baud = 0;
	if(p = getconf("baud"))
		baud = strtoul(p, 0, 0);
	if(baud == 0)
		baud = 9600;

	uartspecial(port, kbdchar, conschar, baud);
	useuart = 1;
}

void
consreset(void)
{
}

void
firmware(void)
{
	char *p;

	/*
	 * Always called splhi().
	 */
	if((p = getconf("reset")) && cistrcmp(p, "manual") == 0){
		predawn = 1;
		print("\Hit Reset\n");
		for(;;);
	}
	pcireset();
	i8042reset();
}

int
isaconfig(char *class, int ctlrno, ISAConf *isa)
{
	char cc[NAMELEN], *p, *q, *r;
	int n;

	sprint(cc, "%s%d", class, ctlrno);
	for(n = 0; n < nconf; n++){
		if(cistrncmp(confname[n], cc, NAMELEN))
			continue;
		isa->nopt = 0;
		p = confval[n];
		while(*p){
			while(*p == ' ' || *p == '\t')
				p++;
			if(*p == '\0')
				break;
			if(cistrncmp(p, "type=", 5) == 0){
				p += 5;
				for(q = isa->type; q < &isa->type[NAMELEN-1]; q++){
					if(*p == '\0' || *p == ' ' || *p == '\t')
						break;
					*q = *p++;
				}
				*q = '\0';
			}
			else if(cistrncmp(p, "port=", 5) == 0)
				isa->port = strtoul(p+5, &p, 0);
			else if(cistrncmp(p, "irq=", 4) == 0)
				isa->irq = strtoul(p+4, &p, 0);
			else if(cistrncmp(p, "dma=", 4) == 0)
				isa->dma = strtoul(p+4, &p, 0);
			else if(cistrncmp(p, "mem=", 4) == 0)
				isa->mem = strtoul(p+4, &p, 0);
			else if(cistrncmp(p, "size=", 5) == 0)
				isa->size = strtoul(p+5, &p, 0);
			else if(cistrncmp(p, "freq=", 5) == 0)
				isa->freq = strtoul(p+5, &p, 0);
			else if(isa->nopt < NISAOPT){
				r = isa->opt[isa->nopt];
				while(*p && *p != ' ' && *p != '\t'){
					*r++ = *p++;
					if(r-isa->opt[isa->nopt] >= ISAOPTLEN-1)
						break;
				}
				*r = '\0';
				isa->nopt++;
			}
			while(*p && *p != ' ' && *p != '\t')
				p++;
		}
		return 1;
	}
	return 0;
}

void
lockinit(void)
{
}

void
launchinit(void)
{
}

void
lights(int, int)
{
}

/* in assembly language
Float
famd(Float a, int b, int c, int d)
{
	return ((a+b) * c) / d;
}

ulong
fdf(Float a, int b)
{
	return a / b;
}
*/
