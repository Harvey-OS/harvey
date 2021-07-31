#include "all.h"

/*
 *	Cf. /lib/rfc/rfc1094
 */

static int	mntnull(int, Rpccall*, Rpccall*);
static int	mntmnt(int, Rpccall*, Rpccall*);
static int	mntdump(int, Rpccall*, Rpccall*);
static int	mntumnt(int, Rpccall*, Rpccall*);
static int	mntumntall(int, Rpccall*, Rpccall*);
static int	mntexport(int, Rpccall*, Rpccall*);

Procmap mntproc[] = {
	0, mntnull,
	1, mntmnt,
	2, mntdump,
	3, mntumnt,
	4, mntumntall,
	5, mntexport,
	0, 0
};

long		starttime;
static int		noauth;
char *		config;
Session *	head;
Session *	tail;
int staletime = 10*60;

void
mnttimer(long now)
{
	Session *s;

	for(s=head; s; s=s->next)
		fidtimer(s, now);
}

void
mntinit(int argc, char **argv)
{
	char *argv0 = 0;
	int tries;

	config = "config";
	starttime = time(0);
	clog("nfs mount server init, starttime = %ud\n", starttime);
	tries = 0;
	ARGBEGIN{
	case 'a':
		++tries;
		srvinit(-1, 0, ARGF());
		break;
	case 'f':
		++tries;
		srvinit(-1, ARGF(), 0);
		break;
	case 's':
		++tries;
		srvinit(1, 0, 0);
		break;
	case 'n':
		++noauth;
		break;
	case 'c':
		config = ARGF();
		break;
	case 'T':
		staletime = atoi(ARGF());
		break;
	}ARGEND
	USED(argv0);
	if(tries == 0 && head == 0)
		srvinit(-1, 0, "il!bootes");
	if(head == 0)
		panic("can't initialize services");
	readunixidmaps(config);
}

void
srvinit(int fd, char *file, char *addr)
{
	char fdservice[16], *naddr;
	Session *s;
	Xfile *xp;
	Xfid *xf;
	Fid *f;
	int i;

	s = calloc(1, sizeof(Session));
	s->spec = "";
	s->fd = -1;
	if(fd >= 0){
		s->fd = fd;
		sprint(fdservice, "/fd/%d", s->fd);
		s->service = strstore(fdservice);
		chat("fd = %d\n", s->fd);
	}else if(file){
		chat("file = \"%s\"\n", file);
		s->service = file;
		s->fd = open(file, ORDWR);
		if(s->fd < 0){
			clog("can't open %s: %r\n", file);
			goto error;
		}
	}else if(addr){
		chat("addr = \"%s\"\n", addr);
		naddr = netmkaddr(addr, 0, "9fs");
		s->service = addr;
		s->fd = dial(naddr, 0, 0, 0);
		if(s->fd < 0){
			clog("can't dial %s: %r\n", naddr);
			goto error;
		}
	}

	chat("nop...");
	s->tag = NOTAG-1;
	xmesg(s, Tnop);

	chat("session...");
	s->tag = NOTAG-1;
	srand(starttime);
	for(i = 0; i < CHALLEN; i++)
		s->cchal[i] = nrand(256);
	memmove(s->f.chal, s->cchal, CHALLEN);
	if(xmesg(s, Tsession)){
		clog("session failed\n");
		goto error;
	}
	memmove(s->schal, s->f.chal, CHALLEN);
	memmove(s->authid, s->f.authid, NAMELEN);
	memmove(s->authdom, s->f.authdom, DOMLEN);

	for(i = 0; i < CHALLEN; i++)
		if(s->schal[i])
			break;
	if(i >= CHALLEN || noauth){
		chat("no schal...");
		++s->noauth;
	}
	chat("attach...");
	s->tag = 0;
	f = newfid(s);
	s->f.fid = f - s->fids;
	strncpy(s->f.uname, "none", NAMELEN);
	strncpy(s->f.aname, s->spec, NAMELEN);
	memset(s->f.ticket, 0, TICKETLEN);
	memset(s->f.auth, 0, AUTHENTLEN);
	if(xmesg(s, Tattach)){
		clog("attach failed\n");
		goto error;
	}
	s->count = 1;

	xp = xfile(s->f.qid.path, s, 1);
	s->root = xp;
	xp->parent = xp;
	xp->name = "/";
	xf = xfid("none", xp, 1);
	xf->urfid = f;
	clog("service=%s uid=%s fid=%d\n",
		s->service, xf->uid, xf->urfid-s->fids);
	if(tail)
		tail->next = s;
	else
		head = s;
	tail = s;
	return;

error:
	if(s->fd >= 0)
		close(s->fd);
	free(s);
}

static int
mntnull(int n, Rpccall *cmd, Rpccall *reply)
{
	USED(n, cmd, reply);
	chat("mntnull\n");
	return 0;
}

static char*
Str2str(String s, char *buf, int nbuf)
{
	int i;
	i = s.n;
	if(i >= nbuf)
		i = nbuf-1;
	memmove(buf, s.s, i);
	buf[i] = 0;
	return buf;
}

static int
mntmnt(int n, Rpccall *cmd, Rpccall *reply)
{
	int i;
	char dom[64];
	uchar *argptr = cmd->args;
	uchar *dataptr = reply->results;
	Authunix au;
	Xfile *xp;
	String root;

	chat("mntmnt...\n");
	if(n < 8)
		return garbage(reply, "n too small");
	argptr += string2S(argptr, &root);
	if(argptr != &((uchar *)cmd->args)[n])
		return garbage(reply, "bad count");
	clog("host=%I, port=%d, root=\"%.*s\"...", 
		cmd->host, cmd->port, utfnlen(root.s, root.n), root.s);
	if(auth2unix(&cmd->cred, &au) != 0){
		chat("auth flavor=%d, count=%d\n",
			cmd->cred.flavor, cmd->cred.count);
		for(i=0; i<cmd->cred.count; i++)
			chat(" %.2ux", ((uchar *)cmd->cred.data)[i]);
		chat("\n");
		clog("auth: bad credentials");
		return error(reply, 1);
	}
	clog("auth: %d %.*s u=%d g=%d",
		au.stamp, utfnlen(au.mach.s, au.mach.n), au.mach.s, au.uid, au.gid);
	for(i=0; i<au.gidlen; i++)
		chat(", %d", au.gids[i]);
	chat("...");
	if(getdom(cmd->host, dom, sizeof(dom))<0){
		clog("auth: unknown ip address");
		return error(reply, 1);
	}
	chat("dom=%s...", dom);
	xp = xfroot(root.s, root.n);
	if(xp == 0){
		chat("xp=0...");
		clog("mntmnt: no fs");
		return error(reply, 3);
	}

	PLONG(0);
	dataptr += xp2fhandle(xp, dataptr);
	chat("OK\n");
	return dataptr - (uchar *)reply->results;
}

static int
mntdump(int n, Rpccall *cmd, Rpccall *reply)
{
	if(n != 0)
		return garbage(reply, "mntdump");
	USED(cmd);
	chat("mntdump...");
	return error(reply, FALSE);
}

static int
mntumnt(int n, Rpccall *cmd, Rpccall *reply)
{
	if(n <= 0)
		return garbage(reply, "mntumnt");
	USED(cmd);
	chat("mntumnt\n");
	return 0;
}

static int
mntumntall(int n, Rpccall *cmd, Rpccall *reply)
{
	if(n != 0)
		return garbage(reply, "mntumntall");
	USED(cmd);
	chat("mntumntall\n");
	return 0;
}

static int
mntexport(int n, Rpccall *cmd, Rpccall *reply)
{
	uchar *dataptr = reply->results;
	Authunix au;
	int i;

	chat("mntexport...");
	if(n != 0)
		return garbage(reply, "mntexport");
	if(auth2unix(&cmd->cred, &au) != 0){
		chat("auth flavor=%d, count=%d\n",
			cmd->cred.flavor, cmd->cred.count);
		for(i=0; i<cmd->cred.count; i++)
			chat(" %.2ux", ((uchar *)cmd->cred.data)[i]);
		chat("...");
		au.mach.n = 0;
	}else
		chat("%d@%.*s...", au.uid, utfnlen(au.mach.s, au.mach.n), au.mach.s);
	PLONG(TRUE);
	PLONG(1);
	PPTR("/", 1);
	if(au.mach.n > 0){
		PLONG(TRUE);
		PLONG(au.mach.n);
		PPTR(au.mach.s, au.mach.n);
	}
	PLONG(FALSE);
	PLONG(FALSE);
	chat("OK\n");
	return dataptr - (uchar *)reply->results;
}

Xfile *
xfroot(char *name, int n)
{
	Session *s;
	char *p;

	if(n <= 0)
		n = strlen(name);
	chat("xfroot: %.*s...", utfnlen(name, n), name);
	if(n == 1 && name[0] == '/')
		return head->root;
	for(s=head; s; s=s->next){
		if(strncmp(name, s->service, n) == 0)
			return s->root;
		p = strrchr(s->service, '!');
		if(p && strncmp(name, p+1, n) == 0)
			return s->root;
	}
	return 0;
}
