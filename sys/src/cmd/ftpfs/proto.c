#include <u.h>
#include <libc.h>
#include <bio.h>
#include <ip.h>
#include <auth.h>
#include <fcall.h>
#include <ctype.h>
#include "ftpfs.h"

enum
{
	/* return codes */
	Extra=		1,
	Success=	2,
	Incomplete=	3,
	TempFail=	4,
	PermFail=	5,
	Impossible=	6,
};

Node	*remdir;		/* current directory on remote machine */
Node	*remroot;		/* root directory on remote machine */

int	ctlfd;			/* fd for control connection */
Biobuf	ctlin;			/* input buffer for control connection */
Biobuf	stdin;			/* input buffer for standard input */
Biobuf	dbuf;			/* buffer for data connection */
char	msg[512];		/* buffer for replies */
char	net[NETPATHLEN];	/* network for connections */
int	listenfd;		/* fd to listen on for connections */
char	netdir[NETPATHLEN];
int	os, defos;
char	topsdir[64];		/* name of listed directory for TOPS */
char	remrootpath[256];	/* path on remote side to remote root */
char	user[NAMELEN];
int	nopassive;
long	lastsend;

static void	sendrequest(char*, char*);
static int	getreply(Biobuf*, char*, int, int);
static int	active(int, Biobuf**, char*, char*);
static int	passive(int, Biobuf**, char*, char*);
static int	data(int, Biobuf**, char*, char*);
static int	port(void);
static void	ascii(void);
static void	image(void);
static char*	unixpath(Node*, char*);
static char*	vmspath(Node*, char*);
static char*	mvspath(Node*, char*);
static Node*	vmsdir(char*);
static int	getpassword(char*, char*);

/*
 *  connect to remote server, default network is "tcp/ip"
 */
void
hello(char *dest)
{
	char *p;
	char dir[NETPATHLEN];

	Binit(&stdin, 0, OREAD);	/* init for later use */

	ctlfd = dial(netmkaddr(dest, "tcp", "ftp"), 0, dir, 0);
	if(ctlfd < 0){
		fprint(2, "can't dial %s: %r\n", dest);
		exits("dialing");
	}
	Binit(&ctlin, ctlfd, OREAD);

	/* remember network for the data connections */
	p = strrchr(dir+1, '/');
	if(p == 0)
		fatal("wrong dial(2) linked with ftp");
	*p = 0;
	safecpy(net, dir, sizeof(net));

	/* wait for hello from other side */
	if(getreply(&ctlin, msg, sizeof(msg), 1) != Success)
		fatal("bad hello");
}

/*
 *  login to remote system
 */
void
rlogin(void)
{
	char *line;
	char pass[128];

	strcpy(user, getuser());
	for(;;){
		print("User[default = %s]: ", user);
		line = Brdline(&stdin, '\n');
		if(line == 0)
			exits(0);
		line[Blinelen(&stdin)-1] = 0;
		if(*line){
			strncpy(user, line, sizeof(user));
			user[sizeof(user)-1] = 0;
		}
		sendrequest("USER", user);
		switch(getreply(&ctlin, msg, sizeof(msg), 1)){
		case Success:
			return;
		case Incomplete:
			break;
		case TempFail:
		case PermFail:
			continue;
		}

		if(getpassword(pass, pass+sizeof(pass)) < 0)
			exits(0);
		sendrequest("PASS", pass);
		if(getreply(&ctlin, msg, sizeof(msg), 1) == Success){
			if(strstr(msg, "Sess#"))
				defos = MVS;
			return;
		}
	}
}

/*
 *  login to remote system with given user name and password.
 */
void
clogin(char *cuser, char *cpassword)
{
	strncpy(user, cuser, sizeof(user));
	user[sizeof(user)-1] = 0;
	if (strcmp(user, "anonymous") != 0 &&
		strcmp(user, "ftp") != 0)
		fatal("User must be 'anonymous' or 'ftp'");
	sendrequest("USER", user);
	switch(getreply(&ctlin, msg, sizeof(msg), 1)){
	case Success:
		return;
	case Incomplete:
		break;
	case TempFail:
	case PermFail:
		fatal("login failed");
	}
	if (cpassword == 0)
		fatal("password needed");
	sendrequest("PASS", cpassword);
	if(getreply(&ctlin, msg, sizeof(msg), 1) != Success)
		fatal("password failed");
	if(strstr(msg, "Sess#"))
		defos = MVS;
	return;
}

/*
 *  find out about the other side.  go to it's root if requested.  set
 *  image mode if a Plan9 system.
 */
void
preamble(char *mountroot)
{
	char *p, *ep;
	int rv;
	OS *o;

	/*
	 *  create a root directory mirror
	 */
	remroot = newnode(0, "/");
	remroot->d.qid.path |= CHDIR;
	remroot->d.mode = CHDIR|0777;
	remdir = remroot;

	/*
	 *  get system type
	 */
	sendrequest("SYST", nil);
	switch(getreply(&ctlin, msg, sizeof(msg), 1)){
	case Success:
		for(o = oslist; o->os != Unknown; o++)
			if(strncmp(msg+4, o->name, strlen(o->name)) == 0)
				break;
		os = o->os;
		if(os == NT)
			os = Unix;
		break;
	default:
		os = defos;
		break;
	}
	if(os == Unknown)
		os = defos;

	switch(os){
	case Unix:
	case Plan9:
	case NetWare:
		/*
		 *  go to the remote root, if asked
		 */
		if(mountroot){
			sendrequest("CWD", mountroot);
			getreply(&ctlin, msg, sizeof(msg), 0);
			*remrootpath = 0;
		} else
			sprint(remrootpath, "/usr/%s", user);

		/*
		 *  get the root directory
		 */
		sendrequest("PWD", nil);
		rv = getreply(&ctlin, msg, sizeof(msg), 1);
		if(rv == PermFail){
			sendrequest("XPWD", nil);
			rv = getreply(&ctlin, msg, sizeof(msg), 1);
		}
		if(rv == Success){
			p = strchr(msg, '"');
			if(p){
				p++;
				ep = strchr(p, '"');
				if(ep){
					*ep = 0;
					safecpy(remrootpath, p, sizeof(remrootpath));
				}
			}
		}

		break;
	case Tops:
	case VM:
		/*
		 *  top directory is a figment of our imagination.
		 *  make it permanently cached & valid.
		 */
		CACHED(remroot);
		VALID(remroot);
		remroot->d.atime = time(0) + 100000;

		/*
		 *  no initial directory.  We are in the
		 *  imaginary root.
		 */
		remdir = newtopsdir("???");
		topsdir[0] = 0;
		if(os == Tops && readdir(remdir) >= 0){
			CACHED(remdir);
			if(*topsdir)
				safecpy(remdir->d.name, topsdir, NAMELEN);
			VALID(remdir);
		}
		break;
	case VMS:
		/*
		 *  top directory is a figment of our imagination.
		 *  make it permanently cached & valid.
		 */
		CACHED(remroot);
		VALID(remroot);
		remroot->d.atime = time(0) + 100000;

		/*
		 *  get current directory
		 */
		sendrequest("PWD", nil);
		rv = getreply(&ctlin, msg, sizeof(msg), 1);
		if(rv == PermFail){
			sendrequest("XPWD", nil);
			rv = getreply(&ctlin, msg, sizeof(msg), 1);
		}
		if(rv == Success){
			p = strchr(msg, '"');
			if(p){
				p++;
				ep = strchr(p, '"');
				if(ep){
					*ep = 0;
					remroot = remdir = vmsdir(p);
				}
			}
		}
		break;
	case MVS:
		usenlst = 1;
		*remrootpath = 0;
		break;
	}

	if(os == Plan9)
		image();
}

static void
ascii(void)
{
	sendrequest("TYPE A", nil);
	switch(getreply(&ctlin, msg, sizeof(msg), 0)){
	case Success:
		break;
	default:
		fatal("can't set type to ascii");
	}
}

static void
image(void)
{
	sendrequest("TYPE I", nil);
	switch(getreply(&ctlin, msg, sizeof(msg), 0)){
	case Success:
		break;
	default:
		fatal("can't set type to image/binary");
	}
}

/*
 *  decode the time fields, return seconds since epoch began
 */
char *monthchars = "janfebmaraprmayjunjulaugsepoctnovdec";
static Tm now;

static ulong
cracktime(char *month, char *day, char *yr, char *hms)
{
	Tm tm;
	int i;
	char *p;

	/* default time */
	if(now.year == 0)
		now = *localtime(time(0));
	tm = now;

	/* convert ascii month to a number twixt 1 and 12 */
	if(*month >= '0' && *month <= '9'){
		tm.mon = atoi(month) - 1;
		if(tm.mon < 0 || tm.mon > 11)
			tm.mon = 5;
	} else {
		for(p = month; *p; p++)
			*p = tolower(*p);
		for(i = 0; i < 12; i++)
			if(strncmp(&monthchars[i*3], month, 3) == 0){
				tm.mon = i;
				break;
			}
	}

	tm.mday = atoi(day);

	if(hms){
		tm.hour = strtol(hms, &p, 0);
		if(*p == ':'){
			tm.min = strtol(p+1, &p, 0);
			if(*p == ':')
				tm.sec = strtol(p+1, &p, 0);
		}
		if(tolower(*p) == 'p')
			tm.hour += 12;
	}

	if(yr){
		tm.year = atoi(yr);
		if(tm.year >= 1900)
			tm.year -= 1900;
	} else {
		if(tm.mon > now.mon || (tm.mon == now.mon && tm.mday > now.mday+1))
			tm.year--;
	}

	/* convert to epoch seconds */
	return tm2sec(&tm);
}

/*
 *  decode a Unix or Plan 9 file mode
 */
static ulong
crackmode(char *p)
{
	ulong flags;
	ulong mode;
	int i;

	flags = 0;
	switch(strlen(p)){
	case 10:	/* unix and new style plan 9 */
		switch(*p){
		case 'l':
			return CHSYML|0777;
		case 'd':
			flags |= CHDIR;
		case 'a':
			flags |= CHAPPEND;
		}
		p++;
		if(p[2] == 'l')
			flags |= CHEXCL;
		break;
	case 11:	/* old style plan 9 */
		switch(*p++){
		case 'd':
			flags |= CHDIR;
			break;
		case 'a':
			flags |= CHAPPEND;
			break;
		}
		if(*p++ == 'l')
			flags |= CHEXCL;
		break;
	default:
		return CHDIR|0777;
	}
	mode = 0;
	for(i = 0; i < 3; i++){
		mode <<= 3;
		if(*p++ == 'r')
			mode |= CHREAD;
		if(*p++ == 'w')
			mode |= CHWRITE;
		if(*p == 'x' || *p == 's' || *p == 'S')
			mode |= CHEXEC;
		p++;
	}
	return mode | flags;
}

/*
 *  find first punctuation
 */
char*
strpunct(char *p)
{
	int c;

	for(;c = *p; p++){
		if(ispunct(c))
			return p;
	}
	return 0;
}

/* convert a single cha
int
latin1toutf(char *out, char *in, char *e)
{
	Rune r;
	char *p;

	p = out;
	for(; in < e; in++){
		r = (*in) & 0xff;
		p += runetochar(p, &r);
	}
	*p = 0;
	return p - out;
}

/*
 *  convert from latin1 to utf
 */
static char*
fromlatin1(char *from, char *to, int len)
{
	char *p, *e;
	Rune r;

	e = to + len - 4;
	for(p = to; *from && p < e; from++){
		r = (*from) & 0xff;
		p += runetochar(p, &r);
	}
	*p = 0;

	return to;
}

/*
 *  shorten a symbol to NAMELEN bytes
 */
static void
shorten(char *from, char *to, int offset)
{
	int n, s;
	char *p;
	char tmp[512];
	char utf[1024];

	memset(to, 0, NAMELEN);

	/*
	 *  if it contains latin-1, convert to utf
	 */
	from = fromlatin1(from, utf, sizeof(utf));

	/*
	 *  if it fits, keep it
	 */
	n = strlen(from);
	if(n < NAMELEN){
		strcpy(to, from);
		return;
	}

	/*
	 *  quick and stupid hacks
	 */
	if(offset == 0){
		if(strncmp(from, "draft-", 6) == 0){
			from += 6;
			n -= 6;
		}
		if(strncmp(from+n-4, ".txt", 4) == 0){
			strncpy(tmp, from, n-4);
			tmp[n-4] = 0;
			from = tmp;
		}
		n = strlen(from);
		if(n < NAMELEN){
			strcpy(to, from);
			return;
		}
	}

	/*
	 *  try to keep extensions
	 */
	p = strpunct(from);
	while(p){
		if(strlen(p) < NAMELEN/3)
			break;
		p = strpunct(p+1);
	}
	if(p == 0)
		p = from + n;

	p -= offset;
	if(p < from)
		p = from;
	s = strlen(p);
	if(s >= NAMELEN - 1)
		s = NAMELEN - 2;
	n = NAMELEN - 2 - s;
	if(n > 0)
		memmove(to, from, n);
	to[n] = '*';
	if(s > 0)
		memmove(to + n + 1, p, s);
}

/*
 *  make shortened names unique
 */
static int
mkunique(Node *parent, int off)
{
	Node *np, *nnp;
	int change;

	change = 0;
	for(np = parent->children; np; np = np->sibs){
		for(nnp = np->sibs; nnp; nnp = nnp->sibs){
			if(strcmp(np->d.name, nnp->d.name))
				continue;
			shorten(nnp->longname, nnp->d.name, off);
			change = 1;
		}
	}
	return change;
}

/*
 *  decode a Unix or Plan 9 directory listing
 */
static char*
crackdir(char *p, Dir *dp)
{
	char *field[15];
	char *dfield[4];
	char *cp;
	static char longname[128];
	int dn, n;

	n = getfields(p, field, 15, 1, " \t");
	if(n > 2 && strcmp(field[n-2], "->") == 0)
		n -= 2;
	switch(os){
	case TSO:
		cp = strchr(field[0], '.');
		if(cp){
			*cp++ = 0;
			safecpy(longname, cp, sizeof(longname));
			safecpy(dp->uid, field[0], NAMELEN);
		} else {
			safecpy(longname, field[0], sizeof(longname));
			safecpy(dp->uid, "TSO", NAMELEN);
		}
		safecpy(dp->gid, "TSO", NAMELEN);
		dp->mode = 0666;
		dp->length = 0;
		dp->atime = 0;
		break;
	case OS½:
		safecpy(longname, field[n-1], sizeof(longname));
		safecpy(dp->uid, "OS½", NAMELEN);
		safecpy(dp->gid, "OS½", NAMELEN);
		dp->mode = 0666;
		switch(n){
		case 5:
			if(strcmp(field[1], "DIR") == 0)
				dp->mode |= CHDIR;
			dp->length = atoi(field[0]);
			dn = getfields(field[2], dfield, 4, 1, "-");
			if(dn == 3)
				dp->atime = cracktime(dfield[0], dfield[1], dfield[2], field[3]);
			break;
		}
		break;
	case Tops:
		if(n != 4){ /* tops directory name */
			safecpy(topsdir, field[0], sizeof(topsdir));
			return 0;
		}
		safecpy(longname, field[3], sizeof(longname));
		dp->length = atoi(field[0]);
		dp->mode = 0666;
		strcpy(dp->uid, "Tops");
		strcpy(dp->gid, "Tops");
		dn = getfields(field[1], dfield, 4, 1, "-");
		if(dn == 3)
			dp->atime = cracktime(dfield[1], dfield[0], dfield[2], field[2]);
		else
			dp->atime = time(0);
		break;
	case VM:
		switch(n){
		case 9:
			sprint(longname, "%s.%s", field[0], field[1]);
			dp->length = atoi(field[3])*atoi(field[4]);
			if(*field[2] == 'F')
				dp->mode = 0666;
			else
				dp->mode = 0777;
			strcpy(dp->uid, "VM");
			strcpy(dp->gid, "VM");
			dn = getfields(field[6], dfield, 4, 1, "/-");
			if(dn == 3)
				dp->atime = cracktime(dfield[0], dfield[1], dfield[2], field[7]);
			else
				dp->atime = time(0);
			break;
		case 1:
			safecpy(longname, field[0], sizeof(longname));
			strcpy(dp->uid, "VM");
			strcpy(dp->gid, "VM");
			dp->mode = 0777;
			dp->atime = 0;
			break;
		default:
			return 0;
		}
		break;
	case VMS:
		switch(n){
		case 6:
			for(cp = field[0]; *cp; cp++)
				*cp = tolower(*cp);
			cp = strchr(field[0], ';');
			if(cp)
				*cp = 0;
			dp->mode = 0666;
			cp = field[0] + strlen(field[0]) - 4;
			if(strcmp(cp, ".dir") == 0){
				dp->mode |= CHDIR;
				*cp = 0;
			}
			safecpy(longname, field[0], sizeof(longname));
			dp->length = atoi(field[1])*512;
			field[4][strlen(field[4])-1] = 0;
			safecpy(dp->uid, field[4]+1, sizeof(dp->uid));
			safecpy(dp->gid, field[4]+1, sizeof(dp->gid));
			dn = getfields(field[2], dfield, 4, 1, "/-");
			if(dn == 3)
				dp->atime = cracktime(dfield[1], dfield[0], dfield[2], field[3]);
			else
				dp->atime = time(0);
			break;
		default:
			return 0;
		}
		break;
	case NetWare:
		switch(n){
		case 9:
			safecpy(longname, field[8], sizeof(longname));
			safecpy(dp->uid, field[2], NAMELEN);
			safecpy(dp->gid, field[2], NAMELEN);
			dp->mode = 0666;
			if(*field[0] == 'd')
				dp->mode |= CHDIR;
			dp->length = atoi(field[3]);
			dp->atime = cracktime(field[4], field[5], field[6], field[7]);
			break;
		case 1:
			safecpy(longname, field[0], sizeof(longname));
			strcpy(dp->uid, "none");
			strcpy(dp->gid, "none");
			dp->mode = 0777;
			dp->atime = 0;
			break;
		default:
			return 0;
		}
		break;
	case Unix:
	case Plan9:
	default:
		switch(n){
		case 8:		/* ls -l */
			safecpy(longname, field[7], sizeof(longname));
			safecpy(dp->uid, field[2], NAMELEN);
			safecpy(dp->gid, field[2], NAMELEN);
			dp->mode = crackmode(field[0]);
			dp->length = atoi(field[3]);
			if(strchr(field[6], ':'))
				dp->atime = cracktime(field[4], field[5], 0, field[6]);
			else
				dp->atime = cracktime(field[4], field[5], field[6], 0);
			break;
		case 9:		/* ls -lg */
			safecpy(longname, field[8], sizeof(longname));
			safecpy(dp->uid, field[2], NAMELEN);
			safecpy(dp->gid, field[3], NAMELEN);
			dp->mode = crackmode(field[0]);
			dp->length = atoi(field[4]);
			if(strchr(field[7], ':'))
				dp->atime = cracktime(field[5], field[6], 0, field[7]);
			else
				dp->atime = cracktime(field[5], field[6], field[7], 0);
			break;
		case 10:	/* plan 9 */
			safecpy(longname, field[9], sizeof(longname));
			safecpy(dp->uid, field[3], NAMELEN);
			safecpy(dp->gid, field[4], NAMELEN);
			dp->mode = crackmode(field[0]);
			dp->length = atoi(field[5]);
			if(strchr(field[8], ':'))
				dp->atime = cracktime(field[6], field[7], 0, field[8]);
			else
				dp->atime = cracktime(field[6], field[7], field[8], 0);
			break;
		case 4:		/* a Windows_NT version */
			safecpy(longname, field[3], sizeof(longname));
			strcpy(dp->uid, "NT");
			strcpy(dp->gid, "NT");
			if(strcmp("<DIR>", field[2]) == 0){
				dp->length = 0;
				dp->mode = CHDIR|0777;
			} else {
				dp->mode = 0666;
				dp->length = atoi(field[2]);
			}
			dn = getfields(field[0], dfield, 4, 1, "/-");
			if(dn == 3)
				dp->atime = cracktime(dfield[0], dfield[1], dfield[2], field[1]);
			break;
		case 1:
			safecpy(longname, field[0], sizeof(longname));
			strcpy(dp->uid, "none");
			strcpy(dp->gid, "none");
			dp->mode = 0777;
			dp->atime = 0;
			break;
		default:
			return 0;
		}
	}
	if(strcmp(longname, ".") == 0 || strcmp(longname, "..") == 0)
		return 0;
	dp->qid.path = dp->mode & CHDIR;
	dp->mtime = dp->atime;
	if(ext && (dp->mode & CHDIR) == 0){
		if(sizeof(longname) - strlen(longname) > strlen(ext))
			strcat(longname, ext);
	}
	shorten(longname, dp->name, 0);
	return longname;
}

/*
 *  probe files in a directory to see if they are directories
 */
/*
 *  read a remote directory
 */
int
readdir(Node *node)
{
	Biobuf *bp;
	char *line, *longname;
	Node *np;
	Dir d;
	long n;
	int i, tries, x, files;
	static int uselist;
	int usenlist;

	if(changedir(node) < 0)
		return -1;

	usenlist = 0;
	for(tries = 0; tries < 3; tries++){
		if(usenlist || usenlst)
			x = data(OREAD, &bp, "NLST", nil);
		else if(os == Unix && !uselist)
			x = data(OREAD, &bp, "LIST -l", nil);
		else
			x = data(OREAD, &bp, "LIST", nil);
		switch(x){
		case Extra:
			break;
/*		case TempFail:
			continue;
*/
		default:
			if(os == Unix && uselist == 0){
				uselist = 1;
				continue;
			}
			return seterr(nosuchfile);
		}
		files = 0;
		while(line = Brdline(bp, '\n')){
			n = Blinelen(bp);
			if(debug)
				write(2, line, n);
			if(n > 1 && line[n-2] == '\r')
				n--;
			line[n - 1] = 0;
	
			if((longname = crackdir(line, &d)) == 0)
				continue;
			files++;
			np = extendpath(node, longname);
			d.qid = np->d.qid;
			d.qid.path |= d.mode & CHDIR;
			d.type = np->d.type;
			d.dev = 1;			/* mark node as valid */
			if(os == MVS && node == remroot){
				d.qid.path |= CHDIR;
				d.mode |= CHDIR;
			}
			np->d = d;
		}
		close(Bfildes(bp));
		for(i = 0; i < NAMELEN-5; i++)
			if(mkunique(node, i) == 0)
				break;

		switch(getreply(&ctlin, msg, sizeof(msg), 0)){
		case Success:
			if(files == 0 && !usenlst && !usenlist){
				usenlist = 1;
				continue;
			}
			if(files && usenlist)
				usenlst = 1;
			if(usenlst)
				node->chdirunknown = 1;
			return 0;
		case TempFail:
			break;
		default:
			return seterr(nosuchfile);
		}
	}
	return seterr(nosuchfile);
}

/*
 *  create a remote directory
 */
int
createdir(Node *node)
{
	if(changedir(node->parent) < 0)
		return -1;
	
	sendrequest("MKD", node->d.name);
	if(getreply(&ctlin, msg, sizeof(msg), 0) != Success)
		return -1;
	return 0;
}

/*
 *  change to a remote directory.
 */
int
changedir(Node *node)
{
	Node *to;
	char cdpath[256];

	to = node;
	if(to == remdir)
		return 0;

	/* build an absolute path */
	switch(os){
	case Tops:
	case VM:
		switch(node->depth){
		case 0:
			remdir = node;
			return 0;
		case 1:
			strcpy(cdpath, node->longname);
			break;
		default:
			return seterr(nosuchfile);
		}
		break;
	case VMS:
		switch(node->depth){
		case 0:
			remdir = node;
			return 0;
		default:
			vmspath(node, cdpath);
		}
		break;
	case MVS:
		if(node->depth == 0)
			strcpy(cdpath, remrootpath);
		else
			mvspath(node, cdpath);
	default:
		if(node->depth == 0)
			strcpy(cdpath, remrootpath);
		else
			unixpath(node, cdpath);
		break;
	}

	uncachedir(remdir, 0);

	/*
	 *  connect, if we need a password (Incomplete)
	 *  act like it worked (best we can do).
	 */
	sendrequest("CWD", cdpath);
	switch(getreply(&ctlin, msg, sizeof(msg), 0)){
	case Success:
	case Incomplete:
		remdir = node;
		return 0;
	default:
		return seterr(nosuchfile);
	}
}

/*
 *  read a remote file
 */
int
readfile1(Node *node)
{
	Biobuf *bp;
	char buf[4*1024];
	long off;
	int n;
	int tries;
extern char errstring[ERRLEN];

	if(changedir(node->parent) < 0)
		return -1;

	for(tries = 0; tries < 4; tries++){
		switch(data(OREAD, &bp, "RETR", node->longname)){
		case Extra:
			break;
		case TempFail:
			continue;
		default:
			return seterr(nosuchfile);
		}
		off = 0;
		while((n = read(Bfildes(bp), buf, sizeof buf)) > 0){
			if(filewrite(node, buf, off, n) != n){
				off = -1;
				break;
			}
			off += n;
		}
		if(off < 0)
			return -1;

		/* make sure a file gets created even for a zero length file */
		if(off == 0)
			filewrite(node, buf, 0, 0);

		close(Bfildes(bp));
		switch(getreply(&ctlin, msg, sizeof(msg), 0)){
		case Success:
			return off;
		case TempFail:
			continue;
		default:
			return seterr(nosuchfile);
		}
	}
	return seterr(nosuchfile);
}

int
readfile(Node *node)
{
	int rv, inimage;

	switch(os){
	case MVS:
	case Plan9:
	case Tops:
	case TSO:
		inimage = 0;
		break;
	default:
		inimage = 1;
		image();
		break;
	}

	rv = readfile1(node);

	if(inimage)
		ascii();
	return rv;
}

/*
 *  write back a file
 */
int
createfile1(Node *node)
{
	Biobuf *bp;
	char buf[4*1024];
	long off;
	int n;

	if(changedir(node->parent) < 0)
		return -1;

	if(data(OWRITE, &bp, "STOR", node->longname) != Extra)
		return -1;
	for(off = 0; ; off += n){
		n = fileread(node, buf, off, sizeof(buf));
		if(n <= 0)
			break;
		write(Bfildes(bp), buf, n);
	}
	close(Bfildes(bp));
	getreply(&ctlin, msg, sizeof(msg), 0);
	return off;
}

int
createfile(Node *node)
{
	int rv;

	switch(os){
	case Plan9:
	case Tops:
		break;
	default:
		image();
		break;
	}
	rv = createfile1(node);
	switch(os){
	case Plan9:
	case Tops:
		break;
	default:
		ascii();
		break;
	}
	return rv;
}

/*
 *  remove a remote file
 */
int
removefile(Node *node)
{
	if(changedir(node->parent) < 0)
		return -1;
	
	sendrequest("DELE", node->d.name);
	if(getreply(&ctlin, msg, sizeof(msg), 0) != Success)
		return -1;
	return 0;
}

/*
 *  remove a remote directory
 */
int
removedir(Node *node)
{
	if(changedir(node->parent) < 0)
		return -1;
	
	sendrequest("RMD", node->d.name);
	if(getreply(&ctlin, msg, sizeof(msg), 0) != Success)
		return -1;
	return 0;
}

/*
 *  tell remote that we're exiting and then do it
 */
void
quit(void)
{
	sendrequest("QUIT", nil);
	getreply(&ctlin, msg, sizeof(msg), 0);
	exits(0);
}

/*
 *  send a request
 */
static void
sendrequest(char *a, char *b)
{
	char buf[2*1024];
	int n;

	n = strlen(a)+2+1;
	if(b != nil)
		n += strlen(b)+1;
	if(n >= sizeof(buf))
		fatal("proto request too long");
	strcpy(buf, a);
	if(b != nil){
		strcat(buf, " ");
		strcat(buf, b);
	}
	strcat(buf, "\r\n");
	n = strlen(buf);
	if(write(ctlfd, buf, n) != n)
		fatal("remote side hung up");
	if(debug)
		write(2, buf, n);
	lastsend = time(0);
}

/*
 *  replies codes are in the range [100, 999] and may contain multiple lines of
 *  continuation.
 */
static int
getreply(Biobuf *bp, char *msg, int len, int printreply)
{
	char *line;
	int rv;
	int i, n;

	while(line = Brdline(bp, '\n')){
		/* add line to message buffer, strip off \r */
		n = Blinelen(bp);
		if(n > 1 && line[n-2] == '\r'){
			n--;
			line[n-1] = '\n';
		}
		if(printreply && !quiet)
			write(1, line, n);
		else if(debug)
			write(2, line, n);
		if(n > len - 1)
			i = len - 1;
		else
			i = n;
		if(i > 0){
			memmove(msg, line, i);
			msg += i;
			len -= i;
			*msg = 0;
		}

		/* stop if not a continuation */
		rv = atoi(line);
		if(rv >= 100 && rv < 600 && (n == 4 || (n > 4 && line[3] == ' ')))
			return rv/100;

		/* tell user about continuations */
		if(!debug && !quiet && !printreply)
			write(2, line, n);
	}

	fatal("remote side closed connection");
	return 0;
}

/*
 *  Announce on a local port and tell its address to the the remote side
 */
static int
port(void)
{
	char buf[256];
	int n, fd;
	char *ptr;
	uchar ipaddr[IPaddrlen];
	int port;

	/* get a channel to listen on, let kernel pick the port number */
	sprint(buf, "%s!*!0", net);
	listenfd = announce(buf, netdir);
	if(listenfd < 0)
		return seterr("can't announce");

	/* get the local address and port number */
	sprint(buf, "%s/local", netdir);
	fd = open(buf, OREAD);
	if(fd < 0)
		return seterr("opening %s: %r", buf);
	n = read(fd, buf, sizeof(buf)-1);
	close(fd);
	if(n <= 0)
		return seterr("opening %s/local: %r", netdir);
	buf[n] = 0;
	ptr = strchr(buf, ' ');
	if(ptr)
		*ptr = 0;
	ptr = strchr(buf, '!')+1;
	parseip(ipaddr, buf);
	port = atoi(ptr);

	/* tell remote side */
	sprint(buf, "PORT %d,%d,%d,%d,%d,%d", ipaddr[IPv4off+0], ipaddr[IPv4off+1],
		ipaddr[IPv4off+2], ipaddr[IPv4off+3], port>>8, port&0xff);
	sendrequest(buf, nil);
	if(getreply(&ctlin, msg, sizeof(msg), 0) != Success)
		return seterr(msg);
	return 0;
}

/*
 *  have server call back for a data connection
 */
static int
active(int mode, Biobuf **bpp, char *cmda, char *cmdb)
{
	int cfd, dfd, rv;
	char newdir[NETPATHLEN];
	char datafile[NETPATHLEN + 6];

	if(port() < 0)
		return TempFail;

	sendrequest(cmda, cmdb);

	rv = getreply(&ctlin, msg, sizeof(msg), 0);
	if(rv != Extra){
		close(listenfd);
		return rv;
	}

	/* wait for a new call */
	cfd = listen(netdir, newdir);
	if(cfd < 0)
		fatal("waiting for data connection");
	close(listenfd);

	/* open it's data connection and close the control connection */
	sprint(datafile, "%s/data", newdir);
	dfd = open(datafile, ORDWR);
	close(cfd);
	if(dfd < 0)
		fatal("opening data connection");
	Binit(&dbuf, dfd, mode);
	*bpp = &dbuf;
	return Extra;
}

/*
 *  call out for a data connection
 */
static int
passive(int mode, Biobuf **bpp, char *cmda, char *cmdb)
{
	char msg[1024];
	char *f[6];
	char *p;
	int x, fd;

	if(nopassive)
		return Impossible;

	sendrequest("PASV", nil);
	if(getreply(&ctlin, msg, sizeof(msg), 0) != Success){
		nopassive = 1;
		return Impossible;
	}

	/* get address and port number from reply, this is AI */
	p = strchr(msg, '(');
	if(p == 0){
		for(p = msg+3; *p; p++)
			if(isdigit(*p))
				break;
	} else
		p++;
	if(getfields(p, f, 6, 0, ",") < 6){
		if(debug)
			fprint(2, "passive mode protocol botch: %s\n", msg);
		werrstr("ftp protocol botch");
		nopassive = 1;
		return Impossible;
	}
	snprint(msg, sizeof(msg), "%s!%s.%s.%s.%s!%d", net,
		f[0], f[1], f[2], f[3],
		((atoi(f[4])&0xff)<<8) + (atoi(f[5])&0xff));

	/* open data connection */
	fd = dial(msg, 0, 0, 0);
	if(fd < 0){
		if(debug)
			fprint(2, "passive mode connect to %s failed: %r\n", msg);
		nopassive = 1;
		return TempFail;
	}

	/* tell remote to send a file */
	sendrequest(cmda, cmdb);
	x = getreply(&ctlin, msg, sizeof(msg), 0);
	if(x != Extra){
		close(fd);
		if(debug)
			fprint(2, "passive mode retrieve failed: %s\n", msg);
		werrstr(msg);
		return x;
	}

	Binit(&dbuf, fd, mode);
	*bpp = &dbuf;
	return Extra;
}

static int
data(int mode, Biobuf **bpp, char* cmda, char *cmdb)
{
	int x;

	x = passive(mode, bpp, cmda, cmdb);
	if(x != Impossible)
		return x;
	return active(mode, bpp, cmda, cmdb);
}

/*
 *  used for keep alives
 */
void
nop(void)
{
	if(lastsend - time(0) < 15)
		return;
	sendrequest("PWD", nil);
	getreply(&ctlin, msg, sizeof(msg), 0);
}

/*
 *  turn a vms spec into a path
 */
static Node*
vmsextendpath(Node *np, char *name)
{
	np = extendpath(np, name);
	if(!ISVALID(np)){
		np->d.qid.path |= CHDIR;
		np->d.atime = time(0);
		np->d.mtime = np->d.atime;
		strcpy(np->d.uid, "who");
		strcpy(np->d.gid, "cares");
		np->d.mode = CHDIR|0777;
		np->d.length = 0;
		if(changedir(np) >= 0)
			VALID(np);
	}
	return np;
}
static Node*
vmsdir(char *name)
{
	char *cp;
	Node *np;
	char *oname;

	np = remroot;
	cp = strchr(name, '[');
	if(cp)
		strcpy(cp, cp+1);
	cp = strchr(name, ']');
	if(cp)
		*cp = 0;
	oname = name = strdup(name);
	if(name == 0)
		return 0;

	while(cp = strchr(name, '.')){
		*cp = 0;
		np = vmsextendpath(np, name);
		name = cp+1;
	}
	np = vmsextendpath(np, name);

	/*
	 *  walk back to first accessible directory
	 */
	for(; np->parent != np; np = np->parent)
		if(ISVALID(np)){
			CACHED(np->parent);
			break;
		}

	free(oname);
	return np;
}

/*
 *  walk up the tree building a VMS style path
 */
static char*
vmspath(Node *node, char *path)
{
	char *p;
	int n;

	if(node->depth == 1){
		p = strchr(node->longname, ':');
		if(p){
			n = p - node->longname + 1;
			strncpy(path, node->longname, n);
			path[n] = '[';
			path[n+1] = 0;
			strcat(path, p+1);
		} else {
			strcpy(path, "[");
			strcat(path, node->longname);
		}
		strcat(path, "]");
		return path + strlen(path) - 1;
	}
	p = vmspath(node->parent, path);
	*p++ = '.';
	strcpy(p, node->longname);
	strcat(path, "]");
	return p + strlen(node->longname);
}

/*
 *  walk up the tree building a Unix style path
 */
static char*
unixpath(Node *node, char *path)
{
	char *p;

	if(node == node->parent){
		strcpy(path, remrootpath);
		return path + strlen(remrootpath);
	}
	p = unixpath(node->parent, path);
	if(p > path && *(p-1) != '/')
		*p++ = '/';
	strcpy(p, node->longname);
	return p + strlen(node->longname);
}

/*
 *  walk up the tree building a MVS style path
 */
static char*
mvspath(Node *node, char *path)
{
	char *p;

	if(node == node->parent){
		strcpy(path, remrootpath);
		return path + strlen(remrootpath);
	}
	p = mvspath(node->parent, path);
	*p++ = '.';
	strcpy(p, node->longname);
	return p + strlen(node->longname);
}

static int
getpassword(char *buf, char *e)
{
	char *p;
	int c;
	int consctl, rv = 0;

	consctl = open("/dev/consctl", OWRITE);
	if(consctl >= 0)
		write(consctl, "rawon", 5);
	print("Password: ");
	e--;
	for(p = buf; p <= e; p++){
		c = Bgetc(&stdin);
		if(c < 0){
			rv = -1;
			goto out;
		}
		if(c == '\n' || c == '\r')
			break;
		*p = c;
	}
	*p = 0;
	print("\n");

out:
	if(consctl >= 0)
		close(consctl);
	return rv;
}
