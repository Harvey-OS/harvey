/*
 * dial - connect to a service (parallel version)
 */
#include <u.h>
#include <libc.h>

typedef struct DS DS;

enum
{
	Maxstring	= 128,
	Maxpath		= 256,

	/* these are probably overly generous */
	Maxconcurr	= 16,
	Maxcsreply	= 80*50,
};

struct DS {
	/* dist string */
	char	buf[Maxstring];
	char	*netdir;
	char	*proto;
	char	*rem;

	/* other args */
	char	*local;
	char	*dir;
	int	*cfdp;
};

/*
 * these need to be seen by this proc & all children.
 * malloc these so we can execute multiple csdials within a
 * multi-process program concurrently.
 */
typedef struct Conn Conn;
typedef struct Dest Dest;
struct Conn {
	int	pid;
	int	dfd;
	int	cfd;
	int	dead;
	char	err[ERRMAX];
	char	dir[NETPATHLEN];
};
struct Dest {
	Conn	conn[Maxconcurr];
	Conn	*connend;
	Conn	*connjunk;		/* last slot */
	int	nkid;
	QLock	winlck;
	int	winner;			/* index into conn[] */
	char	*nextaddr;
	char	addrlist[Maxcsreply];
};

static int	call(char*, char*, DS*, Dest*, Conn*);
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
 * so it must override this with a private version of dial.
 */
int (*_dial)(char *, char *, char *, int *) = dialimpl;

int
dial(char *dest, char *local, char *dir, int *cfdp)
{
	return (*_dial)(dest, local, dir, cfdp);
}

static void
closeopenfd(int *fdp)
{
	if (*fdp > 0) {
		close(*fdp);
		*fdp = -1;
	}
}

static void
notedeath(Dest *dp, char *exitsts)
{
	int i, n, pid;
	char *fields[5];			/* pid + 3 times + error */
	Conn *conn;

	for (i = 0; i < nelem(fields); i++)
		fields[i] = "";
	n = tokenize(exitsts, fields, nelem(fields));
	if (n < 4)
		return;
	pid = atoi(fields[0]);
	for (conn = dp->conn; conn < dp->connend; conn++)
		if (conn->pid == pid && !conn->dead) {  /* it's one we know? */
			if (conn - dp->conn != dp->winner) {
				closeopenfd(&conn->dfd);
				closeopenfd(&conn->cfd);
			}
			strncpy(conn->err, fields[4], sizeof conn->err);
			conn->dead = 1;
			return;
		}
	/* not a proc that we forked */
}

static int
outstandingprocs(Dest *dp)
{
	Conn *conn;

	for (conn = dp->conn; conn < dp->connend; conn++)
		if (!conn->dead)
			return 1;
	return 0;
}

static int
reap(Dest *dp)
{
	char exitsts[2*ERRMAX];

	if (outstandingprocs(dp) && await(exitsts, sizeof exitsts) >= 0) {
		notedeath(dp, exitsts);
		return 0;
	}
	return -1;
}

static int
fillinds(DS *ds, Dest *dp)
{
	Conn *conn;

	if (dp->winner < 0)
		return -1;
	conn = &dp->conn[dp->winner];
	if (ds->cfdp)
		*ds->cfdp = conn->cfd;
	if (ds->dir)
		strncpy(ds->dir, conn->dir, NETPATHLEN);
	return conn->dfd;
}

static int
connectwait(Dest *dp, char *besterr)
{
	Conn *conn;

	/* wait for a winner or all attempts to time out */
	while (dp->winner < 0 && reap(dp) >= 0)
		;

	/* kill all of our still-live kids & reap them */
	for (conn = dp->conn; conn < dp->connend; conn++)
		if (!conn->dead)
			postnote(PNPROC, conn->pid, "die");
	while (reap(dp) >= 0)
		;

	/* rummage about and report some error string */
	for (conn = dp->conn; conn < dp->connend; conn++)
		if (conn - dp->conn != dp->winner && conn->dead &&
		    conn->err[0]) {
			strncpy(besterr, conn->err, ERRMAX);
			break;
		}
	return dp->winner;
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
pickuperr(char *besterr, char *err)
{
	err[0] = '\0';
	errstr(err, ERRMAX);
	if(strstr(err, "does not exist") == 0)
		strcpy(besterr, err);
}

/*
 * try all addresses in parallel and take the first one that answers;
 * this helps when systems have ip v4 and v6 addresses but are
 * only reachable from here on one (or some) of them.
 */
static int
dialmulti(DS *ds, Dest *dp)
{
	int rv, kid, kidme;
	char *clone, *dest;
	char err[ERRMAX], besterr[ERRMAX];
	Conn *conn;

	dp->nkid = dp->winner = -1;
	while(dp->winner < 0 && *dp->nextaddr != '\0' &&
	    parsecs(dp, &clone, &dest) >= 0) {
		if (dp->nkid < nelem(dp->conn))
			++dp->nkid;
		kidme = dp->nkid;		/* make private copy on stack */
		kid = rfork(RFPROC|RFMEM);	/* spin off a call attempt */
		if (kid < 0)
			--dp->nkid;
		else if (kid == 0) {
			*besterr = '\0';
			if (kidme < dp->connend - dp->conn)
				conn = &dp->conn[kidme];
			else			/* too many: let them collide */
				conn = dp->connjunk;
			rv = call(clone, dest, ds, dp, conn);
			if(rv < 0)
				pickuperr(besterr, err);
			_exits(besterr);	/* avoid atexit callbacks */
		}
	}
	rv = connectwait(dp, besterr);
	if(rv < 0 && *besterr)
		werrstr("%s", besterr);
	else
		werrstr("%s", err);
	return rv;
}

static int
csdial(DS *ds)
{
	int n, fd, rv, addrs, bleft;
	char *addrp, *clone2, *dest;
	char buf[Maxstring], clone[Maxpath], err[ERRMAX], besterr[ERRMAX];
	Dest *dp;

	dp = mallocz(sizeof *dp, 1);
	if(dp == nil)
		return -1;
	dp->connjunk = dp->connend = dp->conn + nelem(dp->conn) - 1;
	dp->winner = 0;

	/*
	 *  open connection server
	 */
	snprint(buf, sizeof(buf), "%s/cs", ds->netdir);
	fd = open(buf, ORDWR);
	if(fd < 0){
		/* no connection server, don't translate */
		snprint(clone, sizeof(clone), "%s/%s/clone", ds->netdir, ds->proto);
		rv = call(clone, ds->rem, ds, dp, &dp->conn[0]);
		fillinds(ds, dp);
		free(dp);
		return rv;
	}

	/*
	 *  ask connection server to translate
	 */
	snprint(buf, sizeof(buf), "%s!%s", ds->proto, ds->rem);
	if(write(fd, buf, strlen(buf)) < 0){
		close(fd);
		free(dp);
		return -1;
	}

	/*
	 *  read all addresses from the connection server.
	 */
	seek(fd, 0, 0);
	addrs = 0;
	addrp = dp->nextaddr = dp->addrlist;
	bleft = sizeof dp->addrlist - 2;
	while(bleft > 0 && (n = read(fd, addrp, bleft)) > 0) {
		if (addrp[n-1] != '\n')
			addrp[n++] = '\n';
		addrs++;
		addrp += n;
		bleft -= n;
	}
	close(fd);

	*besterr = 0;
	rv = -1;				/* pessimistic default */
	if (addrs == 0)
		werrstr("no address to dial");
	else if (addrs == 1) {
		/* common case: dial one address without forking */
		if (parsecs(dp, &clone2, &dest) >= 0 &&
		    (rv = call(clone2, dest, ds, dp, &dp->conn[0])) < 0) {
			pickuperr(besterr, err);
			werrstr("%s", besterr);
		}
	} else
		rv = dialmulti(ds, dp);

	/* fill in results */
	if (rv >= 0 && dp->winner >= 0)
		return fillinds(ds, dp);

	free(dp);
	return rv;
}

static int
call(char *clone, char *dest, DS *ds, Dest *dp, Conn *conn)
{
	int fd, cfd, n;
	char cname[Maxpath], name[Maxpath], data[Maxpath], *p;

	/* because cs is in a different name space, replace the mount point */
	if(*clone == '/'){
		p = strchr(clone+1, '/');
		if(p == nil)
			p = clone;
		else 
			p++;
	} else
		p = clone;
	snprint(cname, sizeof cname, "%s/%s", ds->netdir, p);

	conn->pid = getpid();
	conn->cfd = cfd = open(cname, ORDWR);
	if(cfd < 0)
		return -1;

	/* get directory name */
	n = read(cfd, name, sizeof(name)-1);
	if(n < 0){
		close(cfd);
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

	/* connect */
	if(ds->local)
		snprint(name, sizeof(name), "connect %s %s", dest, ds->local);
	else
		snprint(name, sizeof(name), "connect %s", dest);
	if(write(cfd, name, strlen(name)) < 0){
		close(cfd);
		return -1;
	}

	/* open data connection */
	conn->dfd = fd = open(data, ORDWR);
	if(fd < 0){
		close(cfd);
		return -1;
	}
	if(ds->cfdp)
		conn->cfd = cfd;
	else
		close(cfd);

	qlock(&dp->winlck);
	if (dp->winner < 0 && conn < dp->connend)
		dp->winner = conn - dp->conn;
	qunlock(&dp->winlck);
	return fd;
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
			for(p2 = p; *p2 != '/'; p2--)
				;
			*p2++ = 0;
			ds->netdir = ds->buf;
			ds->proto = p2;
		}
		*p = 0;
		ds->rem = p + 1;
	}
}
