#include <u.h>
#include <libc.h>
#include <bio.h>
#include <ip.h>
#include <fcall.h>
#include "ftpfs.h"

enum
{
	/* return codes */
	Extra=		1,
	Success=	2,
	Incomplete=	3,
	TempFail=	4,
	PermFail=	5,
};

Node	*remdir;		/* current directory on remote machine */
Node	*remroot;		/* root directory on remote machine */

int	ctlfd;			/* fd for control connection */
Biobuf	ctlin;			/* input buffer for control connection */
Biobuf	stdin;			/* input buffer for standard input */
Biobuf	dbuf;			/* buffer for data connection */
char	msg[512];		/* buffer for replies */
char	net[NAMELEN];		/* network for connections */
int	listenfd;		/* fd to listen on for connections */
char	netdir[NETPATHLEN];
int	os;
char	topsdir[64];		/* name of listed directory for TOPS */
char	remrootpath[256];	/* path on remote side to remote root */
char	user[NAMELEN];

static void	sendrequest(char*, ...);
static int	getreply(Biobuf*, char*, int, int);
static int	data(int, Biobuf**);
static int	port(void);

/*
 *  connect to remote server, default network is "tcp/ip"
 */
void
hello(char *dest)
{
	char dir[NETPATHLEN];

	Binit(&stdin, 0, OREAD);	/* init for later use */

	ctlfd = dial(netmkaddr(dest, "tcp", "ftp"), 0, dir, 0);
	if(ctlfd < 0){
		fprint(2, "can't dial %s: %r\n", dest);
		exits("dialing");
	}
	Binit(&ctlin, ctlfd, OREAD);

	/* remember network for the data connections */
	if(strncmp(dir, "/net/", 5) != 0)
		fatal("dial is out of date");
	*strchr(dir+5, '/') = 0;
	safecpy(net, dir+5, sizeof(net));

	/* wait for hello from other side */
	if(getreply(&ctlin, msg, sizeof(msg), 1) != Success)
		fatal("bad hello");
}

/*
 *  login to remote system
 */
void
login(void)
{
	char *line;
	int consctl;

	strcpy(user, getuser());
	for(;;){
		/*
		 *  prompt for user
		 */
		print("User[default = %s]: ", user);
		line = Brdline(&stdin, '\n');
		if(line == 0)
			exits(0);
		line[Blinelen(&stdin)-1] = 0;
		if(*line){
			strncpy(user, line, sizeof(user));
			user[sizeof(user)-1] = 0;
		}
		consctl = open("/dev/consctl", OWRITE);
		if(consctl >= 0)
			write(consctl, "rawon", 5);
		sendrequest("USER %s", user);
		switch(getreply(&ctlin, msg, sizeof(msg), 1)){
		case Success:
			close(consctl);
			return;
		case Incomplete:
			break;
		case TempFail:
		case PermFail:
			close(consctl);
			continue;
		}

		/*
		 *  prompt for password, don't print it
		 */
		print("Password: ");
		line = Brdline(&stdin, '\n');
		if(line == 0)
			exits(0);
		if(consctl >= 0)
			close(consctl);
		print("\n");
		line[Blinelen(&stdin)-1] = 0;
		sendrequest("PASS %s", line);
		if(getreply(&ctlin, msg, sizeof(msg), 1) == Success)
			return;
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
	sendrequest("USER %s", user);
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
	sendrequest("PASS %s", cpassword);
	if(getreply(&ctlin, msg, sizeof(msg), 1) != Success)
		fatal("password failed");
	return;
}

/*
 *  find out about the other side.  go to it's root if requested.  set
 *  image mode if not a TOPS system.
 */
void
preamble(int mountroot)
{
	char *p, *ep;
	int rv;

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
	sendrequest("SYST");
	switch(getreply(&ctlin, msg, sizeof(msg), 1)){
	case Success:
		if(strncmp(msg+4, "SUN", 3) == 0
		|| strncmp(msg+4, "UNIX", 4) == 0)
			os = Unix;
		else if(strncmp(msg+4, "VM", 2) == 0)
			os = VM;
		else if(strncmp(msg+4, "Plan 9", 6) == 0)
			os = Plan9;
		else if(strncmp(msg+4, "TOPS", 4) == 0)
			os = Tops;
		else
			os = Unknown;
		break;
	default:
		os = Unix;
		break;
	}

	switch(os){
	case Unix:
	case Plan9:
		/*
		 *  go to the remote root, if asked
		 */
		if(mountroot){
			sendrequest("CWD /");
			getreply(&ctlin, msg, sizeof(msg), 0);
			*remrootpath = 0;
		} else
			sprint(remrootpath, "/usr/%s", user);

		/*
		 *  get the root directory
		 */
		sendrequest("PWD");
		rv = getreply(&ctlin, msg, sizeof(msg), 1);
		if(rv == PermFail){
			sendrequest("XPWD");
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
	}

	if(os != Tops){
		/*
		 *  go to binary mode for tranfers
		 */
		sendrequest("TYPE I");
		switch(getreply(&ctlin, msg, sizeof(msg), 1)){
		case Success:
			break;
		default:
			fatal("can't set type to image/binary");
		}
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
	static int thisyear;

	/* default time */
	if(now.year == 0)
		now = *localtime(time(0));
	tm = now;

	/* convert ascii month to a number twixt 1 and 12 */
	if(*month >= '0' && *month <= '9'){
		tm.mon = atoi(month);
		if(tm.mon < 1 || tm.mon > 12)
			tm.mon = 6;
	} else {
		for(p = month; *p; p++)
			*p = tolower(*p);
		for(i = 0; i < 12; i++)
			if(strncmp(&monthchars[i*3], month, 3) == 0){
				tm.mon = i+1;
				break;
			}
	}

	tm.mday = atoi(day);

	if(hms){
		tm.hour = atoi(hms);
		p = strchr(hms, ':');
		if(p){
			tm.min = strtol(p+1, &p, 0);
			if(*p == ':')
				tm.sec = atoi(p+1);
		}
	}

	if(yr){
		tm.year = atoi(yr);
		if(tm.year >= 1900)
			tm.year -= 1900;
	}

	/* convert to epoch seconds */
	return tm2sec(tm);
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
	case 10:
		if(*p == 'l')
			return CHSYML|0777;
		else if(*p++ == 'd')
			flags |= CHDIR;
		break;
	case 11:
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
 *  decode a Unix or Plan 9 directory listing
 */
static int
crackdir(char *p, Dir *dp)
{
	char *field[15];
	char *dfield[4];
	int dn, n;

	setfields(" \t");
	n = getmfields(p, field, 15);
	if(n > 2 && strcmp(field[n-2], "->") == 0)
		n -= 2;
	switch(os){
	case Tops:
		if(n != 4){ /* tops directory name */
			safecpy(topsdir, field[0], sizeof(topsdir));
			return -1;
		}
		strncpy(dp->name, field[3], NAMELEN);
		dp->length = atoi(field[0]);
		dp->mode = 0666;
		strcpy(dp->uid, "who");
		strcpy(dp->gid, "cares");
		setfields("-");
		dn = getmfields(field[1], dfield, 4);
		if(dn == 3)
			dp->atime = cracktime(dfield[1], dfield[0], dfield[2], field[2]);
		else
			dp->atime = time(0);
		break;
	case VM:
		if(n < 9)
			return -1;
		sprint(dp->name, "%s.%s", field[0], field[1]);
		dp->length = atoi(field[3])*atoi(field[4]);
		if(*field[2] == 'F')
			dp->mode = 0666;
		else
			dp->mode = 0777;
		strcpy(dp->uid, "who");
		strcpy(dp->gid, "cares");
		setfields("/");
		dn = getmfields(field[6], dfield, 4);
		if(dn == 3)
			dp->atime = cracktime(dfield[0], dfield[1], dfield[2], field[7]);
		else
			dp->atime = time(0);
		break;
	case Unix:
	case Plan9:
	default:
		switch(n){
		case 8:		/* ls -l */
			strncpy(dp->name, field[7], NAMELEN);
			dp->name[NAMELEN-1] = 0;
			strncpy(dp->uid, field[2], NAMELEN);
			dp->uid[NAMELEN-1] = 0;
			strncpy(dp->gid, field[2], NAMELEN);
			dp->gid[NAMELEN-1] = 0;
			dp->mode = crackmode(field[0]);
			dp->length = atoi(field[3]);
			if(strchr(field[6], ':'))
				dp->atime = cracktime(field[4], field[5], 0, field[6]);
			else
				dp->atime = cracktime(field[4], field[5], field[6], 0);
			break;
		case 9:		/* ls -lg */
			strncpy(dp->name, field[8], NAMELEN);
			dp->name[NAMELEN-1] = 0;
			strncpy(dp->uid, field[2], NAMELEN);
			dp->uid[NAMELEN-1] = 0;
			strncpy(dp->gid, field[3], NAMELEN);
			dp->gid[NAMELEN-1] = 0;
			dp->mode = crackmode(field[0]);
			dp->length = atoi(field[4]);
			if(strchr(field[7], ':'))
				dp->atime = cracktime(field[5], field[6], 0, field[7]);
			else
				dp->atime = cracktime(field[5], field[6], field[7], 0);
			break;
		case 1:
			strncpy(dp->name, field[0], NAMELEN);
			strcpy(dp->uid, "none");
			strcpy(dp->gid, "none");
			dp->mode = 0777;
			dp->atime = 0;
			break;
		default:
			return -1;
		}
	}
	dp->qid.path = dp->mode & CHDIR;
	dp->mtime = dp->atime;
	return 0;
}

/*
 *  read a remote directory
 */
int
readdir(Node *node)
{
	Biobuf *bp;
	char *line;
	Node *np;
	Dir d;
	long n;
	int tries;

	if(changedir(node) < 0)
		return -1;

	for(tries = 0; tries < 3; tries++){
		if(port() < 0)
			return -1;

		if(usenlst)
			sendrequest("NLST");
		else
			sendrequest("LIST");
		switch(data(OREAD, &bp)){
		case Extra:
			break;
		case TempFail:
			continue;
		default:
			return seterr(nosuchfile);
		}
		while(line = Brdline(bp, '\n')){
			n = Blinelen(bp);
			if(n > 1 && line[n-2] == '\r')
				n--;
			line[n - 1] = 0;
	
			if(crackdir(line, &d) < 0)
				continue;
			np = extendpath(node, d.name);
			d.qid = np->d.qid;
			d.qid.path |= d.mode & CHDIR;
			d.type = np->d.type;
			d.dev = 1;			/* mark node as valid */
			np->d = d;
		}
		close(Bfildes(bp));
	
		switch(getreply(&ctlin, msg, sizeof(msg), 0)){
		case Success:
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
	
	sendrequest("MKD %s", node->d.name);
	if(getreply(&ctlin, msg, sizeof(msg), 0) != Success)
		return -1;
	return 0;
}

/*
 *  walk up the tree building a Unix style path
 */
char*
unixpath(Node *node, char *path)
{
	char *p;

	if(node == node->parent){
		strcpy(path, remrootpath);
		return path + strlen(remrootpath);
	}
	p = unixpath(node->parent, path);
	*p++ = '/';
	strcpy(p, node->d.name);
	return p + strlen(node->d.name);
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
			strcpy(cdpath, node->d.name);
			break;
		default:
			return seterr(nosuchfile);
		}
		break;
	default:
		if(node->depth == 0)
			strcpy(cdpath, remrootpath);
		else
			unixpath(node, cdpath);
		break;
	}

	/*
	 *  connect, if we need a password (Incomplete)
	 *  act like it worked (best we can do).
	 */
	sendrequest("CWD %s", cdpath);
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
readfile(Node *node)
{
	Biobuf *bp;
	char buf[4*1024];
	long off;
	int n;
	int tries;

	if(changedir(node->parent) < 0)
		return -1;

	for(tries = 0; tries < 4; tries++){
		if(port() < 0)
			return -1;
	
		sendrequest("RETR %s", node->d.name);
		switch(data(OREAD, &bp)){
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
		filewrite(node, buf, off, 0);
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

/*
 *  write back a file
 */
int
createfile(Node *node)
{
	Biobuf *bp;
	char buf[4*1024];
	long off;
	int n;

	if(changedir(node->parent) < 0)
		return -1;

	if(port() < 0)
		return -1;

	sendrequest("STOR %s", node->d.name);
	if(data(OWRITE, &bp) != Extra)
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

/*
 *  remove a remote file
 */
int
removefile(Node *node)
{
	if(changedir(node->parent) < 0)
		return -1;
	
	sendrequest("DELE %s", node->d.name);
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
	
	sendrequest("RMD %s", node->d.name);
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
	sendrequest("QUIT");
	getreply(&ctlin, msg, sizeof(msg), 0);
	exits(0);
}

/*
 *  send a request
 */
static void
sendrequest(char *fmt, ...)
{
	char buf[8*1024], *s;

	s = doprint(buf, buf + (sizeof(buf)-4) / sizeof(*buf), fmt, &fmt + 1);
	*s++ = '\r';
	*s++ = '\n';
	if(write(ctlfd, buf, s - buf) != s - buf)
		fatal("remote side hung up");
	if(debug)
		write(2, buf, s - buf);
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
		if(debug || printreply)
			write(1, line, n);
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
	uchar ipaddr[4];
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
		return seterr("can't read port number");
	n = read(fd, buf, sizeof(buf)-1);
	close(fd);
	if(n <= 0)
		return seterr("can't read port number");
	buf[n] = 0;
	ptr = strchr(buf, ' ');
	if(ptr)
		*ptr = 0;
	ptr = strchr(buf, '!')+1;
	parseip(ipaddr, buf);
	port = atoi(ptr);

	/* tell remote side */
	sendrequest("PORT %d,%d,%d,%d,%d,%d", ipaddr[0], ipaddr[1], ipaddr[2],
		ipaddr[3], port>>8, port&0xff);
	if(getreply(&ctlin, msg, sizeof(msg), 0) != Success)
		return seterr(msg);
	return 0;
}

/*
 *  wait for a data connection
 */
static int
data(int mode, Biobuf **bpp)
{
	int cfd, dfd, rv;
	char newdir[NETPATHLEN];
	char datafile[NETPATHLEN + 6];

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
 *  used for keep alives
 */
void
nop(void)
{
	sendrequest("NOOP");
	getreply(&ctlin, msg, sizeof(msg), 0);
}
