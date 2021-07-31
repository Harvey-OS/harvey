#include	"u.h"
#include	"lib.h"
#include	"mem.h"
#include	"dat.h"
#include	"fns.h"
#include	"io.h"


extern ulong edata;

/*
 *  predeclared
 */
int	fddosboot(void);
int	hddosboot(void);
int	hd9boot(void);
int	duartboot(void);
int	parse(char*);
int	getline(int);
int	getstr(char*, char*, int, char*, int);
int	menu(void);
void	help(void);

char	file[2*NAMELEN];
char	server[NAMELEN];
char	sysname[NAMELEN];
char	user[NAMELEN] = "none";
char	linebuf[256];
Conf	conf;
int	have9part;


typedef	struct Booter	Booter;
struct Booter
{
	char	*name;
	char	*srv;
	char	*file;
	int	(*func)(void);
	char	*exp;
};
Booter	booter[] = {
	{ "h",	"0",	"boot",	hd9boot, "Plan 9 hard disk 'x' partition 'y'" },
	{ "fd",	"0",	"9",	fddosboot, "DOS floppy disk 'x' file 'y'"  },
	{ "hd",	"0",	"9",	hddosboot, "DOS hard disk 'x' file 'y'" }, 
};
#define NBOOT (sizeof(booter)/sizeof(Booter))

int	bootdev;

void
main(void)
{
	char	element[2*NAMELEN];	/* next element in a file path */
	char	def[2*NAMELEN];
	int	hards, floppys;
	char	buf[512];

	meminit();
	machinit();
	confinit();
	screeninit();
	trapinit();
	clockinit();
	alarminit();
	kbdinit();
	floppys = floppyinit();
	spllo();
	hards = hardinit();

	/* if we have an empty floppy drive, try booting from hard disk */
	*def = 0;
	if(floppys){
		if(floppyread(0, buf, 512) >= 0){
			bootdev = 1;
			strcpy(def, "fd!0!9dos");
		} else if(floppyread(1, buf, 512) >= 0){
			bootdev = 1;
			strcpy(def, "fd!1!9dos");
		}
	}
	if(*def == 0 && hards){
		if(have9part){
			bootdev = 0;
			strcpy(def, "h!0!boot");
		} else {
			bootdev = 2;
			strcpy(def, "hd!0!9dos");
		}
		strcpy(element, def);
		parse(element);
		(*booter[0].func)();
	}

	bootdev = hards ? 0 : (floppys ? 1 : 2);
	help();
	for(;;){
		if(getstr("boot from", element, sizeof element, def, 0)<0)
			continue;
		if(parse(element) < 0)
			continue;
		if((*booter[bootdev].func)() < 0)
			continue;
		print("success\n");
	}
}

void
meminit(void)
{
	char *id = (char*)(ROMBIOS + 0xFF40);

	/* turn on extended mem */
	if(strncmp(id, "AT&TNSX", 7) == 0)
		heada20();		/* use headland chip */
	else
		i8042a20();		/* use keyboard controller */
}

void
help(void)
{
	int i;

	print("Choose one of the following boot methods\n");
	for(i = 0; i < NBOOT; i++)
		print("\t%s!x!y - boot from %s\n", booter[i].name, booter[i].exp);
	print("\n");
}

/*
 *  parse the server line.  return 0 if OK, -1 if bullshit
 */
int
parse(char *line)
{
	char *def[3];
	char **d;
	char *s;
	int i;

	strcpy(BOOTLINE, line);
	def[0] = booter[bootdev].name;
	def[1] = booter[bootdev].srv;
	def[2] = "9";

	d = &def[2];
	s = line + strlen(line);
	while((*d = s) > line)
		if(*--s == '!'){
			if(d-- == def)
				return -1;
			*s = '\0';
		}
	for(i = 0; i < NBOOT; i++){
		if(strcmp(def[0], booter[i].name)==0){
			strcpy(server, def[1]);
			strcpy(file, def[2]);
			bootdev = i;
			return 0;
		}
/*print("%s %s %d\n", def[0], booter[i].name, strcmp(def[0], booter[i].name));/**/
	}

	return -1;
}

/*
 *  read a line from the keyboard.
 */
int
getline(int quiet)
{
	int c, i=0;

	for (;;) {
	    	do{
			c = getc(&kbdq);
		} while(c==-1);
		if(c == '\r')
			c = '\n'; /* turn carriage return into newline */
		if(c == '\177')
			c = '\010';	/* turn delete into backspace */
		if(!quiet){
			if(c == '\033'){
				menu();
				return -1;
			}
			if(c == '\025')
				screenputc('\n');	/* echo ^U as a newline */
			else
				screenputc(c);
		}
		if(c == '\010'){
			if(i > 0)
				i--; /* bs deletes last character */
			continue;
		}
		/* a newline ends a line */
		if (c == '\n')
			break;
		/* ^U wipes out the line */
		if (c =='\025')
			return -1;
		linebuf[i++] = c;
	}
	linebuf[i] = 0;
	return i;
}

/*
 *  prompt for a string from the keyboard.  <cr> returns the default.
 */
int
getstr(char *prompt, char *buf, int size, char *def, int quiet)
{
	int len;

	for (;;) {
		if(def)
			print("%s[default==%s]: ", prompt, def);
		else
			print("%s: ", prompt);
		len = getline(quiet);
		switch(len){
		case -1:
			/* ^U typed */
			continue;
		case -2:
			/* timeout */
			return -1;
		default:
			break;
		}
		if(len >= size){
			print("line too long\n");
			continue;
		}
		break;
	}
	if(*linebuf==0 && def)
		strcpy(buf, def);
	else
		strcpy(buf, linebuf);
	return 0;
}

int
menu(void)
{
	return 0;
}

void
machinit(void)
{
	int n;

	n = m->machno;
	memset(m, 0, sizeof(Mach));
	m->machno = n;
	m->mmask = 1<<m->machno;
	active.machs = 1;
}

void
confinit(void)
{
	conf.nalarm = 1000;
	conf.nfloppy = 2;
	conf.nhard = 1;
}

int
sprint(char *s, char *fmt, ...)
{
	return doprint(s, s+PRINTSIZE, fmt, (&fmt+1)) - s;
}

int
print(char *fmt, ...)
{
	char buf[PRINTSIZE];
	int n;

	n = doprint(buf, buf+sizeof(buf), fmt, (&fmt+1)) - buf;
	screenputs(buf, n);
	return n;
}

void
panic(char *fmt, ...)
{
	char buf[PRINTSIZE];
	int n;

	screenputs("panic: ", 7);
	n = doprint(buf, buf+sizeof(buf), fmt, (&fmt+1)) - buf;
	screenputs(buf, n);
	screenputs("\n", 1);
	spllo();
	for(;;)
		idle();
}

void
kbdputc(IOQ* q, int c)
{
	if(c==0x10)
		panic("^p");
	putc(q, c);
}

struct Palloc palloc;

void*
ialloc(ulong n, int align)
{
	ulong p;

	if(palloc.active && n!=0)
		print("ialloc bad\n");
	if(palloc.addr == 0)
		palloc.addr = ((ulong)&end)&~KZERO;
	if(align)
		palloc.addr = PGROUND(palloc.addr);

	if((palloc.addr < 640*1024) && (palloc.addr + n > 640*1024))
		palloc.addr = 1024*1024;

	memset((void*)(palloc.addr|KZERO), 0, n);
	p = palloc.addr;
	palloc.addr += n;
	if(align)
		palloc.addr = PGROUND(palloc.addr);

	return (void*)(p|KZERO);
}

/*
 *  grab next element from a path, return the pointer to unprocessed portion of
 *  path.
 */
char *
nextelem(char *path, char *elem)
{
	int i;

	while(*path == '/')
		path++;
	if(*path==0 || *path==' ')
		return 0;
	for(i=0; *path!='\0' && *path!='/' && *path!=' '; i++){
		if(i==28){
			print("name component too long\n");
			return 0;
		}
		*elem++ = *path++;
	}
	*elem = '\0';
	return path;
}

/*
 *  like andrew's getmfields but no hidden state
 */
int
getfields(char *lp,	/* to be parsed */
	char **fields,	/* where to put pointers */
	int n,		/* number of pointers */
	char sep	/* separator */
)
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

/*
 *  some dummy's so we can use kernel code
 */
void
sched(void)
{ }

void
ready(Proc*p)
{
	USED(p);
}

int
postnote(Proc*p, int x, char* y, int z)
{
	USED(p, x, y, z);
	panic("postnote");
	return 0;
}

/*
 *  boot from the duart
 */
int
duartboot(void)
{
	print("duartboot unimplemented\n");
	return -1;
}

#include "dosfs.h"
Dos b;

int
fddosboot(void)
{
	extern int dosboot(Dos*, char*);

	b.dev = atoi(server);
	print("booting from DOS floppy %d\n", b.dev);
	b.seek = floppyseek;
	b.read = floppyread;
	return dosboot(&b, file);
}

int
hddosboot(void)
{
	extern int dosboot(Dos*, char*);

	b.dev = atoi(server);
	print("booting from DOS hard disk %d\n", b.dev);
	b.seek = hardseek;
	b.read = hardread;
	sethardpart(b.dev, "disk");
	return dosboot(&b, file);
}

/*
 *  boot from the harddisk
 */
int
hd9boot(void)
{
	print("booting from hard disk %s Plan 9 parititon %s\n", server, file);
	if(sethardpart(atoi(server), file) < 0){
		print("no '%s' partition on disk %d\n", file, atoi(server));
		return -1;
	}
	return p9boot(atoi(server), hardseek, hardread);
}

enum {
	Paddr=		0x70,	/* address port */
	Pdata=		0x71,	/* data port */
};

uchar
nvramread(int offset)
{
	outb(Paddr, offset);
	return inb(Pdata);
}
