#include <u.h>
#include <libc.h>
#include <bio.h>
#include <mach.h>
#include "../boot/boot.h"

#include "../hobbit/rom.h"

char sys[2*NAMELEN];

enum {
	Pzero		= 0x80000000,	/* physical memory base */
	IOzero		= 0xB0000000,	/* I/O base */
	Kzero		= 0xC0400000,	/* offset into #B/mem of Pzero */

	Pgsize		= 4096,		/*  */

	MaxRW		= 8192,		/* max read/write size */
};
#define KADDR(p)	(((p)-Pzero)+Kzero)

static int envfd = -1;

static ulong dot = Pzero;
static ulong dotincr = 4;

static int catching;
jmp_buf catchbuf;

static char rwbuf[MaxRW];

static Entry permenventry[] = {
	{ "sysname",	"hobbit", },
	{ "station",	"99", },
	{ "bootfile",	"/hobbit/9cpu", },
	{ "bootmode",	"d", },
	{ "fs",		"incon!nj/astro/Nfs", },
	0
};

static PPage0 *ppage0 = (PPage0*)Pzero;

static int
envwrite(void)
{
	if(seek(envfd, 0, 0) == -1 || write(envfd, &ppage0->env, sizeof(Env)) == -1){
		close(envfd);
		envfd = -1;
		return -1;
	}
	return 0;
}

static char *
envget(char *name)
{
	int n, nentry;

	nentry = (ppage0->env.nentry < Nenv) ? ppage0->env.nentry: Nenv;
	for(n = 0; n < nentry; n++){
		if(strcmp(ppage0->env.entry[n].name, name) == 0)
			return ppage0->env.entry[n].value;
	}
	return 0;
}

static int
envput(char *name, char *value)
{
	char *p;

	if(p = envget(name))
		strncpy(p, value, Nvaluelen);
	else if(ppage0->env.nentry < Nenv){
		strncpy(ppage0->env.entry[ppage0->env.nentry].name, name, Nnamelen);
		strncpy(ppage0->env.entry[ppage0->env.nentry].value, value, Nvaluelen);
		ppage0->env.nentry++;
	}
	else
		return -1;
	return envwrite();
}

static int
envinit(void)
{
	int n;

	if(envfd == -1 && (envfd = open("#r/nvram", ORDWR)) == -1){
		warning("can't open #r/nvram");
		return -1;
	}
	if(seek(envfd, 0, 0) == -1 || read(envfd, &ppage0->env, sizeof(Env)) == -1){
		close(envfd);
		envfd = -1;
		return -1;
	}
	for(n = 0; permenventry[n].name[0]; n++){
		if(envget(permenventry[n].name) == 0)
			envput(permenventry[n].name, permenventry[n].value);
	}
	return 0;
}

static int
envdel(char *name)
{
	int n, m;

	for(n = 0; n < ppage0->env.nentry; n++){
		if(strcmp(ppage0->env.entry[n].name, name))
			continue;
		for(m = 0; permenventry[m].name[0]; m++){
			if(strcmp(permenventry[m].name, name) == 0)
				return -1;
		}
		for(ppage0->env.nentry--; n < ppage0->env.nentry; n++)
			ppage0->env.entry[n] = ppage0->env.entry[n+1];
		memset(&ppage0->env.entry[n], 0, sizeof(Entry));
		return envwrite();
	}
	return -1;
}

static int
envcmd(int argc, char *argv[])
{
	char dflag = 0;
	char *p;
	int n;

	ARGBEGIN{

	case 'd':
		dflag = 1;
		break;

	case 'i':
		memset(&ppage0->env, 0, sizeof(Env));
		envwrite();
		envinit();
		break;

	default:
		return -1;
	}ARGEND

	if(dflag && argc != 1)
		return -1;

	switch(argc){

	case 0:
		for(n = 0; n < ppage0->env.nentry; n++)
			print("%s=%s\n",
				ppage0->env.entry[n].name, ppage0->env.entry[n].value);
		return 0;

	case 1:
		if((p = envget(argv[0])) == 0)
			break;
		if(dflag == 0)
			print("%s=%s\n", argv[0], p);
		else
			return envdel(argv[0]);
		return 0;

	case 2:
		if(strlen(argv[0]) > Nnamelen || strlen(argv[1]) > Nvaluelen)
			return -1;
		return envput(argv[0], argv[1]);
	}
	return -1;
}

static void
interrupt(void *a, char *s)
{
	USED(a);
	if(strcmp(s, "alarm") == 0)
		noted(NCONT);
	noted(NDFLT);
}

static void
catcher(void *a, char *s)
{
	if(catching == 0)
		noted(NDFLT);
	if(strncmp(s, "sys: trap: read fault", 21) == 0
	  || strncmp(s, "sys: trap: write fault", 21) == 0)
		notejmp(a, catchbuf, 1);
	noted(NDFLT);
}

static int
probe(ulong addr, void *data, ulong len)
{
	catching = 1;
	if(setjmp(catchbuf)){
		catching = 0;
		print("#%8.8ux\t?\n", addr);
		return -1;
	}
	memcpy(data, (void*)addr, len);
	catching = 0;
	return 0;
}

static int
pwobe(ulong addr, void *data, ulong len)
{
	catching = 1;
	if(setjmp(catchbuf)){
		catching = 0;
		print("#%8.8ux\t?\n", addr);
		return -1;
	}
	memcpy((void*)addr, data, len);
	catching = 0;
	return 0;
}

static ulong
atoul(char *a)
{
	while(*a == ' ' || *a == '\t')
		a++;
	if(*a == '#')
		return strtoul(a+1, 0, 16);
	return strtoul(a, 0, 0);
}

static int
display(ulong addr)
{
	ulong word;
	ushort hword;
	uchar byte;

	if(dotincr == 4){
		if(probe(addr, &word, sizeof(word)) == -1)
			return -1;
		print("#%8.8ux\t#%8.8ux\n", addr, word);
	}
	else if(dotincr == 2){
		if(probe(addr, &hword, sizeof(hword)) == -1)
			return -1;
		print("#%8.8ux\t#%4.4ux\n", addr, hword);
	}
	else {
		if(probe(addr, &byte, sizeof(byte)) == -1)
			return -1;
		print("#%8.8ux\t#%2.2ux\n", addr, byte);
	}
	return 0;
}

static ulong
setdot(ulong addr)
{
	dot = addr;
	switch(dot & 0x03){

	case 0:
		dotincr = 4;
		break;

	case 2:
		dotincr = 2;
		break;

	case 1:
	case 3:
		dotincr = 1;
		break;
	}
	return dot;
}

static int
getcmd(int argc, char *argv[])
{
	char type;
	int count;

	type = 0;
	count = 1;

	ARGBEGIN{

	case 'b':
		type = 'b';
		break;

	case 'h':
		type = 'h';
		break;

	case 'w':
		type = 'w';
		break;

	default:
		return -1;
	}ARGEND
	if(argc){
		setdot(atoul(*argv));
		argc--; argv++;
	}
	if(argc)
		count = atoul(*argv);

	if(count < 0)
		return -1;

	if(type == 'w'){
		if(dotincr != 4 && (dot & 0x03))
			return -1;
		dotincr = 4;
	}
	else if(type == 'h'){
		if(dotincr != 2 && (dot & 0x01))
			return -1;
		dotincr = 2;
	}
	else if(type == 'b')
		dotincr = 1;
	if(count){
		while(display(dot) == 0 && --count)
			dot += dotincr;
	}
	return 0;
}

static int
minuscmd(int argc, char *argv[])
{
	USED(argc); USED(argv);
	dot -= dotincr;
	display(dot);
	return 0;
}

static int
putcmd(int argc, char *argv[])
{
	char type;
	ulong word;
	ushort hword;
	uchar byte;
	void *data;
	int count;

	if(dotincr == 1)
		type = 'b';
	else if(dotincr == 2)
		type = 'h';
	else
		type = 'w';
	count = 1;
	word = 0;

	ARGBEGIN{

	case 'b':
		type = 'b';
		break;

	case 'h':
		type = 'h';
		break;

	case 'w':
		type = 'w';
		break;

	default:
		return -1;
	}ARGEND
	if(argc){
		setdot(atoul(*argv));
		argc--; argv++;
	}
	if(argc){
		word = atoul(*argv);
		argc--; argv++;
	}
	if(argc)
		count = atoul(*argv);

	if(count < 0)
		return -1;

	if(type == 'w'){
		if(dotincr != 4 && (dot & 0x03))
			return -1;
		dotincr = 4;
		data = &word;
	}
	else if(type == 'h'){
		if(dotincr != 2 && (dot & 0x01))
			return -1;
		dotincr = 2;
		hword = word;
		data = &hword;
	}
	else {
		dotincr = 1;
		byte = word;
		data = &byte;
	}
	while(count--){
		if(pwobe(dot, data, dotincr) == -1)
			return -1;
		dot += dotincr;
	}
	return 0;
}

static Method*
rootserver(char *fs)
{
	Method *mp;
	int l;

	for(mp = method; mp->name; mp++){
		l = strlen(mp->name);
		if(strncmp(fs, mp->name, l) == 0){
			if(fs[l] == '!')
				strcpy(sys, fs+l+1);
			return mp;
		}
	}
	return 0;
}

static int
readseg(int in, long inoff, long outoff, int len)
{
	int n;
	char *p;

	if(seek(in, inoff, 0) < 0){
		warning("seeking bootfile");
		return -1;
	}
	for(p = (char*)outoff; len > 0; len -= n){
		if((n = read(in, p, MaxRW)) <= 0){
			warning("reading bootfile");
			return -1;
		}
		p += n;
	}
	return 0;
}

Method *
attach(char *fs)
{
	static Method *mp;
	int fd;

	if(mp)
		return mp;
	if((mp = rootserver(fs)) == 0){
		print("unknown boot device - %s\n", fs);
		return 0;
	}
	(*mp->config)(mp);
	if((fd = (*mp->connect)()) == -1)
		fatal("can't connect to file server");
	if(strcmp(mp->name, "local")){
		nop(fd);
		session(fd);
	}

	if(bind("/", "/", MREPL) < 0)
		fatal("bind");
	if(mount(fd, "/", MAFTER|MCREATE, "", "") < 0)
		fatal("mount");
	close(fd);
	return mp;
}

static int
bootcmd(int argc, char *argv[])
{
	char *bootfile, *fs;
	int fd, n;
	Fhdr f;

	fs = envget("fs");
	ARGBEGIN{

	case 's':
		fs = ARGF();
		break;

	default:
		return -1;
	}ARGEND

	if(argc && argv[0][0] != '-'){
		bootfile = argv[0];
		argc--; argv++;
	}
	else
		bootfile = envget("bootfile");

	if(bootfile[0] != '#' && attach(fs) == 0)
		return -1;

	print("%s...", bootfile);
	if((fd = open(bootfile, OREAD)) == -1)
		fatal("couldn't open bootfile");

	if(crackhdr(fd, &f) == 0)
		fatal("not a header I understand");

	print("\n%d", f.txtsz);
	if(readseg(fd, f.txtoff, f.txtaddr, f.txtsz) == -1)
		fatal("can't read boot file");
	print("+%d", f.datsz);
	if(readseg(fd, f.datoff, f.dataddr, f.datsz) == -1)
		fatal("can't read boot file");
	print("+%d", f.bsssz);

	close(fd);

	/*
	 * fix up argc, argv as seen by the loaded programme.
	 * we need to make sure argc is at least one, although
	 * we don't really care what the 0'th argument is.
	 */
	ppage0->argc = argc+1;
	for(n = 1; argc; argc--, n++)
		ppage0->argv[n] = *argv++;
	ppage0->argv[n] = 0;

	/*
	 * clear the vectors for the loaded programme, so
	 * it has a chance to get back to the ROM if it
	 * takes an exception before setting its own.
	 */
	memset(ppage0->vectors, 0, sizeof(ppage0->vectors));

	if((fd = open("#B/boot", OWRITE)) == -1)
		fatal("can't open #B/boot");
	print(" entry: 0x%ux\n", f.entry);
	sleep(500);
	if(write(fd, &f.entry, sizeof f.entry) != sizeof f.entry)
		fatal("can't start kernel");
	return 0;
}

static int
registers(int argc, char *argv[])
{
	USED(argc, argv);
	print("%8.8s %.8ux", "MSP", ppage0->msp);
	print("%8.8s %.8ux", "ISP", ppage0->isp);
	print("%8.8s %.8ux\n", "SP", ppage0->sp);
	print("%8.8s %.8ux", "CONFIG", ppage0->config);
	print("%8.8s %.8ux", "PSW", ppage0->psw);
	print("%8.8s %.8ux\n", "SHAD", ppage0->shad);
	print("%8.8s %.8ux", "VB", ppage0->vb);
	print("%8.8s %.8ux", "STB", ppage0->stb);
	print("%8.8s %.8ux\n", "FAULT", ppage0->fault);
	print("%8.8s %.8ux", "ID", ppage0->id);
	print("%8.8s %.8ux", "r0", ppage0->r0);
	print("%8.8s %.8ux\n", "r16", ppage0->r16);
	return 0;
}

/*
 * mmu table entries
 */
#define STE_V		0x00000001	/* valid */
#define STE_S		0x00000008	/* segment (1 == non-paged segment) */

#define PTE_V		0x00000001	/* valid */

#define SEGSIZE		0x400000
#define SEGSHIFT	22
#define SEGNUM(va)	(((va)>>SEGSHIFT) & 0x03FF)
#define SEGOFFSET	0x3FFFFF
#define PGSHIFT		12
#define PGNUM(va)	(((va)>>PGSHIFT) & 0x03FF)
#define PGOFFSET	0x0FFF

static int
walkcmd(int argc, char *argv[])
{
	ulong va, addr, *table;

	ARGBEGIN{
	default:
		return -1;
	}ARGEND
	if(argc == 0)
		return -1;

	va = atoul(*argv);
	table = (ulong*)(ppage0->stb & ~PGOFFSET);
	addr = table[SEGNUM(va)];
	print("#%lux -> %lux -> ", &table[SEGNUM(va)], addr);

	if((addr & STE_V) == 0){
		print("invalid\n");
		return 0;
	}

	if(addr & STE_S){
		va &= SEGOFFSET;
		if(va >= (((addr & SEGOFFSET) & ~PGOFFSET) + Pgsize)){
			print("out of bounds\n");
			return 0;
		}
		print("#%lux\n", (addr & ~SEGOFFSET) + va);
		return 0;
	}

	table = (ulong*)(addr & ~PGOFFSET);
	addr = table[PGNUM(va)];
	print("#%lux -> #%lux ->", &table[PGNUM(va)], addr);

	if((addr & PTE_V) == 0){
		print("invalid\n");
		return 0;
	}

	va &= PGOFFSET;
	print("#%lux\n", (addr & ~PGOFFSET) + va);
	return 0;
}

static int
execcmd(int argc, char *argv[])
{
	char *fs, buf[ERRLEN];

	ARGBEGIN{
	default:
		return -1;
	}ARGEND
	if(argc == 0)
		return -1;
	fs = envget("fs");
	if(attach(fs) == 0)
		return -1;
	exec(argv[0], argv);
	errstr(buf);
	print("can't exec %s: %s\n", argv[0], buf);
	return -1;
}

static int
response(char *buf, char *fields[])
{
	char *p;
	int argc, n;

	do{
		if((n = read(0, buf, Ncmdlen)) == -1)
			fatal("can't read #c/cons");
	}while(n == 0);
	buf[n-1] = 0;

	argc = 0;
	if(p = strtok(buf, " \t")){
		fields[argc++] = p;
		while(p = strtok(0, " \t")){
			if(argc >= Nargc-1)
				return -1;
			fields[argc++] = p;
		}
	}
	fields[argc] = 0;
	return argc;
}

typedef struct {
	char	*name;
	int	(*f)(int, char *[]);
} Command;

static Command command[] = {
	{ "e",	envcmd, },
	{ "g",	getcmd, },
	{ "-",	minuscmd, },
	{ "p",	putcmd, },
	{ "b",	bootcmd, },
	{ "r",	registers, },
	{ "w",	walkcmd, },
	{ "x",	execcmd, },
	0
};

void
romboot(int ac, char *av[])
{
	int fd;
	Dir dir;
	char buf[ERRLEN];
	Command *cmd;
	char *p;

	USED(ac); USED(av);

	open("#c/cons", OREAD);
	open("#c/cons", OWRITE);
	open("#c/cons", OWRITE);

	if(dirstat("#c/time", &dir) == -1)
		dir.mtime = 0;
	print("Hobbit ROM %s", ctime(dir.mtime));

	segdetach((void*)Pzero);
	if(segattach(0, "physm", (void*)Pzero, 0x00400000) == -1){
		errstr(buf);
		print("segattach - %s\n", buf);
		exits("physm");
	}

	/*
	 * must do envinit before setting user to "none",
	 * otherwise won't be allowed to open '#r/nvram'.
	 */
	envinit();

	fd = open("#c/user", OWRITE|OTRUNC);
	if(fd == -1 || write(fd, "none", 4) == -1)
		fatal("can't init #c/user");
	close(fd);

	/*
	 * Check for autoboot. If it's set, fake up
	 * a command line and call bootcmd().
	 * Give the user 5 seconds to abort it.
	 */
	if((p = envget("bootmode")) && *p == 'a'){
		print("autoboot: hit any key (except ^P) within 5 seconds to abort\n");
		notify(interrupt);
		alarm(5000);
		if(read(0, buf, 1) == -1){
			strcpy(ppage0->argbuf, "b");
			ppage0->argv[0] = ppage0->argbuf;
			ppage0->argv[1] = 0;
			ppage0->argc = 1;
			bootcmd(ppage0->argc, ppage0->argv);
		}
		alarm(0);
	}

	notify(catcher);

loop:
	print(">>>");
	switch(ppage0->argc = response(ppage0->argbuf, ppage0->argv)){

	default:
		for(cmd = command; cmd->name; cmd++){
			if(strcmp(cmd->name, ppage0->argv[0]))
				continue;
			if((*cmd->f)(ppage0->argc, ppage0->argv))
				print("eh?\n");
			break;
		}
		if(cmd->name)
			break;
		/*FALLTHROUGH*/

	case -1:
		print("eh?\n");
		break;

	case 0:
		dot += dotincr;
		display(dot);
		break;
	}
	goto loop;
}
