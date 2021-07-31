#include	"all.h"
#include	"io.h"

static void	dowormcopy(void);

/*
 * This is needed for IP configuration.
 */
#include	"../ip/ip.h"

struct
{
	char*	icharp;
	char*	charp;
	int	error;
	int	newconf;	/* clear befor start */
	int	modconf;	/* write back when done */
	int	nextiter;
	int	lastiter;
	int	diriter;
	int	ipauthset;
	Device*	lastcw;
	Device*	devlist;
} f;

static Device* confdev;
static int copyworm = 0;

int
devcmpr(Device *d1, Device *d2)
{

loop:
	if(d1 == d2)
		return 0;
	if(d1 == 0 || d2 == 0 || d1->type != d2->type)
		return 1;

	switch(d1->type) {
	default:
		print("can't compare dev: %Z\n", d1);
		panic("devcmp");
		break;

	case Devmcat:
	case Devmlev:
	case Devmirr:
		d1 = d1->cat.first;
		d2 = d2->cat.first;
		while(d1 && d2) {
			if(devcmpr(d1, d2))
				return 1;
			d1 = d1->link;
			d2 = d2->link;
		}
		goto loop;

	case Devnone:
		return 0;

	case Devro:
		d1 = d1->ro.parent;
		d2 = d2->ro.parent;
		goto loop;

	case Devjuke:
	case Devcw:
		if(devcmpr(d1->cw.c, d2->cw.c))
			break;
		d1 = d1->cw.w;
		d2 = d2->cw.w;
		goto loop;

	case Devfworm:
		d1 = d1->fw.fw;
		d2 = d2->fw.fw;
		goto loop;

	case Devwren:
	case Devworm:
	case Devlworm:
	case Devide:
		if(d1->wren.ctrl == d2->wren.ctrl)
		if(d1->wren.targ == d2->wren.targ)
		if(d1->wren.lun == d2->wren.lun)
			return 0;
		break;

	case Devpart:
		if(d1->part.base == d2->part.base)
		if(d1->part.size == d2->part.size) {
			d1 = d1->part.d;
			d2 = d2->part.d;
			goto loop;
		}
		break;
	}
	return 1;
}

void
cdiag(char *s, int c1)
{

	f.charp--;
	if(f.error == 0) {
		print("config diag: %s -- <%c>\n", s, c1);
		f.error = 1;
	}
}

int
cnumb(void)
{
	int c, n;

	c = *f.charp++;
	if(c == '<') {
		n = f.nextiter;
		if(n >= 0) {
			f.nextiter = n+f.diriter;
			if(n == f.lastiter) {
				f.nextiter = -1;
				f.lastiter = -1;
			}
			for(;;) {
				c = *f.charp++;
				if(c == '>')
					break;
			}
			return n;
		}
		n = cnumb();
		if(*f.charp++ != '-') {
			cdiag("- expected", f.charp[-1]);
			return 0;
		}
		c = cnumb();
		if(*f.charp++ != '>') {
			cdiag("> expected", f.charp[-1]);
			return 0;
		}
		f.lastiter = c;
		f.diriter = 1;
		if(n > c)
			f.diriter = -1;
		f.nextiter = n+f.diriter;
		return n;
	}
	if(c < '0' || c > '9') {
		cdiag("number expected", c);
		return 0;
	}
	n = 0;
	while(c >= '0' && c <= '9') {
		n = n*10 + (c-'0');
		c = *f.charp++;
	}
	f.charp--;
	return n;
}

Device*
config1(int c)
{
	Device *d, *t;
	int m;

	d = ialloc(sizeof(Device), 0);
	for(;;) {
		t = config();
		if(d->cat.first == 0)
			d->cat.first = t;
		else
			d->cat.last->link = t;
		d->cat.last = t;
		if(f.error)
			goto bad;
		m = *f.charp;
		if(c == '(' && m == ')') {
			d->type = Devmcat;
			break;
		}
		if(c == '[' && m == ']') {
			d->type = Devmlev;
			break;
		}
		if(c == '{' && m == '}') {
			d->type = Devmirr;
			break;
		}
	}
	f.charp++;
	if(d->cat.first == d->cat.last)
		d = d->cat.first;
	return d;

bad:
	return devnone;
}

Device*
config(void)
{
	int c, m;
	Device *d;
	char *icp;

	if(f.error)
		goto bad;
	d = ialloc(sizeof(Device), 0);

	c = *f.charp++;
	switch(c) {
	default:
		cdiag("unknown type", c);
		goto bad;

	case '(':	/* (d+) one or multiple cat */
	case '[':	/* [d+] one or multiple interleave */
	case '{':	/* {d+} a mirrored device and optional mirrors */
		return config1(c);

	case 'f':	/* fd fake worm */
		d->type = Devfworm;
		d->fw.fw = config();
		break;

	case 'n':
		d->type = Devnone;
		break;

	case 'w':	/* w[#.]#[.#] wren [ctrl] unit [lun] */
	case 'h':	/* h[#.]# ide [ctlr] unit */
	case 'r':	/* r# worm side */
	case 'l':	/* l# labelled-worm side */
		icp = f.charp;
		if(c == 'h')
			d->type = Devide;
		else
			d->type = Devwren;
		d->wren.ctrl = 0;
		d->wren.targ = cnumb();
		d->wren.lun = 0;
		m = *f.charp;
		if(m == '.') {
			f.charp++;
			d->wren.lun = cnumb();
			m = *f.charp;
			if(m == '.') {
				f.charp++;
				d->wren.ctrl = d->wren.targ;
				d->wren.targ = d->wren.lun;
				d->wren.lun = cnumb();
			}
		}
		if(f.nextiter >= 0)
			f.charp = icp-1;
		if(c == 'r') {	/* worms are virtual and not uniqued */
			d->type = Devworm;
			break;
		}
		if(c == 'l') {
			d->type = Devlworm;
			break;
		}
		break;

	case 'o':	/* o ro part of last cw */
		if(f.lastcw == 0) {
			cdiag("no cw to match", c);
			goto bad;
		}
		return f.lastcw->cw.ro;

	case 'j':	/* DD jukebox */
		d->type = Devjuke;
		d->j.j = config();
		d->j.m = config();
		break;

	case 'c':	/* cache/worm */
		d->type = Devcw;
		d->cw.c = config();
		d->cw.w = config();
		d->cw.ro = ialloc(sizeof(Device), 0);
		d->cw.ro->type = Devro;
		d->cw.ro->ro.parent = d;
		f.lastcw = d;
		break;

	case 'p':	/* pd#.# partition base% size% */
		d->type = Devpart;
		d->part.d = config();
		d->part.base = cnumb();
		c = *f.charp++;
		if(c != '.')
			cdiag("dot expected", c);
		d->part.size = cnumb();
		break;

	case 'x':	/* xD swab a device */
		d->type = Devswab;
		d->swab.d = config();
		break;
	}
	d->dlink = f.devlist;
	f.devlist = d;
	return d;

bad:
	return devnone;
}

char*
strdup(char *s)
{
	int n;
	char *s1;

	n = strlen(s);
	s1 = ialloc(n+1, 0);
	strcpy(s1, s);
	return s1;
}

Device*
iconfig(char *s)
{
	Device *d;

	f.nextiter = -1;
	f.lastiter = -1;
	f.error = 0;
	f.icharp = s;
	f.charp = f.icharp;
	d = config();
	if(*f.charp) {
		cdiag("junk on end", *f.charp);
		f.error = 1;
	}
	return d;
}

int
testconfig(char *s)
{

	iconfig(s);
	return f.error;
}

int
astrcmp(char *a, char *b)
{
	int n, c;

	n = strlen(b);
	if(memcmp(a, b, n))
		return 1;
	c = a[n];
	if(c == 0) {
		aindex = 0;
		return 0;
	}
	if(a[n+1])
		return 1;
	if(c >= '0' && c <= '9') {
		aindex = c - '0';
		return 0;
	}
	return 1;
}

void
mergeconf(Iobuf *p)
{
	char word[100];
	char *cp;
	Filsys *fs;

	cp = p->iobuf;
	goto line;

loop:
	if(*cp != '\n')
		goto bad;
	cp++;

line:
	cp = getwd(word, cp);
	if(strcmp(word, "") == 0)
		return;
	if(strcmp(word, "service") == 0) {
		cp = getwd(word, cp);
		if(service[0] == 0)
			strcpy(service, word);
		goto loop;
	}
	if(strcmp(word, "ipauth") == 0) {
		cp = getwd(word, cp);
		if(!f.ipauthset)
			if(chartoip(authip, word))
				goto bad;
		goto loop;
	}
	if(astrcmp(word, "ip") == 0) {
		cp = getwd(word, cp);
		if(!isvalidip(ipaddr[aindex].sysip))
			if(chartoip(ipaddr[aindex].sysip, word))
				goto bad;
		goto loop;
	}
	if(astrcmp(word, "ipgw") == 0) {
		cp = getwd(word, cp);
		if(!isvalidip(ipaddr[aindex].defgwip))
			if(chartoip(ipaddr[aindex].defgwip, word))
				goto bad;
		goto loop;
	}
	if(astrcmp(word, "ipsntp") == 0) {
		cp = getwd(word, cp);
		if (!isvalidip(sntpip))
			if (chartoip(sntpip, word))
				goto bad;
		goto loop;
	}
	if(astrcmp(word, "ipmask") == 0) {
		cp = getwd(word, cp);
		if(!isvalidip(ipaddr[aindex].defmask))
			if(chartoip(ipaddr[aindex].defmask, word))
				goto bad;
		goto loop;
	}
	if(strcmp(word, "filsys") == 0) {
		cp = getwd(word, cp);
		for(fs=filsys; fs->name; fs++)
			if(strcmp(fs->name, word) == 0) {
				if(fs->flags & FEDIT) {
					cp = getwd(word, cp);
					goto loop;
				}
				break;
			}
		fs->name = strdup(word);
		cp = getwd(word, cp);
		fs->conf = strdup(word);
		goto loop;
	}
	goto bad;

bad:
	putbuf(p);
	panic("unknown word in config block: %s", word);
}

void
cmd_printconf(int, char *[])
{
	char *p, *s;
	Iobuf *iob;

	iob = getbuf(confdev, 0, Bread);
	if(iob == nil)
		return;
	if(checktag(iob, Tconfig, 0)){
		putbuf(iob);
		return;
	}

	print("config %s\n", nvrgetconfig());
	s = p = iob->iobuf;
	while(*p != 0 && p < iob->iobuf+BUFSIZE){
		if(*p++ != '\n')
			continue;
		print("%.*s", (int)(p-s), s);
		s = p;
	}
	if(p != s)
		print("%.*s", (int)(p-s), s);
	print("end\n");

	putbuf(iob);
}

void
sysinit(void)
{
	Filsys *fs;
	int error, i;
	Device *d;
	Iobuf *p;
	char *cp;

	dofilter(u->time+0, C0a, C0b, 1);
	dofilter(u->time+1, C1a, C1b, 1);
	dofilter(u->time+2, C2a, C2b, 1);
	dofilter(cons.work+0, C0a, C0b, 1);
	dofilter(cons.work+1, C1a, C1b, 1);
	dofilter(cons.work+2, C2a, C2b, 1);
	dofilter(cons.rate+0, C0a, C0b, 1000);
	dofilter(cons.rate+1, C1a, C1b, 1000);
	dofilter(cons.rate+2, C2a, C2b, 1000);
	dofilter(cons.bhit+0, C0a, C0b, 1);
	dofilter(cons.bhit+1, C1a, C1b, 1);
	dofilter(cons.bhit+2, C2a, C2b, 1);
	dofilter(cons.bread+0, C0a, C0b, 1);
	dofilter(cons.bread+1, C1a, C1b, 1);
	dofilter(cons.bread+2, C2a, C2b, 1);
	dofilter(cons.brahead+0, C0a, C0b, 1);
	dofilter(cons.brahead+1, C1a, C1b, 1);
	dofilter(cons.brahead+2, C2a, C2b, 1);
	dofilter(cons.binit+0, C0a, C0b, 1);
	dofilter(cons.binit+1, C1a, C1b, 1);
	dofilter(cons.binit+2, C2a, C2b, 1);
	cons.chan = chaninit(Devcon, 1, 0);

start:
	/*
	 * part 1 -- read the config file
	 */
	devnone = iconfig("n");

	cp = nvrgetconfig();
	print("config %s\n", cp);

	confdev = d = iconfig(cp);
	devinit(d);
	if(f.newconf) {
		p = getbuf(d, 0, Bmod);
		memset(p->iobuf, 0, RBUFSIZE);
		settag(p, Tconfig, 0);
	} else
		p = getbuf(d, 0, Bread|Bmod);
	if(!p || checktag(p, Tconfig, 0))
		panic("config io");
	mergeconf(p);
	if(f.modconf) {
		memset(p->iobuf, 0, BUFSIZE);
		p->flags |= Bmod|Bimm;
		if(service[0])
			sprint(strchr(p->iobuf, 0), "service %s\n", service);
		for(fs=filsys; fs->name; fs++)
			if(fs->conf)
				sprint(strchr(p->iobuf, 0),
					"filsys %s %s\n", fs->name, fs->conf);
		sprint(strchr(p->iobuf, 0), "ipauth %I\n", authip);
		sprint(strchr(p->iobuf, 0), "ipsntp %I\n", sntpip);
		for(i=0; i<10; i++) {
			if(isvalidip(ipaddr[i].sysip))
				sprint(strchr(p->iobuf, 0),
					"ip%d %I\n", i, ipaddr[i].sysip);
			if(isvalidip(ipaddr[i].defgwip))
				sprint(strchr(p->iobuf, 0),
					"ipgw%d %I\n", i, ipaddr[i].defgwip);
			if(isvalidip(ipaddr[i].defmask))
				sprint(strchr(p->iobuf, 0),
					"ipmask%d %I\n", i, ipaddr[i].defmask);
		}
		putbuf(p);
		f.modconf = 0;
		f.newconf = 0;
		print("config block written\n");
		goto start;
	}
	putbuf(p);

	print("service    %s\n", service);
	print("ipauth  %I\n", authip);
	print("ipsntp  %I\n", sntpip);
	for(i=0; i<10; i++) {
		if(isvalidip(ipaddr[i].sysip)) {
			print("ip%d     %I\n", i, ipaddr[i].sysip);
			print("ipgw%d   %I\n", i, ipaddr[i].defgwip);
			print("ipmask%d %I\n", i, ipaddr[i].defmask);
		}
	}

loop:
	/*
	 * part 2 -- squeeze out the deleted filesystems
	 */
	for(fs=filsys; fs->name; fs++)
		if(fs->conf == 0) {
			for(; fs->name; fs++)
				*fs = *(fs+1);
			goto loop;
		}
	if(filsys[0].name == 0)
		panic("no filsys");

	/*
	 * part 3 -- compile the device expression
	 */
	error = 0;
	for(fs=filsys; fs->name; fs++) {
		print("filsys %s %s\n", fs->name, fs->conf);
		fs->dev = iconfig(fs->conf);
		if(f.error) {
			error = 1;
			continue;
		}
	}
	if(error)
		panic("fs config");

	/*
	 * part 4 -- initialize the devices
	 */
	for(fs=filsys; fs->name; fs++) {
		delay(3000);
		print("sysinit: %s\n", fs->name);
		if(fs->flags & FREAM)
			devream(fs->dev, 1);
		if(fs->flags & FRECOVER)
			devrecover(fs->dev);
		devinit(fs->dev);
	}

	if (copyworm)
		dowormcopy();		/* no return */
}

extern int devatadebug, devataidedebug;

/* copy worm fs from "main" to "output" */
static void
dowormcopy(void)
{
	Filsys *f1, *f2;
	Device *from, *to;
	Iobuf *p;
	long a, lim;

	/* find source and target file systems */
	f1 = fsstr("main");
	if(f1 == nil)
		panic("main file system missing");
	f2 = fsstr("output");
	if(f2 == nil)
		print("no output file system - check only");
	from = f1->dev;
	if(from->type == Devcw)
		from = from->cw.w;
	if (f2) {
		to = f2->dev;
		print("copying worm from %Z to %Z, starting in 8 seconds\n",
			from, to);
		delay(8000);
	} else {
		to = nil;
		print("reading worm from %Z\n", from);
	}

	/* ream target file system; initialise both fs's */
	devinit(from);
	if(to) {
		print("reaming %Z in 8 seconds\n", to);
		delay(8000);
		devream(to, 0);
		devinit(to);
	}

	/* find last valid block in case fworm */
	for (lim = devsize(from); lim != 0; lim--) {
		p = getbuf(from, lim-1, Bread);
		if (p != 0) {
			putbuf(p);
			break;
		}
	}
	if(lim == 0)
		panic("no blocks to copy on %Z", from);
	print("limit %ld\n", lim);

	/* copy written fs blocks from source to target */
	if (to)
		print("copying worm\n");
// devatadebug = 1;
// devataidedebug = 1;
	for (a = 0; a < lim; a++) {
		p = getbuf(from, a, Bread);
		if (p == 0) {
			print("%ld not written\n", a);
			continue;
		}
		if (to != 0 && devwrite(to, p->addr, p->iobuf)) {
			print("out block %ld: write error; bailing", a);
			break;
		}
		putbuf(p);
		if(a % 20000 == 0)
			print("block %ld %T\n", a, time());
	}

	/* sync target, loop */
	print("copied %ld blocks from %Z to %Z\n", a, from, to);
	sync("wormcopy");
	delay(1000);
	print("looping; reset the machine at any time.\n");
	for (; ; )
		continue;		/* await reset */
}

void
getline(char *line)
{
	char *p;
	int c;

	p = line;
	for(;;) {
		c = rawchar(0);
		if(c == 0 || c == '\n') {
			*p = 0;
			return;
		}
		if(c == '\b') {
			p--;
			continue;
		}
		*p++ = c;
	}
}

void
arginit(void)
{
	int verb, c;
	char line[300], word[300], *cp;
	uchar localip[Pasize];
	Filsys *fs;

	if(nvrcheck() == 0){
		print("for config mode hit a key within 5 seconds\n");
		c = rawchar(5);
		if(c == 0) {
			print("	no config\n");
			return;
		}
	}

loop:
	print("config: ");
	getline(line);
	cp = getwd(word, line);
	if(strcmp(word, "end") == 0)
		return;
	if(strcmp(word, "halt") == 0)
		exit();

	if(strcmp(word, "allow") == 0) {
		wstatallow = 1;
		writeallow = 1;
		goto loop;
	}
	if(strcmp(word, "copyworm") == 0) {
		copyworm = 1;
		goto loop;
	}
	if(strcmp(word, "noauth") == 0) {
		noauth = !noauth;
		goto loop;
	}
	if(strcmp(word, "noattach") == 0) {
		noattach = !noattach;
		goto loop;
	}
	if(strcmp(word, "ream") == 0) {
		verb = FREAM;
		goto gfsname;
	}
	if(strcmp(word, "recover") == 0) {
		verb = FRECOVER;
		goto gfsname;
	}
	if(strcmp(word, "filsys") == 0) {
		verb = FEDIT;
		goto gfsname;
	}
	if(strcmp(word, "nvram") == 0) {
		getwd(word, cp);
		if(testconfig(word))
			goto loop;
		if(nvrsetconfig(word) != 0)
			goto loop;
		goto loop;
	}
	if(strcmp(word, "config") == 0) {
		getwd(word, cp);
		if(testconfig(word))
			goto loop;
		if(nvrsetconfig(word) != 0)
			goto loop;
		f.newconf = 1;
		goto loop;
	}
	if(strcmp(word, "service") == 0) {
		getwd(word, cp);
		strcpy(service, word);
		f.modconf = 1;
		goto loop;
	}
	if(strcmp(word, "ipauth") == 0) {
		f.ipauthset = 1;
		verb = 2;
		goto ipname;
	}
	if(astrcmp(word, "ip") == 0) {
		verb = 0;
		goto ipname;
	}
	if(astrcmp(word, "ipgw") == 0) {
		verb = 1;
		goto ipname;
	}
	if(astrcmp(word, "ipmask") == 0) {
		verb = 3;
		goto ipname;
	}
	if(astrcmp(word, "ipsntp") == 0) {
		verb = 4;
		goto ipname;
	}

	print("unknown config command\n");
	print("	type end to get out\n");
	goto loop;

ipname:
	getwd(word, cp);
	if(chartoip(localip, word)) {
		print("bad ip address\n");
		goto loop;
	}
	switch(verb) {
	case 0:
		memmove(ipaddr[aindex].sysip, localip,
			sizeof(ipaddr[aindex].sysip));
		break;
	case 1:
		memmove(ipaddr[aindex].defgwip, localip,
			sizeof(ipaddr[aindex].defgwip));
		break;
	case 2:
		memmove(authip, localip,
			sizeof(authip));
		break;
	case 3:
		memmove(ipaddr[aindex].defmask, localip,
			sizeof(ipaddr[aindex].defmask));
		break;
	case 4:
		memmove(sntpip, localip,
			sizeof(sntpip));
		break;
	}
	f.modconf = 1;
	goto loop;

gfsname:
	cp = getwd(word, cp);
	for(fs=filsys; fs->name; fs++)
		if(strcmp(word, fs->name) == 0)
			goto found;
	memset(fs, 0, sizeof(*fs));
	fs->name = strdup(word);

found:
	switch(verb) {
	case FREAM:
		if(strcmp(fs->name, "main") == 0)
			wstatallow = 1;		/* only set, never reset */
	case FRECOVER:
		fs->flags |= verb;
		goto loop;
	case FEDIT:
		f.modconf = 1;
		getwd(word, cp);
		fs->flags |= verb;
		if(word[0] == 0) {
			fs->conf = 0;
			goto loop;
		}
		if(testconfig(word))
			goto loop;
		fs->conf = strdup(word);
		goto loop;
	}
}
