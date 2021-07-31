#include "u.h"
#include "lib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"
#include "io.h"

#include "fs.h"

/*
 * Where configuration info is left for the loaded programme.
 * This will turn into a structure as more is done by the boot loader
 * (e.g. why parse the .ini file twice?).
 * There are 3584 bytes available at CONFADDR.
 *
 * The low-level boot routines in l.s leave data for us at CONFADDR,
 * which we pick up before reading the plan9.ini file.
 */
#define BOOTLINELEN	64
#define BOOTARGS	((char*)(CONFADDR+BOOTLINELEN))
#define	BOOTARGSLEN	(3584-0x200-BOOTLINELEN)
#define	MAXCONF		100

static char *confname[MAXCONF];
static char *confval[MAXCONF];
static int nconf;

extern char **ini;

typedef struct {
	char*	name;
	int	start;
	int	end;
} Mblock;

typedef struct {
	char*	tag;
	Mblock*	mb;
} Mitem;

static Mblock mblock[MAXCONF];
static int nmblock;
static Mitem mitem[MAXCONF];
static int nmitem;
static char* mdefault;
static char mdefaultbuf[10];
static int mtimeout;

static char*
comma(char* line, char** residue)
{
	char *q, *r;

	if((q = strchr(line, ',')) != nil){
		*q++ = 0;
		if(*q == ' ')
			q++;
	}
	*residue = q;

	if((r = strchr(line, ' ')) != nil)
		*r = 0;

	if(*line == ' ')
		line++;
	return line;
}

static Mblock*
findblock(char* name, char** residue)
{
	int i;
	char *p;

	p = comma(name, residue);
	for(i = 0; i < nmblock; i++){
		if(strcmp(p, mblock[i].name) == 0)
			return &mblock[i];
	}
	return nil;
}

static Mitem*
finditem(char* name, char** residue)
{
	int i;
	char *p;

	p = comma(name, residue);
	for(i = 0; i < nmitem; i++){
		if(strcmp(p, mitem[i].mb->name) == 0)
			return &mitem[i];
	}
	return nil;
}

static void
parsemenu(char* str, char* scratch, int len)
{
	Mitem *mi;
	Mblock *mb, *menu;
	char buf[20], *p, *q, *line[MAXCONF];
	int i, inblock, n, show;

	inblock = 0;
	menu = nil;
	memmove(scratch, str, len);
	n = getfields(scratch, line, MAXCONF, '\n');
	if(n >= MAXCONF)
		print("warning: possibly too many lines in plan9.ini\n");
	for(i = 0; i < n; i++){
		p = line[i];
		if(inblock && *p == '['){
			mblock[nmblock].end = i;
			if(strcmp(mblock[nmblock].name, "menu") == 0)
				menu = &mblock[nmblock];
			nmblock++;
			inblock = 0;
		}
		if(*p == '['){
			if(nmblock == 0 && i != 0){
				mblock[nmblock].name = "common";
				mblock[nmblock].start = 0;
				mblock[nmblock].end = i;
				nmblock++;
			}
			q = strchr(p+1, ']');
			if(q == nil || *(q+1) != 0){
				print("malformed menu block header - %s\n", p);
				return;
			}
			*q = 0;
			mblock[nmblock].name = p+1;
			mblock[nmblock].start = i+1;
			inblock = 1;
		}
	}

	if(inblock){
		mblock[nmblock].end = i;
		nmblock++;
	}
	if(menu == nil)
		return;
	if(nmblock < 2){
		print("incomplete menu specification\n");
		return;
	}

	for(i = menu->start; i < menu->end; i++){
		p = line[i];
		if(cistrncmp(p, "menuitem=", 9) == 0){
			p += 9;
			if((mb = findblock(p, &q)) == nil){
				print("no block for menuitem %s\n", p);
				return;
			}
			if(q != nil)
				mitem[nmitem].tag = q;
			else
				mitem[nmitem].tag = mb->name;
			mitem[nmitem].mb = mb;
			nmitem++;
		}
		else if(cistrncmp(p, "menudefault=", 12) == 0){
			p += 12;
			if((mi = finditem(p, &q)) == nil){
				print("no item for menudefault %s\n", p);
				return;
			}
			if(q != nil)
				mtimeout = strtol(q, 0, 0);
			sprint(mdefaultbuf, "%ld", mi-mitem+1);
			mdefault = mdefaultbuf;
		}
		else if(cistrncmp(p, "menuconsole=", 12) == 0){
			p += 12;
			p = comma(p, &q);
			consinit(p, q);
		}
		else{
			print("invalid line in [menu] block - %s\n", p);
			return;
		}
	}

again:
	print("\nPlan 9 Startup Menu:\n====================\n");
	for(i = 0; i < nmitem; i++)
		print("    %d. %s\n", i+1, mitem[i].tag);
	for(;;){
		getstr("Selection", buf, sizeof(buf), mdefault, mtimeout);
		mtimeout = 0;
		i = strtol(buf, &p, 0)-1;
		if(i < 0 || i >= nmitem)
			goto again;
		switch(*p){
		case 'p':
		case 'P':
			show = 1;
			print("\n");
			break;
		case 0:
			show = 0;
			break;
		default:
			continue;
			
		}
		mi = &mitem[i];
	
		p = str;
		p += sprint(p, "menuitem=%s\n", mi->mb->name);
		for(i = 0; i < nmblock; i++){
			mb = &mblock[i];
			if(mi->mb != mb && cistrcmp(mb->name, "common") != 0)
				continue;
			for(n = mb->start; n < mb->end; n++)
				p += sprint(p, "%s\n", line[n]);
		}

		if(show){
			for(q = str; q < p; q += i){
				if((i = print(q)) <= 0)
					break;
			}
			goto again;
		}
		break;
	}
	print("\n");
}

/*
static void
msleep(int msec)
{
	ulong start;

	for(start = m->ticks; TK2MS(m->ticks - start) < msec; )
		;
}
*/

void
readlsconf(void)
{
	uchar *p;

	p = (uchar*)CONFADDR;
	for(;;) {
		if(strcmp((char*)p, "APM") == 0){
			apm.haveinfo = 1;
			apm.ax = *(ushort*)(p+4);
			apm.cx = *(ushort*)(p+6);
			apm.dx = *(ushort*)(p+8);
			apm.di = *(ushort*)(p+10);
			apm.ebx = *(ulong*)(p+12);
			apm.esi = *(ulong*)(p+16);
			print("apm ax=%x cx=%x dx=%x di=%x ebx=%x esi=%x\n",
				apm.ax, apm.cx, apm.dx, apm.di, apm.ebx, apm.esi);
			p += 20;
			continue;
		}
		break;
	}
}

char*
getconf(char *name)
{
	int i, n, nmatch;
	char buf[20];

	nmatch = 0;
	for(i = 0; i < nconf; i++)
		if(cistrcmp(confname[i], name) == 0)
			nmatch++;

	switch(nmatch) {
	default:
		print("\n");
		nmatch = 0;
		for(i = 0; i < nconf; i++)
			if(cistrcmp(confname[i], name) == 0)
				print("%d. %s\n", ++nmatch, confval[i]);
		print("%d. none of the above\n", ++nmatch);
		do {
			getstr(name, buf, sizeof(buf), nil, 0);
			n = atoi(buf);
		} while(n < 1 || n > nmatch);

		for(i = 0; i < nconf; i++)
			if(cistrcmp(confname[i], name) == 0)
				if(--n == 0)
					return confval[i];
		break;

	case 1:
		for(i = 0; i < nconf; i++)
			if(cistrcmp(confname[i], name) == 0)
				return confval[i];
		break;

	case 0:
		break;
	}
	return nil;
}

void
addconf(char *fmt, ...)
{
	va_list arg;

	va_start(arg, fmt);
	vseprint(BOOTARGS+strlen(BOOTARGS), BOOTARGS+BOOTARGSLEN, fmt, arg);
	va_end(arg);
}

void
changeconf(char *fmt, ...)
{
	va_list arg;
	char *p, *q, pref[20], buf[128];

	va_start(arg, fmt);
	vseprint(buf, buf+sizeof buf, fmt, arg);
	va_end(arg);
	strncpy(pref+1, buf, 19);
	pref[19] = '\0';
	if(p = strchr(pref, '='))
		*(p+1) = '\0';
	else
		print("warning: did not change %s in plan9.ini\n", buf);

	/* find old line by looking for \nwhat= */
	pref[0] = '\n';
	if(strncmp(BOOTARGS, pref+1, strlen(pref+1)) == 0)
		p = BOOTARGS;
	else if(p = strstr(BOOTARGS, pref))
		p++;
	else
		p = nil;

	/* move rest of args up, deleting what= line. */
	if(p != nil && (q = strchr(p, '\n')) != nil)
		memmove(p, q+1, strlen(q+1)+1);

	/* add replacement to end */
	addconf("%s", buf);
}

/*
 *  read configuration file
 */
static char inibuf[BOOTARGSLEN];
static char id[8] = "ZORT 0\r\n";

int
dotini(Fs *fs)
{
	File rc;
	int blankline, i, incomment, inspace, n;
	char *cp, *p, *q, *line[MAXCONF];

	if(fswalk(fs, *ini, &rc) <= 0)
		return -1;

	cp = inibuf;
	*cp = 0;
	n = fsread(&rc, cp, BOOTARGSLEN-1);
	if(n <= 0)
		return -1;

	cp[n] = 0;

	/*
	 * Strip out '\r', change '\t' -> ' '.
	 * Change runs of spaces into single spaces.
	 * Strip out trailing spaces, blank lines.
	 *
	 * We do this before we make the copy so that if we 
	 * need to change the copy, it is already fairly clean.
	 * The main need is in the case when plan9.ini has been
	 * padded with lots of trailing spaces, as is the case 
	 * for those created during a distribution install.
	 */
	p = cp;
	blankline = 1;
	incomment = inspace = 0;
	for(q = cp; *q; q++){
		if(*q == '\r')
			continue;
		if(*q == '\t')
			*q = ' ';
		if(*q == ' '){
			inspace = 1;
			continue;
		}
		if(*q == '\n'){
			if(!blankline){
				if(!incomment)
					*p++ = '\n';
				blankline = 1;
			}
			incomment = inspace = 0;
			continue;
		}
		if(inspace){
			if(!blankline && !incomment)
				*p++ = ' ';
			inspace = 0;
		}
		if(blankline && *q == '#')
			incomment = 1;
		blankline = 0;
		if(!incomment)
			*p++ = *q;	
	}
	if(p > cp && p[-1] != '\n')
		*p++ = '\n';
	*p++ = 0;
	n = p-cp;

	parsemenu(cp, BOOTARGS, n);

	/*
	 * Keep a copy.
	 * We could change this to pass the parsed strings
	 * to the booted programme instead of the raw
	 * string, then it only gets done once.
	 */
	if(strncmp(cp, id, sizeof(id))){
		memmove(BOOTARGS, id, sizeof(id));
		if(n+1+sizeof(id) >= BOOTARGSLEN)
			n -= sizeof(id);
		memmove(BOOTARGS+sizeof(id), cp, n+1);
	}
	else
		memmove(BOOTARGS, cp, n+1);

	n = getfields(cp, line, MAXCONF, '\n');
	for(i = 0; i < n; i++){
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
	return 0;
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
			else if(cistrncmp(p, "mem=", 4) == 0)
				isa->mem = strtoul(p+4, &p, 0);
			else if(cistrncmp(p, "size=", 5) == 0)
				isa->size = strtoul(p+5, &p, 0);
			else if(cistrncmp(p, "ea=", 3) == 0){
				if(parseether(isa->ea, p+3) == -1)
					memset(isa->ea, 0, 6);
			}
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
