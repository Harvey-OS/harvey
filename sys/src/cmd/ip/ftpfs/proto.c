#include <u.h>
#include <libc.h>
#include <bio.h>
#include <ip.h>
#include <mp.h>
#include <libsec.h>
#include <auth.h>
#include <fcall.h>
#include <ctype.h>
#include <String.h>
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
char	net[Maxpath];		/* network for connections */
int	listenfd;		/* fd to listen on for connections */
char	netdir[Maxpath];
int	os, defos;
char	topsdir[64];		/* name of listed directory for TOPS */
String	*remrootpath;	/* path on remote side to remote root */
char	*user;
int	nopassive;
long	lastsend;
extern int usetls;

static void	sendrequest(char*, char*);
static int	getreply(Biobuf*, char*, int, int);
static int	active(int, Biobuf**, char*, char*);
static int	passive(int, Biobuf**, char*, char*);
static int	data(int, Biobuf**, char*, char*);
static int	port(void);
static void	ascii(void);
static void	image(void);
static void	unixpath(Node*, String*);
static void	vmspath(Node*, String*);
static void	mvspath(Node*, String*);
static Node*	vmsdir(char*);
static int	getpassword(char*, char*);
static int	nw_mode(char dirlet, char *s);

/*
 *  connect to remote server, default network is "tcp/ip"
 */
void
hello(char *dest)
{
	char *p;
	char dir[Maxpath];
	TLSconn conn;

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
	if(strstr(msg, "Plan 9"))
		os = Plan9;

	if(usetls){
		sendrequest("AUTH", "TLS");
		if(getreply(&ctlin, msg, sizeof(msg), 1) != Success)
			fatal("bad auth tls");

		ctlfd = tlsClient(ctlfd, &conn);
		if(ctlfd < 0)
			fatal("starting tls: %r");
		free(conn.cert);

		Binit(&ctlin, ctlfd, OREAD);

		sendrequest("PBSZ", "0");
		if(getreply(&ctlin, msg, sizeof(msg), 1) != Success)
			fatal("bad pbsz 0");
		sendrequest("PROT", "P");
		if(getreply(&ctlin, msg, sizeof(msg), 1) != Success)
			fatal("bad prot p");
	}
}

/*
 *  login to remote system
 */
void
rlogin(char *rsys, char *keyspec)
{
	char *line;
	char pass[128];
	UserPasswd *up;

	up = nil;
	for(;;){
		if(up == nil && os != Plan9)
			up = auth_getuserpasswd(auth_getkey, "proto=pass server=%s service=ftp %s", rsys, keyspec);
		if(up != nil){
			sendrequest("USER", up->user);
		} else {
			print("User[default = %s]: ", user);
			line = Brdline(&stdin, '\n');
			if(line == 0)
				exits(0);
			line[Blinelen(&stdin)-1] = 0;
			if(*line){
				free(user);
				user = strdup(line);
			}
			sendrequest("USER", user);
		}
		switch(getreply(&ctlin, msg, sizeof(msg), 1)){
		case Success:
			goto out;
		case Incomplete:
			break;
		case TempFail:
		case PermFail:
			continue;
		}

		if(up != nil){
			sendrequest("PASS", up->passwd);
		} else {
			if(getpassword(pass, pass+sizeof(pass)) < 0)
				exits(0);
			sendrequest("PASS", pass);
		}
		if(getreply(&ctlin, msg, sizeof(msg), 1) == Success){
			if(strstr(msg, "Sess#"))
				defos = MVS;
			break;
		}
	}
out:
	if(up != nil){
		memset(up, 0, sizeof(*up));
		free(up);
	}
}

/*
 *  login to remote system with given user name and password.
 */
void
clogin(char *cuser, char *cpassword)
{
	free(user);
	user = strdup(cuser);
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
	remroot = newnode(0, s_copy("/"));
	remroot->d->qid.type = QTDIR;
	remroot->d->mode = DMDIR|0777;
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

	remrootpath = s_reset(remrootpath);
	switch(os){
	case NetWare:
              /*
               * Request long, rather than 8.3 filenames,
               * where the Servers & Volume support them.
               */
              sendrequest("SITE LONG", nil);
              getreply(&ctlin, msg, sizeof(msg), 0);
              /* FALL THRU */
	case Unix:
	case Plan9:
		/*
		 *  go to the remote root, if asked
		 */
		if(mountroot){
			sendrequest("CWD", mountroot);
			getreply(&ctlin, msg, sizeof(msg), 0);
		} else {
			s_append(remrootpath, "/usr/");
			s_append(remrootpath, user);
		}

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
					s_append(s_reset(remrootpath), p);
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
		remroot->d->atime = time(0) + 100000;

		/*
		 *  no initial directory.  We are in the
		 *  imaginary root.
		 */
		remdir = newtopsdir("???");
		topsdir[0] = 0;
		if(os == Tops && readdir(remdir) >= 0){
			CACHED(remdir);
			if(*topsdir)
				remdir->remname = s_copy(topsdir);
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
		remroot->d->atime = time(0) + 100000;

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
	tm.yday = 0;

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
			return DMSYML|0777;
		case 'd':
			flags |= DMDIR;
		case 'a':
			flags |= DMAPPEND;
		}
		p++;
		if(p[2] == 'l')
			flags |= DMEXCL;
		break;
	case 11:	/* old style plan 9 */
		switch(*p++){
		case 'd':
			flags |= DMDIR;
			break;
		case 'a':
			flags |= DMAPPEND;
			break;
		}
		if(*p++ == 'l')
			flags |= DMEXCL;
		break;
	default:
		return DMDIR|0777;
	}
	mode = 0;
	for(i = 0; i < 3; i++){
		mode <<= 3;
		if(*p++ == 'r')
			mode |= DMREAD;
		if(*p++ == 'w')
			mode |= DMWRITE;
		if(*p == 'x' || *p == 's' || *p == 'S')
			mode |= DMEXEC;
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

/*
 *  decode a Unix or Plan 9 directory listing
 */
static Dir*
crackdir(char *p, String **remname)
{
	char *field[15];
	char *dfield[4];
	char *cp;
	String *s;
	int dn, n;
	Dir d, *dp;

	memset(&d, 0, sizeof(d));

	n = getfields(p, field, 15, 1, " \t");
	if(n > 2 && strcmp(field[n-2], "->") == 0)
		n -= 2;
	switch(os){
	case TSO:
		cp = strchr(field[0], '.');
		if(cp){
			*cp++ = 0;
			s = s_copy(cp);
			d.uid = field[0];
		} else {
			s = s_copy(field[0]);
			d.uid = "TSO";
		}
		d.gid = "TSO";
		d.mode = 0666;
		d.length = 0;
		d.atime = 0;
		break;
	case OS½:
		s = s_copy(field[n-1]);
		d.uid = "OS½";
		d.gid = d.uid;
		d.mode = 0666;
		switch(n){
		case 5:
			if(strcmp(field[1], "DIR") == 0)
				d.mode |= DMDIR;
			d.length = atoll(field[0]);
			dn = getfields(field[2], dfield, 4, 1, "-");
			if(dn == 3)
				d.atime = cracktime(dfield[0], dfield[1], dfield[2], field[3]);
			break;
		}
		break;
	case Tops:
		if(n != 4){ /* tops directory name */
			safecpy(topsdir, field[0], sizeof(topsdir));
			return 0;
		}
		s = s_copy(field[3]);
		d.length = atoll(field[0]);
		d.mode = 0666;
		d.uid = "Tops";
		d.gid = d.uid;
		dn = getfields(field[1], dfield, 4, 1, "-");
		if(dn == 3)
			d.atime = cracktime(dfield[1], dfield[0], dfield[2], field[2]);
		else
			d.atime = time(0);
		break;
	case VM:
		switch(n){
		case 9:
			s = s_copy(field[0]);
			s_append(s, ".");
			s_append(s, field[1]);
			d.length = atoll(field[3]) * atoll(field[4]);
			if(*field[2] == 'F')
				d.mode = 0666;
			else
				d.mode = 0777;
			d.uid = "VM";
			d.gid = d.uid;
			dn = getfields(field[6], dfield, 4, 1, "/-");
			if(dn == 3)
				d.atime = cracktime(dfield[0], dfield[1], dfield[2], field[7]);
			else
				d.atime = time(0);
			break;
		case 1:
			s = s_copy(field[0]);
			d.uid = "VM";
			d.gid = d.uid;
			d.mode = 0777;
			d.atime = 0;
			break;
		default:
			return nil;
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
			d.mode = 0666;
			cp = field[0] + strlen(field[0]) - 4;
			if(strcmp(cp, ".dir") == 0){
				d.mode |= DMDIR;
				*cp = 0;
			}
			s = s_copy(field[0]);
			d.length = atoll(field[1]) * 512;
			field[4][strlen(field[4])-1] = 0;
			d.uid = field[4]+1;
			d.gid = d.uid;
			dn = getfields(field[2], dfield, 4, 1, "/-");
			if(dn == 3)
				d.atime = cracktime(dfield[1], dfield[0], dfield[2], field[3]);
			else
				d.atime = time(0);
			break;
		default:
			return nil;
		}
		break;
	case NetWare:
		switch(n){
		case 8:		/* New style */
			s = s_copy(field[7]);
			d.uid = field[2];
			d.gid = d.uid;
			d.mode = nw_mode(field[0][0], field[1]);
			d.length = atoll(field[3]);
			if(strchr(field[6], ':'))
				d.atime = cracktime(field[4], field[5], nil, field[6]);
			else
				d.atime = cracktime(field[4], field[5], field[6], nil);
			break;
		case 9:
			s = s_copy(field[8]);
			d.uid = field[2];
			d.gid = d.uid;
			d.mode = 0666;
			if(*field[0] == 'd')
				d.mode |= DMDIR;
			d.length = atoll(field[3]);
			d.atime = cracktime(field[4], field[5], field[6], field[7]);
			break;
		case 1:
			s = s_copy(field[0]);
			d.uid = "none";
			d.gid = d.uid;
			d.mode = 0777;
			d.atime = 0;
			break;
		default:
			return nil;
		}
		break;
	case Unix:
	case Plan9:
	default:
		switch(n){
		case 8:		/* ls -l */
			s = s_copy(field[7]);
			d.uid = field[2];
			d.gid = d.uid;
			d.mode = crackmode(field[0]);
			d.length = atoll(field[3]);
			if(strchr(field[6], ':'))
				d.atime = cracktime(field[4], field[5], 0, field[6]);
			else
				d.atime = cracktime(field[4], field[5], field[6], 0);
			break;
		case 9:		/* ls -lg */
			s = s_copy(field[8]);
			d.uid = field[2];
			d.gid = field[3];
			d.mode = crackmode(field[0]);
			d.length = atoll(field[4]);
			if(strchr(field[7], ':'))
				d.atime = cracktime(field[5], field[6], 0, field[7]);
			else
				d.atime = cracktime(field[5], field[6], field[7], 0);
			break;
		case 10:	/* plan 9 */
			s = s_copy(field[9]);
			d.uid = field[3];
			d.gid = field[4];
			d.mode = crackmode(field[0]);
			d.length = atoll(field[5]);
			if(strchr(field[8], ':'))
				d.atime = cracktime(field[6], field[7], 0, field[8]);
			else
				d.atime = cracktime(field[6], field[7], field[8], 0);
			break;
		case 4:		/* a Windows_NT version */
			s = s_copy(field[3]);
			d.uid = "NT";
			d.gid = d.uid;
			if(strcmp("<DIR>", field[2]) == 0){
				d.length = 0;
				d.mode = DMDIR|0777;
			} else {
				d.mode = 0666;
				d.length = atoll(field[2]);
			}
			dn = getfields(field[0], dfield, 4, 1, "/-");
			if(dn == 3)
				d.atime = cracktime(dfield[0], dfield[1], dfield[2], field[1]);
			break;
		case 1:
			s = s_copy(field[0]);
			d.uid = "none";
			d.gid = d.uid;
			d.mode = 0777;
			d.atime = 0;
			break;
		default:
			return nil;
		}
	}
	d.muid = d.uid;
	d.qid.type = (d.mode & DMDIR) ? QTDIR : QTFILE;
	d.mtime = d.atime;
	if(ext && (d.qid.type & QTDIR) == 0)
		s_append(s, ext);
	d.name = s_to_c(s);

	/* allocate a freeable dir structure */
	dp = reallocdir(&d, 0);
	*remname = s;

	return dp;
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
	char *line;
	Node *np;
	Dir *d;
	long n;
	int tries, x, files;
	static int uselist;
	int usenlist;
	String *remname;

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

			d = crackdir(line, &remname);
			if(d == nil)
				continue;
			files++;
			np = extendpath(node, remname);
			d->qid.path = np->d->qid.path;
			d->qid.vers = np->d->qid.vers;
			d->type = np->d->type;
			d->dev = 1;			/* mark node as valid */
			if(os == MVS && node == remroot){
				d->qid.type = QTDIR;
				d->mode |= DMDIR;
			}
			free(np->d);
			np->d = d;
		}
		close(Bfildes(bp));

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
	
	sendrequest("MKD", node->d->name);
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
	String *cdpath;

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
			cdpath = s_clone(node->remname);
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
			cdpath = s_new();
			vmspath(node, cdpath);
		}
		break;
	case MVS:
		if(node->depth == 0)
			cdpath = s_clone(remrootpath);
		else{
			cdpath = s_new();
			mvspath(node, cdpath);
		}
		break;
	default:
		if(node->depth == 0)
			cdpath = s_clone(remrootpath);
		else{
			cdpath = s_new();
			unixpath(node, cdpath);
		}
		break;
	}

	uncachedir(remdir, 0);

	/*
	 *  connect, if we need a password (Incomplete)
	 *  act like it worked (best we can do).
	 */
	sendrequest("CWD", s_to_c(cdpath));
	s_free(cdpath);
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

	if(changedir(node->parent) < 0)
		return -1;

	for(tries = 0; tries < 4; tries++){
		switch(data(OREAD, &bp, "RETR", s_to_c(node->remname))){
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

	if(data(OWRITE, &bp, "STOR", s_to_c(node->remname)) != Extra)
		return -1;
	for(off = 0; ; off += n){
		n = fileread(node, buf, off, sizeof(buf));
		if(n <= 0)
			break;
		write(Bfildes(bp), buf, n);
	}
	close(Bfildes(bp));
	if(getreply(&ctlin, msg, sizeof(msg), 0) != Success)
		return -1;
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
	
	sendrequest("DELE", s_to_c(node->remname));
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
	
	sendrequest("RMD", s_to_c(node->remname));
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
	char *p;
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
		rv = strtol(line, &p, 10);
		if(rv >= 100 && rv < 600 && p==line+3 && *p != '-')
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
	port = atoi(ptr);

	memset(ipaddr, 0, IPaddrlen);
	if (*net){
		strcpy(buf, net);
		ptr = strchr(buf +1, '/');
		if (ptr)
			*ptr = 0;
		myipaddr(ipaddr, buf);
	}

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
	char newdir[Maxpath];
	char datafile[Maxpath + 6];
	TLSconn conn;

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

	if(usetls){
		memset(&conn, 0, sizeof(conn));
		dfd = tlsClient(dfd, &conn);
		if(dfd < 0)
			fatal("starting tls: %r");
		free(conn.cert);
	}

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
	char ds[1024];
	char *f[6];
	char *p;
	int x, fd;
	TLSconn conn;

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
	snprint(ds, sizeof(ds), "%s!%s.%s.%s.%s!%d", net,
		f[0], f[1], f[2], f[3],
		((atoi(f[4])&0xff)<<8) + (atoi(f[5])&0xff));

	/* open data connection */
	fd = dial(ds, 0, 0, 0);
	if(fd < 0){
		if(debug)
			fprint(2, "passive mode connect to %s failed: %r\n", ds);
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

	if(usetls){
		memset(&conn, 0, sizeof(conn));
		fd = tlsClient(fd, &conn);
		if(fd < 0)
			fatal("starting tls: %r");
		free(conn.cert);
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
	np = extendpath(np, s_copy(name));
	if(!ISVALID(np)){
		np->d->qid.type = QTDIR;
		np->d->atime = time(0);
		np->d->mtime = np->d->atime;
		strcpy(np->d->uid, "who");
		strcpy(np->d->gid, "cares");
		np->d->mode = DMDIR|0777;
		np->d->length = 0;
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
static void
vmspath(Node *node, String *path)
{
	char *p;
	int n;

	if(node->depth == 1){
		p = strchr(s_to_c(node->remname), ':');
		if(p){
			n = p - s_to_c(node->remname) + 1;
			s_nappend(path, s_to_c(node->remname), n);
			s_append(path, "[");
			s_append(path, p+1);
		} else {
			s_append(path, "[");
			s_append(path, s_to_c(node->remname));
		}
		s_append(path, "]");
		return;
	}
	vmspath(node->parent, path);
	s_append(path, ".");
	s_append(path, s_to_c(node->remname));
}

/*
 *  walk up the tree building a Unix style path
 */
static void
unixpath(Node *node, String *path)
{
	if(node == node->parent){
		s_append(path, s_to_c(remrootpath));
		return;
	}
	unixpath(node->parent, path);
	if(s_len(path) > 0 && strcmp(s_to_c(path), "/") != 0)
		s_append(path, "/");
	s_append(path, s_to_c(node->remname));
}

/*
 *  walk up the tree building a MVS style path
 */
static void
mvspath(Node *node, String *path)
{
	if(node == node->parent){
		s_append(path, s_to_c(remrootpath));
		return;
	}
	mvspath(node->parent, path);
	if(s_len(path) > 0)
		s_append(path, ".");
	s_append(path, s_to_c(node->remname));
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

/*
 *  convert from latin1 to utf
 */
static char*
fromlatin1(char *from)
{
	char *p, *to;
	Rune r;

	if(os == Plan9)
		return nil;

	/* don't convert if we don't have to */
	for(p = from; *p; p++)
		if(*p & 0x80)
			break;
	if(*p == 0)
		return nil;

	to = malloc(3*strlen(from)+2);
	if(to == nil)
		return nil;
	for(p = to; *from; from++){
		r = (*from) & 0xff;
		p += runetochar(p, &r);
	}
	*p = 0;
	return to;
}

Dir*
reallocdir(Dir *d, int dofree)
{
	Dir *dp;
	char *p;
	int nn, ng, nu, nm;
	char *utf;

	if(d->name == nil)
		d->name = "?name?";
	if(d->uid == nil)
		d->uid = "?uid?";
	if(d->gid == nil)
		d->gid = d->uid;
	if(d->muid == nil)
		d->muid = d->uid;
	
	utf = fromlatin1(d->name);
	if(utf != nil)
		d->name = utf;

	nn = strlen(d->name)+1;
	nu = strlen(d->uid)+1;
	ng = strlen(d->gid)+1;
	nm = strlen(d->muid)+1;
	dp = malloc(sizeof(Dir)+nn+nu+ng+nm);
	if(dp == nil){
		if(dofree)
			free(d);
		if(utf != nil)
			free(utf);
		return nil;
	}
	*dp = *d;
	p = (char*)&dp[1];
	strcpy(p, d->name);
	dp->name = p;
	p += nn;
	strcpy(p, d->uid);
	dp->uid = p;
	p += nu;
	strcpy(p, d->gid);
	dp->gid = p;
	p += ng;
	strcpy(p, d->muid);
	dp->muid = p;
	if(dofree)
		free(d);
	if(utf != nil)
		free(utf);
	return dp;
}

Dir*
dir_change_name(Dir *d, char *name)
{
	if(d->name && strlen(d->name) >= strlen(name)){
		strcpy(d->name, name);
		return d;
	}
	d->name = name;
	return reallocdir(d, 1);
}

Dir*
dir_change_uid(Dir *d, char *name)
{
	if(d->uid && strlen(d->uid) >= strlen(name)){
		strcpy(d->name, name);
		return d;
	}
	d->uid = name;
	return reallocdir(d, 1);
}

Dir*
dir_change_gid(Dir *d, char *name)
{
	if(d->gid && strlen(d->gid) >= strlen(name)){
		strcpy(d->name, name);
		return d;
	}
	d->gid = name;
	return reallocdir(d, 1);
}

Dir*
dir_change_muid(Dir *d, char *name)
{
	if(d->muid && strlen(d->muid) >= strlen(name)){
		strcpy(d->name, name);
		return d;
	}
	d->muid = name;
	return reallocdir(d, 1);
}

static int
nw_mode(char dirlet, char *s)		/* NetWare file mode mapping */
{
	int mode = 0777;

	if(dirlet == 'd')
		mode |= DMDIR;

	if (strlen(s) >= 10 && s[0] != '[' || s[9] != ']')
		return(mode);

	if (s[1] == '-')					/* can't read file */
		mode &= ~0444;
	if (dirlet == 'd' && s[6] == '-')			/* cannot scan dir */
		mode &= ~0444;
	if (s[2] == '-')					/* can't write file */
		mode &= ~0222;
	if (dirlet == 'd' && s[7] == '-' && s[3] == '-')	/* cannot create in, or modify dir */
		mode &= ~0222;

	return(mode);
}
