#include	"all.h"
#include	"mem.h"
#include	"io.h"
#include	"ureg.h"

/*
 * Vecinit is the first hook we have into configuring the machine,
 * so we do it all here. A pox on special fileserver code.
 * We do more in meminit below.
 */
void
vecinit(void)
{
	i8042a20();		/* enable address lines 20 and up */
	cgainit();
}

/* where b.com leaves configuration info */
#define BOOTARGS	((char*)(KZERO|1024))
#define	BOOTARGSLEN	1024
#define	MAXCONF		32

char bootdisk[NAMELEN];
char *confname[MAXCONF];
char *confval[MAXCONF];
int nconf;

char*
getconf(char *name)
{
	int i;

	for(i = 0; i < nconf; i++)
		if(strcmp(confname[i], name) == 0)
			return confval[i];
	return 0;
}

int
getfields(char *lp, char **fields, int n, char sep)
{
	int i;

	for(i=0; lp && *lp && i<n; i++){
		while(*lp == sep)
			*lp++=0;
		if(*lp == 0)
			break;
		fields[i]=lp;
		while(*lp && *lp != sep)
			lp++;
	}
	return i;
}

/* memory map */
#define MAXMEG 256
char mmap[MAXMEG+2];
Mconf mconf;

static void
mconfinit(void)
{
	long x, i, j, n;
	ulong ktop;
	char *cp;
	char *line[MAXCONF];
	Mbank *mbp;

	/*
	 *  parse configuration args from dos file p9rc
	 */
	cp = BOOTARGS;	/* where b.com leaves its config */
	cp[BOOTARGSLEN-1] = 0;
	n = getfields(cp, line, MAXCONF, '\n');
	for(j = 0; j < n; j++){
		cp = strchr(line[j], '\r');
		if(cp)
			*cp = 0;
		cp = strchr(line[j], '=');
		if(cp == 0)
			continue;
		*cp++ = 0;
		if(cp - line[j] >= NAMELEN+1)
			*(line[j]+NAMELEN-1) = 0;
		confname[nconf] = line[j];
		confval[nconf] = cp;
		nconf++;
	}
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
	 * bank[0] goes from the end of BOOTARGS to 640k.
	 * want to use this up first for ialloc because sparemem
	 * will want a large contiguous chunk.
	 */
	mbp = mconf.bank;
	mbp->base = (ulong)(BOOTARGS+BOOTARGSLEN);
	mbp->base = PGROUND(mbp->base);
	mbp->base = PADDR(mbp->base);
	mbp->limit = 640*1024;
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
	dofilter(&p->time);
	ready(p);
}

void
consinit(void (*puts)(char*, int))
{
	char *p;
	int baud, port;

	if((p = getconf("console")) == 0 || strcmp(p, "cga") == 0){
		cgainit();
		kbdinit();
		return;
	}

	consgetc = uartgetc;
	consputc = uartputc;
	consputs = puts;
	port = strtoul(p, 0, 0);

	baud = 0;
	if(p = getconf("baud"))
		baud = strtoul(p, 0, 0);
	if(baud == 0)
		baud = 9600;

	uartspecial(port, kbdchar, conschar, baud);
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
	if((p = getconf("reset")) == 0 || strcmp(p, "manual") == 0){
		predawn = 1;
		print("\Hit Reset\n");
		for(;;);
	}
	i8042reset();
}

static int
parseether(uchar *to, char *from)
{
	char nip[4];
	char *p;
	int i;

	p = from;
	while(*p == ' ')
		++p;
	for(i = 0; i < 6; i++){
		if(*p == 0)
			return -1;
		nip[0] = *p++;
		if(*p == 0)
			return -1;
		nip[1] = *p++;
		nip[2] = 0;
		to[i] = strtoul(nip, 0, 16);
		if(*p == ':')
			p++;
	}
	return 0;
}

int
isaconfig(char *class, int ctlrno, ISAConf *isa)
{
	char cc[NAMELEN], *p, *q;
	int n;

	sprint(cc, "%s%d", class, ctlrno);
	for(n = 0; n < nconf; n++){
		if(strncmp(confname[n], cc, NAMELEN))
			continue;
		p = confval[n];
		while(*p){
			while(*p == ' ' || *p == '\t')
				p++;
			if(*p == '\0')
				break;
			if(strncmp(p, "type=", 5) == 0){
				p += 5;
				for(q = isa->type; q < &isa->type[NAMELEN-1]; q++){
					if(*p == '\0' || *p == ' ' || *p == '\t')
						break;
					*q = *p++;
				}
				*q = '\0';
			}
			else if(strncmp(p, "port=", 5) == 0)
				isa->port = strtoul(p+5, &p, 0);
			else if(strncmp(p, "irq=", 4) == 0)
				isa->irq = strtoul(p+4, &p, 0);
			else if(strncmp(p, "mem=", 4) == 0)
				isa->mem = strtoul(p+4, &p, 0);
			else if(strncmp(p, "size=", 5) == 0)
				isa->size = strtoul(p+5, &p, 0);
			else if(strncmp(p, "ea=", 3) == 0){
				if(parseether(isa->ea, p+3) == -1)
					memset(isa->ea, 0, 6);
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
lights(int n, int on)
{
	USED(n, on);
}

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
