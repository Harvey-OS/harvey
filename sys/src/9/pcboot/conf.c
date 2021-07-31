/*
 * parse plan.ini or /cfg/pxe/%E file into low memory
 */
#include	"u.h"
#include	"../port/lib.h"
#include	"mem.h"
#include	"dat.h"
#include	"fns.h"
#include	"io.h"
#include	"ureg.h"
#include	"pool.h"
#include	"../port/netif.h"
#include	"../ip/ip.h"
#include	"pxe.h"

typedef struct {
	char*	name;
	int	start;
	int	end;
} Mblock;

typedef struct {
	char*	tag;
	Mblock*	mb;
} Mitem;

Chan *conschan;

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

/* timeout is in seconds */
int
getstr(char *prompt, char *buf, int size, char *def, int timeout)
{
	int len, isdefault;
	static char pbuf[PRINTSIZE];

	if(conschan == nil)
		panic("getstr: #c/cons not open");
	buf[0] = 0;
	isdefault = (def && *def);
	if(isdefault == 0){
		timeout = 0;
		snprint(pbuf, sizeof pbuf, "%s: ", prompt);
	}
	else if(timeout)
		snprint(pbuf, sizeof pbuf, "%s[default==%s (%ds timeout)]: ",
			prompt, def, timeout);
	else
		snprint(pbuf, sizeof pbuf, "%s[default==%s]: ", prompt, def);
	for (;;) {
		print("%s", pbuf);
		if (timeout > 0) {
			for(timeout *= 1000; timeout > 0; timeout -= 100) {
				if (qlen(kbdq) > 0)	/* if input queued */
					break; 
				tsleep(&up->sleep, return0, 0, 100);
			}
			if (timeout <= 0) {		/* use default */
				print("\n");
				len = 0;
				break;
			}
		}
		buf[0] = '\0';
		len = devtab[conschan->type]->read(conschan, buf, size - 1,
			conschan->offset);
		if(len >= 0)
			buf[len] = '\0';
		switch(len){
		case 0:				/* eof */
		case 1:				/* newline */
			len = 0;
			buf[len] = '\0';
			if(!isdefault)
				continue;
			break;
		}
		if(len < size - 1)
			break;
		print("line too long\n");
	}
	if(len == 0 && isdefault)
		strncpy(buf, def, size);
	return 0;
}

void
askbootfile(char *buf, int len, char **bootfp, int secs, char *def)
{
	getstr("\nBoot from", buf, len, def, secs);
	trimnl(buf);
	if (bootfp)
		kstrdup(bootfp, buf);
}

int
isconf(char *name)
{
	int i;

	for(i = 0; i < nconf; i++)
		if(cistrcmp(confname[i], name) == 0)
			return 1;
	return 0;
}

/* result is not malloced, unlike user-mode getenv() */
char*
getconf(char *name)
{
	int i, n, nmatch;
	char buf[120];

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
	n = getfields(scratch, line, MAXCONF, 0, "\n");
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
			snprint(mdefaultbuf, sizeof mdefaultbuf, "%ld",
				mi-mitem+1);
			mdefault = mdefaultbuf;
		}
		else if(cistrncmp(p, "menuconsole=", 12) == 0){
			p += 12;
			p = comma(p, &q);
			i8250config(p);
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
		case '\n':
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

/* dig out tables created by l16r.s in real mode */
void
readlsconf(void)
{
	int i, n;
	uchar *p;
	MMap *map;
	u64int addr, len;

	/*
	 * we could be running above 1MB, so put bios tables in low memory,
	 * not after end.
	 */
	p = (uchar*)KADDR(BIOSTABLES);
	for(n = 0; n < nelem(mmap); n++){
		if(*p == 0)
			break;
		if(memcmp(p, "APM\0", 4) == 0){
			p += 20;
			continue;
		}
		else if(memcmp(p, "MAP\0", 4) == 0){
			map = (MMap*)p;

			switch(map->type){
			default:
				if(v_flag)
					print("type %ud", map->type);
				break;
			case 1:
				if(v_flag)
					print("Memory");
				break;
			case 2:
				if(v_flag)
					print("reserved");
				break;
			case 3:
				if(v_flag)
					print("ACPI Reclaim Memory");
				break;
			case 4:
				if(v_flag)
					print("ACPI NVS Memory");
				break;
			}
			addr = (((u64int)map->base[1])<<32)|map->base[0];
			len = (((u64int)map->length[1])<<32)|map->length[0];
			if(v_flag)
				print("\t%#16.16lluX %#16.16lluX (%llud)\n",
					addr, addr+len, len);

			if(nmmap < nelem(mmap)){
				memmove(&mmap[nmmap], map, sizeof(MMap));
				mmap[nmmap].size = 20;
				nmmap++;
			}
			p += 24;
			continue;
		}
		else{
			 /* ideally we shouldn't print here */
			print("\nweird low-memory map at %#p:\n", p);
			for(i = 0; i < 24; i++)
				print(" %2.2uX", *(p+i));
			print("\n");
			delay(5000);
		}
		break;
	}
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
dumpbootargs(void)
{
	char *p, *nl;

	/* in the boot, we can only print PRINTSIZE (256) bytes at a time. */
	print("boot args: ");
	for (p = (char *)BOOTARGS; *p != '\0'; p = nl) {
		nl = strchr(p, '\n');
		if (nl != nil) {
			++nl;
			print("%.*s", (int)(nl - p), p);
		}
	}
}

void
changeconf(char *fmt, ...)
{
	va_list arg;
	char *p, *q, pref[20], buf[128];

	va_start(arg, fmt);
	vseprint(buf, buf+sizeof buf, fmt, arg);
	va_end(arg);

	pref[0] = '\n';
	strncpy(pref+1, buf, 19);
	pref[19] = '\0';
	if(p = strchr(pref, '='))
		*(p+1) = '\0';
	else
		print("warning: did not change %s in plan9.ini\n", buf);

	/* find old line by looking for \nwhat= */
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
static char id[8] = "ZORT 0\r\n";

int
dotini(char *inibuf)
{
	int blankline, i, incomment, inspace, n;
	char *cp, *p, *q, *line[MAXCONF];

	cp = inibuf;

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

	n = getfields(cp, line, MAXCONF, 0, "\n");
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
