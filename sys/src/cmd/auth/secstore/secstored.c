#include <u.h>
#include <libc.h>
#include <bio.h>
#include <ndb.h>
#include <mp.h>
#include <libsec.h>
#include "SConn.h"
#include "secstore.h"

int secureidcheck(char *, char *);   // from /sys/src/cmd/auth/
extern char* dirls(char *path);

int verbose;
Ndb *db;

static void
usage(void)
{
	fprint(2, "secstored: [-S servername] [-s tcp!*!5356] [-v] [-x netmtpt]\n");
	exits("usage");
}

static int
getdir(SConn *conn, char *id)
{
	char *ls, *s; 
	int n, len;

	s = emalloc(Maxmsg);
	snprint(s, Maxmsg, "%s/store/%s", SECSTORE_DIR, id);

	if((ls = dirls(s)) == nil)
		len = 0;
	else
		len = strlen(ls);

	/* send file size */
	snprint(s, Maxmsg, "%d", len);
	conn->write(conn, (uchar*)s, strlen(s));

	/* send directory listing in Maxmsg chunks */
	n = Maxmsg;
	while(len > 0){
		if(len < Maxmsg)
			n = len;
		conn->write(conn, (uchar*)ls, n);
		ls += n;
		len -= n;
	}
	free(s);
	return 0;
}

static int
getfile(SConn *conn, char *id, char *gf)
{
	int n, gd, len;
	char *s;

	if(strcmp(gf,".")==0)
		return getdir(conn, id);

	/* send file size */
	if(strchr(gf,'/') != nil || strcmp(gf,"..")==0){
		fprint(2, "no slashes allowed: %s\n", gf);
		conn->write(conn, (uchar*)"-2", 2);
		return -1;
	}
	s = emalloc(Maxmsg);
	snprint(s, Maxmsg, "%s/store/%s/%s", SECSTORE_DIR, id, gf);
	gd = open(s, OREAD);
	if(gd < 0){
		fprint(2, "can't open %s: %r\n", s);
		free(s);
		conn->write(conn, (uchar*)"-1", 2);
		return -1;
	}
	len = seek(gd, 0, 2);
	if(len < 0 || len > MAXFILESIZE){//assert
		fprint(2, "implausible filesize %d for %s\n", len, gf);
		free(s);
		conn->write(conn, (uchar*)"-3", 2);
		return -1;
	}
	seek(gd, 0, 0);
	snprint(s, Maxmsg, "%d", len);
	conn->write(conn, (uchar*)s, strlen(s));

	/* send file in Maxmsg chunks */
	while(len > 0){
		n = read(gd, s, Maxmsg);
		if(n <= 0){
			fprint(2, "read error on %s: %r\n", gf);
			free(s);
			return -1;
		}
		conn->write(conn, (uchar*)s, n);
		len -= n;
	}
	close(gd);
	free(s);
	return 0;
}

static int
putfile(SConn *conn, char *id, char *pf)
{
	int n, nw, pd;
	long len;
	char s[Maxmsg+1];

	/* get file size */
	n = readstr(conn, s);
	if(n < 0){//assert
		fprint(2, "remote: %s: %r\n", s);
		return -1;
	}
	len = atoi(s);
	if(len == -1){
		fprint(2, "remote file %s does not exist\n", pf);
		return -1;
	}else if(len < 0 || len > MAXFILESIZE){//assert
		fprint(2, "implausible filesize %ld for %s\n", len, pf);
		return -1;
	}

	/* get file in Maxmsg chunks */
	if(strchr(pf,'/') != nil || strcmp(pf,"..")==0){
		fprint(2, "no slashes allowed: %s\n", pf);
		return -1;
	}
	snprint(s, Maxmsg, "%s/store/%s/%s", SECSTORE_DIR, id, pf);
	pd = create(s, OWRITE, 0660);
	if(pd < 0){//assert
		fprint(2, "can't open %s: %r\n", s);
		return -1;
	}
	while(len > 0){
		n = conn->read(conn, (uchar*)s, Maxmsg);
		if(n <= 0){//assert
			fprint(2, "empty file chunk\n");
			return -1;
		}
		nw = write(pd, s, n);
		if(nw != n){//assert
			fprint(2, "write error on %s: %r", pf);
			return -1;
		}
		len -= n;
	}
	close(pd);
	return 0;

}

static int
removefile(SConn *conn, char *id, char *f)
{
	Dir *d;
	char buf[Maxmsg];

	snprint(buf, Maxmsg, "%s/store/%s/%s", SECSTORE_DIR, id, f);

	if((d =dirstat(buf)) == nil){
		snprint(buf, sizeof buf, "remove failed: %r");
		writerr(conn, buf);
		return -1;
	}else if(d->mode & DMDIR){
		snprint(buf, sizeof buf, "can't remove a directory");
		writerr(conn, buf);
		return -1;
	}

	if(remove(buf) < 0){
		snprint(buf, sizeof buf, "remove failed: %r");
		writerr(conn, buf);
		return -1;
	}
	return 0;
}

/* given line directory from accept, returns ipaddr!port */
static char*
remoteIP(char *ldir)
{
	int fd, n;
	char rp[100], ap[500];

	snprint(rp, sizeof rp, "%s/remote", ldir);
	fd = open(rp, OREAD);
	if(fd < 0)
		return strdup("?!?");
	n = read(fd, ap, sizeof ap);
	if(n <= 0 || n == sizeof ap){
		fprint(2, "error %d reading %s: %r\n", n, rp);
		return strdup("?!?");
	}
	close(fd);
	ap[n--] = 0;
	if(ap[n] == '\n')
		ap[n] = 0;
	return strdup(ap);
}

static int
dologin(int fd, char *S, int forceSTA)
{
	int i, n, rv;
	char *file, *nl;
	char msg[Maxmsg+1];
	PW *pw;
	SConn *conn;

	pw = nil;
	rv = -1;

	// collect the first message
	if((conn = newSConn(fd)) == nil)
		return -1;
	if(readstr(conn, msg) < 0){
		fprint(2, "remote: %s: %r\n", msg);
		writerr(conn, "can't read your first message");
		goto Out;
	}

	// authenticate
	if(PAKserver(conn, S, msg, &pw) < 0){
		if(pw != nil)
			syslog(0, LOG, "secstore denied for %s", pw->id);
		goto Out;
	}
	if((forceSTA || pw->status&STA) != 0){
		conn->write(conn, (uchar*)"STA", 3);
		if(readstr(conn, msg) < 10 || strncmp(msg, "STA", 3) != 0){
			syslog(0, LOG, "no STA from %s", pw->id);
			goto Out;
		}
		if(secureidcheck(pw->id, msg+3) <= 0){
			syslog(0, LOG, "secureidcheck denied %s", pw->id);
			goto Out;
		}
	}
	conn->write(conn, (uchar*)"OK", 2);
	syslog(0, LOG, "AUTH %s", pw->id);

	// perform operations as asked
	while((n = readstr(conn, msg)) > 0){
		if(strncmp(msg, "GET ", 4) == 0){
			file = msg+4;
			if(nl = strchr(file, '\n'))
				*nl = 0;
			if(getfile(conn, pw->id, file) < 0)
				goto Out;
			syslog(0, LOG, "GET %s/%s", pw->id, file);
		}else if(strncmp(msg, "PUT ", 4) == 0){
			file = msg+4;
			if(nl = strchr(file, '\n'))
				*nl = 0;
			if(putfile(conn, pw->id, file) < 0)
				goto Out;
			syslog(0, LOG, "PUT %s/%s", pw->id, file);
		}else if(strncmp(msg, "RM ", 3) == 0){
			file = msg + 3;
			if(nl = strchr(file, '\n'))
				*nl = 0;
			if(removefile(conn, pw->id, file) < 0)
				goto Out;
			syslog(0, LOG, "RM %s/%s", pw->id, file);
		}else if(strncmp(msg, "CHPASS", 6) == 0){
			if(readstr(conn, msg) < 0){
				writerr(conn, "protocol botch while setting PAK");
				goto Out;
			}
			pw->Hi = strtomp(msg, nil, 64, pw->Hi);
			for(i=0; i < 4 && putPW(pw) < 0; i++)
				syslog(0, LOG, "password change failed for %s (%d): %r", pw->id, i);
			if(i==4)
				goto Out;
			syslog(0, LOG, "CHPASS for '%s'", pw->id);
		}else if(strncmp(msg, "BYE", 3) == 0){
			rv = 0;
			break;
		}else{
			syslog(0, LOG, "unrecognized operation: %s\n", msg);
			writerr(conn, "unrecognized operation");
			break;
		}

	}
	if(n <= 0)
		syslog(0, LOG, "%s closed connection without saying goodbye\n", pw->id);

Out:
	freePW(pw);
	conn->free(conn);
	return rv;
}

void
main(int argc, char **argv)
{
	int afd, dfd, lcfd, forceSTA = 0;
	char adir[40], ldir[40], *remote;
	char *serve = "tcp!*!5356", *p, aserve[128], net[128];
	char *S = "secstore";
	Ndb *db2;

	setnetmtpt(net, sizeof(net), nil);
	ARGBEGIN{
	case 's':
		serve = EARGF(usage());
		break;
	case 'S':
		S = EARGF(usage());
		break;
	case 'x':
		p = ARGF();
		if(p == nil)
			usage();
		setnetmtpt(net, sizeof(net), p);
		forceSTA = 1;  // for any non-standard network setting, be paranoid
		break;
	case 'v':
		verbose++;
		break;
	default:
		usage();
	}ARGEND;

	if(!verbose)
		switch(rfork(RFNOTEG|RFPROC|RFFDG)) {
		case -1:
			sysfatal("fork: %r");
		case 0:
			break;
		default:
			exits(0);
		}

	snprint(aserve, sizeof aserve, "%s/%s", net, serve);
	afd = announce(aserve, adir);
	if(afd < 0)
		sysfatal("%s: %r\n", aserve);
	syslog(0, LOG, "ANNOUNCE %s", aserve);
	for(;;){
		if((lcfd = listen(adir, ldir)) < 0)
			exits("can't listen");
		switch(fork()){
		case -1:
			fprint(2, "secstore forking: %r\n");
			close(lcfd);
			break;
		case 0:
			// "/lib/ndb/common.radius does not exist" if db set before fork
			db = ndbopen("/lib/ndb/auth");
			if(db == 0)
				syslog(0, LOG, "no /lib/ndb/auth");
			db2 = ndbopen(0);
			if(db2 == 0)
				syslog(0, LOG, "no /lib/ndb/local");
			db = ndbcat(db, db2);
			if((dfd = accept(lcfd, ldir)) < 0)
				exits("can't accept");
			alarm(30*60*1000); 	// 30 min
			remote = remoteIP(ldir);
			syslog(0, LOG, "secstore from %s", remote);
			free(remote);
			dologin(dfd, S, forceSTA);
			exits(nil);
		default:
			close(lcfd);
			break;
		}
	}
}

