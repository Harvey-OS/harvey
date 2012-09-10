/*
 * dial - connect to a service (threaded parallel version)
 */
#include <u.h>
#include <libc.h>
#include <ctype.h>
#include <fcall.h>
#include <thread.h>
#include <9p.h>

typedef struct Conn Conn;
typedef struct Dest Dest;
typedef struct DS DS;
typedef struct Kidargs Kidargs;
typedef struct Restup Restup;

enum
{
	Noblock,
	Block,

	Defstksize	= 8192,

	Maxstring	= 128,
	Maxpath		= 256,

	Maxcsreply	= 64*80,	/* this is probably overly generous */
	/*
	 * this should be a plausible slight overestimate for non-interactive
	 * use even if it's ridiculously long for interactive use.
	 */
	Maxconnms	= 2*60*1000,	/* 2 minutes */
};

struct DS {
	/* dial string */
	char	buf[Maxstring];
	char	*netdir;
	char	*proto;
	char	*rem;

	/* other args */
	char	*local;
	char	*dir;
	int	*cfdp;
};

struct Conn {
	int	cfd;
	char	dir[NETPATHLEN+1];
};
struct Dest {
	DS	*ds;

	Channel *reschan;		/* all callprocs send results on this */
	int	nkid;
	int	kidthrids[64];		/* one per addr; ought to be enough */

	int	windfd;
	char	err[ERRMAX];

	long	oalarm;

	int	naddrs;
	char	*nextaddr;
	char	addrlist[Maxcsreply];
};

struct Kidargs {			/* arguments to callproc */
	Dest	*dp;
	int	thridsme;
	char	*clone;
	char	*dest;
};

struct Restup {				/* result tuple from callproc */
	int	dfd;
	int	cfd;
	char	*err;
	char	*conndir;
};

static int	call(char*, char*, Dest*, Conn*);
static int	call1(char*, char*, Dest*, Conn*);
static int	csdial(DS*);
static void	_dial_string_parse(char*, DS*);

/*
 *  the dialstring is of the form '[/net/]proto!dest'
 */
static int
dialimpl(char *dest, char *local, char *dir, int *cfdp)
{
	DS ds;
	int rv;
	char err[ERRMAX], alterr[ERRMAX];

	ds.local = local;
	ds.dir = dir;
	ds.cfdp = cfdp;

	_dial_string_parse(dest, &ds);
	if(ds.netdir)
		return csdial(&ds);

	ds.netdir = "/net";
	rv = csdial(&ds);
	if(rv >= 0)
		return rv;
	err[0] = '\0';
	errstr(err, sizeof err);
	if(strstr(err, "refused") != 0){
		werrstr("%s", err);
		return rv;
	}
	ds.netdir = "/net.alt";
	rv = csdial(&ds);
	if(rv >= 0)
		return rv;

	alterr[0] = 0;
	errstr(alterr, sizeof alterr);
	if(strstr(alterr, "translate") || strstr(alterr, "does not exist"))
		werrstr("%s", err);
	else
		werrstr("%s", alterr);
	return rv;
}

/*
 * the thread library can't cope with rfork(RFMEM|RFPROC),
 * so it must override _dial with this version of dial.
 */
int (*_dial)(char *, char *, char *, int *) = dialimpl;

int
dial(char *dest, char *local, char *dir, int *cfdp)
{
	return (*_dial)(dest, local, dir, cfdp);
}

static void
freedest(Dest *dp)
{
	if (dp) {
		if (dp->oalarm >= 0)
			alarm(dp->oalarm);
		free(dp);
	}
}

static void
closeopenfd(int *fdp)
{
	if (*fdp >= 0) {
		close(*fdp);
		*fdp = -1;
	}
}

static int
parsecs(Dest *dp, char **clonep, char **destp)
{
	char *dest, *p;

	dest = strchr(dp->nextaddr, ' ');
	if(dest == nil)
		return -1;
	*dest++ = '\0';
	p = strchr(dest, '\n');
	if(p == nil)
		return -1;
	*p++ = '\0';
	*clonep = dp->nextaddr;
	*destp = dest;
	dp->nextaddr = p;		/* advance to next line */
	return 0;
}

static void
pickuperr(char *besterr)
{
	char err[ERRMAX];

	err[0] = '\0';
	errstr(err, ERRMAX);
	if(strstr(err, "does not exist") == 0)
		strcpy(besterr, err);
}

static int
catcher(void *, char *s)
{
	return strstr(s, "alarm") != nil;
}

static void
callproc(void *p)
{
	int dfd;
	char besterr[ERRMAX];
	Conn lconn;
	Conn *conn;
	Kidargs *args;
	Restup *tup;

	threadnotify(catcher, 1);	/* avoid atnotify callbacks in parent */

	conn = &lconn;
	memset(conn, 0, sizeof *conn);
	*besterr = '\0';
	args = (Kidargs *)p;
	dfd = call(args->clone, args->dest, args->dp, conn);
	if(dfd < 0)
		pickuperr(besterr);

	tup = (Restup *)emalloc9p(sizeof *tup);
	*tup = (Restup){dfd, conn->cfd, nil, nil};
	if (dfd >= 0)
		tup->conndir = strdup(conn->dir);
	else
		tup->err = strdup(besterr);
	sendp(args->dp->reschan, tup);

	args->dp->kidthrids[args->thridsme] = -1;
	free(args);
	threadexits(besterr);		/* better be no atexit callbacks */
}

/* interrupt all of our still-live kids */
static void
intrcallprocs(Dest *dp)
{
	int i;

	for (i = 0; i < nelem(dp->kidthrids); i++)
		if (dp->kidthrids[i] >= 0)
			threadint(dp->kidthrids[i]);
}

static int
recvresults(Dest *dp, int block)
{
	DS *ds;
	Restup *tup;

	for (; dp->nkid > 0; dp->nkid--) {
		if (block)
			tup = recvp(dp->reschan);
		else
			tup = nbrecvp(dp->reschan);
		if (tup == nil)
			break;
		if (tup->dfd >= 0)		/* connected? */
			if (dp->windfd < 0) {	/* first connection? */
				ds = dp->ds;
				dp->windfd = tup->dfd;
				if (ds->cfdp)
					*ds->cfdp = tup->cfd;
				if (ds->dir) {
					strncpy(ds->dir, tup->conndir,
						NETPATHLEN);
					ds->dir[NETPATHLEN-1] = '\0';
				}
				intrcallprocs(dp);
			} else {
				close(tup->dfd);
				close(tup->cfd);
			}
		else if (dp->err[0] == '\0' && tup->err) {
			strncpy(dp->err, tup->err, ERRMAX - 1);
			dp->err[ERRMAX - 1] = '\0';
		}
		free(tup->conndir);
		free(tup->err);
		free(tup);
	}
	return dp->windfd;
}

/*
 * try all addresses in parallel and take the first one that answers;
 * this helps when systems have ip v4 and v6 addresses but are
 * only reachable from here on one (or some) of them.
 */
static int
dialmulti(Dest *dp)
{
	int kidme;
	char *clone, *dest;
	Kidargs *argp;

	dp->reschan = chancreate(sizeof(void *), 0);
	dp->err[0] = '\0';
	dp->nkid = 0;
	dp->windfd = -1;
	/* if too many addresses for dp->kidthrids, ignore the last few */
	while(dp->windfd < 0 && dp->nkid < nelem(dp->kidthrids) &&
	    *dp->nextaddr != '\0' && parsecs(dp, &clone, &dest) >= 0) {
		kidme = dp->nkid++;

		argp = (Kidargs *)emalloc9p(sizeof *argp);
		*argp = (Kidargs){dp, kidme, clone, dest};

		dp->kidthrids[kidme] = proccreate(callproc, argp, Defstksize);
		if (dp->kidthrids[kidme] < 0)
			--dp->nkid;
	}

	recvresults(dp, Block);
	assert(dp->nkid == 0);

	chanclose(dp->reschan);
	chanfree(dp->reschan);
	if(dp->windfd < 0 && dp->err[0])
		werrstr("%s", dp->err);
	return dp->windfd;
}

/* call a single address and pass back cfd & conn dir after */
static int
call1(char *clone, char *rem, Dest *dp, Conn *conn)
{
	int dfd;
	DS *ds;

	ds = dp->ds;
	dfd = call(clone, rem, dp, conn);
	if (dfd < 0)
		return dfd;

	if (ds->cfdp)
		*ds->cfdp = conn->cfd;
	if (ds->dir) {
		strncpy(ds->dir, conn->dir, NETPATHLEN);
		ds->dir[NETPATHLEN-1] = '\0';
	}
	return dfd;
}

static int
csdial(DS *ds)
{
	int n, fd, dfd, addrs, bleft;
	char c;
	char *addrp, *clone2, *dest;
	char buf[Maxstring], clone[Maxpath], besterr[ERRMAX];
	Conn lconn;
	Conn *conn;
	Dest *dp;

	dp = mallocz(sizeof *dp, 1);
	if(dp == nil)
		return -1;
	conn = &lconn;
	memset(conn, 0, sizeof *conn);
	dp->ds = ds;
	if (ds->cfdp)
		*ds->cfdp = -1;
	if (ds->dir)
		ds->dir[0] = '\0';
	dp->oalarm = alarm(0);

	/*
	 *  open connection server
	 */
	snprint(buf, sizeof(buf), "%s/cs", ds->netdir);
	fd = open(buf, ORDWR);
	if(fd < 0){
		/* no connection server, don't translate */
		snprint(clone, sizeof(clone), "%s/%s/clone", ds->netdir, ds->proto);
		dfd = call1(clone, ds->rem, dp, conn);
		freedest(dp);
		return dfd;
	}

	/*
	 *  ask connection server to translate
	 */
	snprint(buf, sizeof(buf), "%s!%s", ds->proto, ds->rem);
	if(write(fd, buf, strlen(buf)) < 0){
		close(fd);
		freedest(dp);
		return -1;
	}

	/*
	 *  read all addresses from the connection server.
	 */
	seek(fd, 0, 0);
	addrs = 0;
	addrp = dp->nextaddr = dp->addrlist;
	bleft = sizeof dp->addrlist - 2;	/* 2 is room for \n\0 */
	while(bleft > 0 && (n = read(fd, addrp, bleft)) > 0) {
		if (addrp[n-1] != '\n')
			addrp[n++] = '\n';
		addrs++;
		addrp += n;
		bleft -= n;
	}
	/*
	 * if we haven't read all of cs's output, assume the last line might
	 * have been truncated and ignore it.  we really don't expect this
	 * to happen.
	 */
	if (addrs > 0 && bleft <= 0 && read(fd, &c, 1) == 1)
		addrs--;
	close(fd);

	*besterr = 0;
	dfd = -1;				/* pessimistic default */
	dp->naddrs = addrs;
	if (addrs == 0)
		werrstr("no address to dial");
	else if (addrs == 1) {
		/* common case: dial one address without forking */
		if (parsecs(dp, &clone2, &dest) >= 0 &&
		    (dfd = call1(clone2, dest, dp, conn)) < 0) {
			pickuperr(besterr);
			werrstr("%s", besterr);
		}
	} else
		dfd = dialmulti(dp);

	freedest(dp);
	return dfd;
}

/* returns dfd, stores cfd through cfdp */
static int
call(char *clone, char *dest, Dest *dp, Conn *conn)
{
	int fd, cfd, n, calleralarm, oalarm;
	char cname[Maxpath], name[Maxpath], data[Maxpath], *p;
	DS *ds;

	/* because cs is in a different name space, replace the mount point */
	if(*clone == '/'){
		p = strchr(clone+1, '/');
		if(p == nil)
			p = clone;
		else 
			p++;
	} else
		p = clone;
	ds = dp->ds;
	snprint(cname, sizeof cname, "%s/%s", ds->netdir, p);

	conn->cfd = cfd = open(cname, ORDWR);
	if(cfd < 0)
		return -1;

	/* get directory name */
	n = read(cfd, name, sizeof(name)-1);
	if(n < 0){
		closeopenfd(&conn->cfd);
		return -1;
	}
	name[n] = 0;
	for(p = name; *p == ' '; p++)
		;
	snprint(name, sizeof(name), "%ld", strtoul(p, 0, 0));
	p = strrchr(cname, '/');
	*p = 0;
	if(ds->dir)
		snprint(conn->dir, NETPATHLEN, "%s/%s", cname, name);
	snprint(data, sizeof(data), "%s/%s/data", cname, name);

	/* should be no alarm pending now; re-instate caller's alarm, if any */
	calleralarm = dp->oalarm > 0;
	if (calleralarm)
		alarm(dp->oalarm);
	else if (dp->naddrs > 1)	/* in a sub-process? */
		alarm(Maxconnms);

	/* connect */
	if(ds->local)
		snprint(name, sizeof(name), "connect %s %s", dest, ds->local);
	else
		snprint(name, sizeof(name), "connect %s", dest);
	if(write(cfd, name, strlen(name)) < 0){
		closeopenfd(&conn->cfd);
		return -1;
	}

	oalarm = alarm(0);	/* don't let alarm interrupt critical section */
	if (calleralarm)
		dp->oalarm = oalarm;	/* time has passed, so update user's */

	/* open data connection */
	fd = open(data, ORDWR);
	if(fd < 0){
		closeopenfd(&conn->cfd);
		alarm(dp->oalarm);
		return -1;
	}
	if(ds->cfdp == nil)
		closeopenfd(&conn->cfd);

	alarm(calleralarm? dp->oalarm: 0);
	return fd;
}

/*
 * assume p points at first '!' in dial string.  st is start of dial string.
 * there could be subdirs of the conn dirs (e.g., ssh/0) that must count as
 * part of the proto string, so skip numeric components.
 * returns pointer to delimiter after right-most non-numeric component.
 */
static char *
backoverchans(char *st, char *p)
{
	char *sl;

	for (sl = p; --p >= st && isascii(*p) && isdigit(*p); sl = p) {
		while (--p >= st && isascii(*p) && isdigit(*p))
			;
		if (p < st || *p != '/')
			break;			/* "net.alt2" or ran off start */
		while (p > st && p[-1] == '/')	/* skip runs of slashes */
			p--;
	}
	return sl;
}

/*
 *  parse a dial string
 */
static void
_dial_string_parse(char *str, DS *ds)
{
	char *p, *p2;

	strncpy(ds->buf, str, Maxstring);
	ds->buf[Maxstring-1] = 0;

	p = strchr(ds->buf, '!');
	if(p == 0) {
		ds->netdir = 0;
		ds->proto = "net";
		ds->rem = ds->buf;
	} else {
		if(*ds->buf != '/' && *ds->buf != '#'){
			ds->netdir = 0;
			ds->proto = ds->buf;
		} else {
			p2 = backoverchans(ds->buf, p);

			/* back over last component of netdir (proto) */
			while (--p2 > ds->buf && *p2 != '/')
				;
			*p2++ = 0;
			ds->netdir = ds->buf;
			ds->proto = p2;
		}
		*p = 0;
		ds->rem = p + 1;
	}
}
