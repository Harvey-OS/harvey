#include "std.h"
#include "dat.h"

int mainstacksize = 64*1024;
int extrafactotumdir;
int debug;
int doprivate = 1;
int trysecstore = 1;
int kflag;
int sflag;
int uflag;
int askforkeys;
char *factname = "factotum";
char *service /*= "factotum"*/;
char *service;
char *owner;
char *authaddr;
static void private(void);
void gflag(char*);

void
usage(void)
{
	fprint(2, "usage: factotum [-DSdkgpu] [-a authaddr] [-m mtpt] [-s service]\n");
	fprint(2, " or   factotum -g keypattern\n");
	fprint(2, " or   factotum -g 'badkeyattr\\nmsg\\nkeypattern'\n");
	threadexitsall("usage");
}

void
threadmain(int argc, char *argv[])
{
	char *mtpt, *s;
	char *secstorepw;
	char err[ERRMAX];
	Dir d;

	rfork(RFNOTEG);

	mtpt = "/mnt";
	extrafactotumdir = 1;
	secstorepw = nil;
	quotefmtinstall();
	fmtinstall('A', attrfmt);
	fmtinstall('H', encodefmt);
	fmtinstall('N', attrnamefmt);

	if(argc == 3 && strcmp(argv[1], "-g") == 0){
		gflag(argv[2]);
		threadexitsall(nil);
	}

	ARGBEGIN{
	default:
		usage();
	case 'D':
		chatty9p++;
		break;
	case 'S':		/* server: read nvram, no prompting for keys */
		askforkeys = 0;
		trysecstore = 0;
		sflag = 1;
		break;
	case 'a':
		authaddr = EARGF(usage());
		break;
	case 'd':
		debug = 1;
		doprivate = 0;
		break;
	case 'g':
		usage();
	case 'k':		/* reinitialize nvram */
		kflag = 1;
		break;
	case 'm':
		mtpt = EARGF(usage());
		break;
	case 'n':
		trysecstore = 0;
		break;
	case 'p':
		doprivate = 0;
		break;
	case 's':
		service = EARGF(usage());
		break;
	case 'u':		/* user: set hostowner */
		uflag = 1;
		break;
	case 'x':
		extrafactotumdir = 0;
		break;
	}ARGEND

	if(argc != 0)
		usage();
	if(doprivate)
		private();

	initcap();

	if(sflag){
		s = getnvramkey(kflag ? NVwrite : NVwriteonerr, &secstorepw);
		if(s == nil)
			fprint(2, "factotum warning: cannot read nvram: %r\n");
		else if(ctlwrite(s) < 0)
			fprint(2, "factotum warning: cannot add nvram key: %r\n");
		if(secstorepw != nil)
			trysecstore = 1;
		if (s != nil) {
			memset(s, 0, strlen(s));
			free(s);
		}
	} else if(uflag)
		promptforhostowner();
	owner = getuser();

	if(trysecstore && havesecstore()){
		while(secstorefetch(secstorepw) < 0){
			rerrstr(err, sizeof err);
			if(strcmp(err, "cancel") == 0)
				break;
			fprint(2, "secstorefetch: %r\n");
			fprint(2, "Enter an empty password to quit.\n");
			free(secstorepw);
			secstorepw = nil; /* just try nvram pw once */
		}
	}
	
	fsinit0();
	threadpostmountsrv(&fs, service, mtpt, MBEFORE);
	if(service){
		nulldir(&d);
		d.mode = 0666;
		s = emalloc(10+strlen(service));
		strcpy(s, "/srv/");
		strcat(s, service);
		if(dirwstat(s, &d) < 0)
			fprint(2, "factotum warning: cannot chmod 666 %s: %r\n", s);
		free(s);
	}
	threadexits(nil);
}

char *pmsg = "Warning! %s can't protect itself from debugging: %r\n";
char *smsg = "Warning! %s can't turn off swapping: %r\n";

/* don't allow other processes to debug us and steal keys */
static void
private(void)
{
	int fd;
	char buf[64];

	snprint(buf, sizeof(buf), "#p/%d/ctl", getpid());
	fd = open(buf, OWRITE);
	if(fd < 0){
		fprint(2, pmsg, argv0);
		return;
	}
	if(fprint(fd, "private") < 0)
		fprint(2, pmsg, argv0);
	if(fprint(fd, "noswap") < 0)
		fprint(2, smsg, argv0);
	close(fd);
}

/*
 *  prompt user for a key.  don't care about memory leaks, runs standalone
 */
static Attr*
promptforkey(int fd, char *params)
{
	char *def, *v;
	Attr *a, *attr;

	attr = _parseattr(params);
	fprint(fd, "\n!Adding key:");
	for(a=attr; a; a=a->next)
		if(a->type != AttrQuery && a->name[0] != '!')
			fprint(fd, " %q=%q", a->name, a->val);
	fprint(fd, "\n");

	for(a=attr; a; a=a->next){
		v = a->name;
		if(a->type != AttrQuery || v[0]=='!')
			continue;
		def = nil;
		if(strcmp(v, "user") == 0)
			def = getuser();
		a->val = readcons(v, def, 0);
		if(a->val == nil)
			sysfatal("user terminated key input");
		a->type = AttrNameval;
	}
	for(a=attr; a; a=a->next){
		v = a->name;
		if(a->type != AttrQuery || v[0]!='!')
			continue;
		def = nil;
		if(strcmp(v+1, "user") == 0)
			def = getuser();
		a->val = readcons(v+1, def, 1);
		if(a->val == nil)
			sysfatal("user terminated key input");
		a->type = AttrNameval;
	}
	fprint(fd, "!\n");
	return attr;
}

/*
 *  send a key to the mounted factotum
 */
static int
sendkey(Attr *attr)
{
	char buf[8192];
	int rv, fd;
	
	fd = open("/mnt/factotum/ctl", OWRITE);
	if(fd < 0)
		sysfatal("opening factotum/ctl: %r");
	snprint(buf, sizeof buf, "key %A\n", attr);
	rv = write(fd, buf, strlen(buf));
	close(fd);
	return rv;
}

/* askuser */
void
askuser(int fd, char *params)
{
	Attr *attr;

	attr = promptforkey(fd, params);
	if(attr == nil)
		sysfatal("no key supplied");
	if(sendkey(attr) < 0)
		sysfatal("sending key to factotum: %r");
}

void
gflag(char *s)
{
	char *f[4];
	int nf, fd;

	fd = open("/dev/cons", ORDWR);
	if(fd < 0)
		sysfatal("opening /dev/cons: %r");
	nf = getfields(s, f, nelem(f), 0, "\n");
	if(nf == 1){	/* needkey or old badkey */
		askuser(fd, s);
		threadexitsall(nil);
	}
	if(nf == 3){	/* new badkey */
		fprint(fd, "\n");
		fprint(fd, "!replace: %s\n", f[0]);
		fprint(fd, "!because: %s\n", f[1]);
		askuser(fd, f[2]);
		threadexitsall(nil);
	}
	usage();
}
