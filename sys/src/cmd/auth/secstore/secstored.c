/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

/* secstored - secure store daemon */
#include <u.h>
#include <libc.h>
#include <bio.h>
#include <ndb.h>
#include <mp.h>
#include <libsec.h>
#include "SConn.h"
#include "secstore.h"

char* secureidcheck(char *, char *);	/* from /sys/src/cmd/auth/ */
extern char* dirls(char *path);

int verbose;
Ndb *db;

static void
usage(void)
{
	fprint(2, "usage: secstored [-R] [-S servername] [-s tcp!*!5356] "
		"[-v] [-x netmtpt]\n");
	exits("usage");
}

static int
getdir(SConn *conn, char *id)
{
	char *ls, *s;
	uint8_t *msg;
	int n, len;

	s = emalloc(Maxmsg);
	snprint(s, Maxmsg, "%s/store/%s", SECSTORE_DIR, id);

	if((ls = dirls(s)) == nil)
		len = 0;
	else
		len = strlen(ls);

	/* send file size */
	snprint(s, Maxmsg, "%d", len);
	conn->write(conn, (uint8_t*)s, strlen(s));

	/* send directory listing in Maxmsg chunks */
	n = Maxmsg;
	msg = (uint8_t*)ls;
	while(len > 0){
		if(len < Maxmsg)
			n = len;
		conn->write(conn, msg, n);
		msg += n;
		len -= n;
	}
	free(s);
	free(ls);
	return 0;
}

static int
getfile(SConn *conn, char *id, char *gf)
{
	int n, gd, len;
	uint32_t mode;
	char *s;
	Dir *st;

	if(strcmp(gf,".")==0)
		return getdir(conn, id);

	/* send file size */
	s = emalloc(Maxmsg);
	snprint(s, Maxmsg, "%s/store/%s/%s", SECSTORE_DIR, id, gf);
	gd = open(s, OREAD);
	if(gd < 0){
		syslog(0, LOG, "can't open %s: %r", s);
		free(s);
		conn->write(conn, (uint8_t*)"-1", 2);
		return -1;
	}
	st = dirfstat(gd);
	if(st == nil){
		syslog(0, LOG, "can't stat %s: %r", s);
		free(s);
		conn->write(conn, (uint8_t*)"-1", 2);
		return -1;
	}
	mode = st->mode;
	len = st->length;
	free(st);
	if(mode & DMDIR) {
		syslog(0, LOG, "%s should be a plain file, not a directory", s);
		free(s);
		conn->write(conn, (uint8_t*)"-1", 2);
		return -1;
	}
	if(len < 0 || len > MAXFILESIZE){
		syslog(0, LOG, "implausible filesize %d for %s", len, gf);
		free(s);
		conn->write(conn, (uint8_t*)"-3", 2);
		return -1;
	}
	snprint(s, Maxmsg, "%d", len);
	conn->write(conn, (uint8_t*)s, strlen(s));

	/* send file in Maxmsg chunks */
	while(len > 0){
		n = read(gd, s, Maxmsg);
		if(n <= 0){
			syslog(0, LOG, "read error on %s: %r", gf);
			free(s);
			return -1;
		}
		conn->write(conn, (uint8_t*)s, n);
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
	int32_t len;
	char s[Maxmsg+1];

	/* get file size */
	n = readstr(conn, s);
	if(n < 0){
		syslog(0, LOG, "remote: %s: %r", s);
		return -1;
	}
	len = atoi(s);
	if(len == -1){
		syslog(0, LOG, "remote file %s does not exist", pf);
		return -1;
	}else if(len < 0 || len > MAXFILESIZE){
		syslog(0, LOG, "implausible filesize %ld for %s", len, pf);
		return -1;
	}

	snprint(s, Maxmsg, "%s/store/%s/%s", SECSTORE_DIR, id, pf);
	pd = create(s, OWRITE, 0660);
	if(pd < 0){
		syslog(0, LOG, "can't open %s: %r\n", s);
		return -1;
	}
	while(len > 0){
		n = conn->read(conn, (uint8_t*)s, Maxmsg);
		if(n <= 0){
			syslog(0, LOG, "empty file chunk");
			return -1;
		}
		nw = write(pd, s, n);
		if(nw != n){
			syslog(0, LOG, "write error on %s: %r", pf);
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

	if((d = dirstat(buf)) == nil){
		snprint(buf, sizeof buf, "remove failed: %r");
		writerr(conn, buf);
		return -1;
	}else if(d->mode & DMDIR){
		snprint(buf, sizeof buf, "can't remove a directory");
		writerr(conn, buf);
		free(d);
		return -1;
	}

	free(d);
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
		fprint(2, "secstored: error %d reading %s: %r\n", n, rp);
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
	char *file, *mess, *nl;
	char msg[Maxmsg+1];
	PW *pw;
	SConn *conn;

	pw = nil;
	rv = -1;

	/* collect the first message */
	if((conn = newSConn(fd)) == nil)
		return -1;
	if(readstr(conn, msg) < 0){
		fprint(2, "secstored: remote: %s: %r\n", msg);
		writerr(conn, "can't read your first message");
		goto Out;
	}

	/* authenticate */
	if(PAKserver(conn, S, msg, &pw) < 0){
		if(pw != nil)
			syslog(0, LOG, "secstore denied for %s", pw->id);
		goto Out;
	}
	if((forceSTA || pw->status&STA) != 0){
		conn->write(conn, (uint8_t*)"STA", 3);
		if(readstr(conn, msg) < 10 || strncmp(msg, "STA", 3) != 0){
			syslog(0, LOG, "no STA from %s", pw->id);
			goto Out;
		}
		mess = secureidcheck(pw->id, msg+3);
		if(mess != nil){
			syslog(0, LOG, "secureidcheck denied %s because %s", pw->id, mess);
			goto Out;
		}
	}
	conn->write(conn, (uint8_t*)"OK", 2);
	syslog(0, LOG, "AUTH %s", pw->id);

	/* perform operations as asked */
	while((n = readstr(conn, msg)) > 0){
		if((nl = strchr(msg, '\n')) != nil)
			*nl = 0;
		syslog(0, LOG, "[%s] %s", pw->id, msg);

		if(strncmp(msg, "GET ", 4) == 0){
			file = validatefile(msg+4);
			if(file==nil || getfile(conn, pw->id, file) < 0)
				goto Err;

		}else if(strncmp(msg, "PUT ", 4) == 0){
			file = validatefile(msg+4);
			if(file==nil || putfile(conn, pw->id, file) < 0){
				syslog(0, LOG, "failed PUT %s/%s", pw->id, file);
				goto Err;
			}

		}else if(strncmp(msg, "RM ", 3) == 0){
			file = validatefile(msg+3);
			if(file==nil || removefile(conn, pw->id, file) < 0){
				syslog(0, LOG, "failed RM %s/%s", pw->id, file);
				goto Err;
			}

		}else if(strncmp(msg, "CHPASS", 6) == 0){
			if(readstr(conn, msg) < 0){
				syslog(0, LOG, "protocol botch CHPASS for %s", pw->id);
				writerr(conn, "protocol botch while setting PAK");
				goto Out;
			}
			pw->Hi = strtomp(msg, nil, 64, pw->Hi);
			for(i=0; i < 4 && putPW(pw) < 0; i++)
				syslog(0, LOG, "password change failed for %s (%d): %r", pw->id, i);
			if(i==4)
				goto Out;

		}else if(strncmp(msg, "BYE", 3) == 0){
			rv = 0;
			break;

		}else{
			writerr(conn, "unrecognized operation");
			break;
		}

	}
	if(n <= 0)
		syslog(0, LOG, "%s closed connection without saying goodbye", pw->id);

Out:
	freePW(pw);
	conn->free(conn);
	return rv;
Err:
	writerr(conn, "operation failed");
	goto Out;
}

void
main(int argc, char **argv)
{
	int afd, dfd, lcfd, forceSTA = 0;
	char aserve[128], net[128], adir[40], ldir[40];
	char *remote, *serve = "tcp!*!5356", *S = "secstore";
	Ndb *db2;

	setnetmtpt(net, sizeof(net), nil);
	ARGBEGIN{
	case 'R':
		forceSTA = 1;
		break;
	case 's':
		serve = EARGF(usage());
		break;
	case 'S':
		S = EARGF(usage());
		break;
	case 'x':
		setnetmtpt(net, sizeof(net), EARGF(usage()));
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
		sysfatal("%s: %r", aserve);
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
			/*
			 * "/lib/ndb/common.radius does not exist"
			 * if db set before fork.
			 */
			db = ndbopen("/lib/ndb/auth");
			if(db == 0)
				syslog(0, LOG, "no /lib/ndb/auth");
			db2 = ndbopen(0);
			if(db2 == 0)
				syslog(0, LOG, "no /lib/ndb/local");
			db = ndbcat(db, db2);
			if((dfd = accept(lcfd, ldir)) < 0)
				exits("can't accept");
			alarm(30*60*1000);		/* 30 min */
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
