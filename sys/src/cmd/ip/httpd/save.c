/*
 * for GET or POST to /magic/save/foo.
 * add incoming data to foo.data.
 * send foo.html as reply.
 *
 * supports foo.data with "exclusive use" mode to prevent interleaved saves.
 * thus http://cm.bell-labs.com/magic/save/t?args should access:
 * -lrw-rw--w- M 21470 ehg web 1533 May 21 18:19 /usr/web/save/t.data
 * --rw-rw-r-- M 21470 ehg web   73 May 21 18:17 /usr/web/save/t.html
*/
#include <u.h>
#include <libc.h>
#include <bio.h>
#include "httpd.h"
#include "httpsrv.h"

enum
{
	MaxLog		= 24*1024,		/* limit on length of any one log request */
	LockSecs	= MaxLog/500,		/* seconds to wait before giving up on opening the data file */
};

static int
dangerous(char *s)
{
	if(s == nil)
		return 1;

	/*
	 * This check shouldn't be needed;
	 * filename folding is already supposed to have happened.
	 * But I'm paranoid.
	 */
	while(s = strchr(s,'/')){
		if(s[1]=='.' && s[2]=='.')
			return 1;
		s++;
	}
	return 0;
}

/*
 * open a file which might be locked.
 * if it is, spin until available
 */
int
openLocked(char *file, int mode)
{
	char buf[ERRMAX];
	int tries, fd;

	for(tries = 0; tries < LockSecs*2; tries++){
		fd = open(file, mode);
		if(fd >= 0)
			return fd;
		errstr(buf, sizeof buf);
		if(strstr(buf, "locked") == nil)
			break;
		sleep(500);
	}
	return -1;
}

void
main(int argc, char **argv)
{
	HConnect *c;
	Dir *dir;
	Hio *hin, *hout;
	char *s, *t, *fn;
	int n, nfn, datafd, htmlfd;

	c = init(argc, argv);

	if(dangerous(c->req.uri)){
		hfail(c, HSyntax);
		exits("failed");
	}

	if(hparseheaders(c, HSTIMEOUT) < 0)
		exits("failed");
	hout = &c->hout;
	if(c->head.expectother){
		hfail(c, HExpectFail, nil);
		exits("failed");
	}
	if(c->head.expectcont){
		hprint(hout, "100 Continue\r\n");
		hprint(hout, "\r\n");
		hflush(hout);
	}

	s = nil;
	if(strcmp(c->req.meth, "POST") == 0){
		hin = hbodypush(&c->hin, c->head.contlen, c->head.transenc);
		if(hin != nil){
			alarm(HSTIMEOUT);
			s = hreadbuf(hin, hin->pos);
			alarm(0);
		}
		if(s == nil){
			hfail(c, HBadReq, nil);
			exits("failed");
		}
		t = strchr(s, '\n');
		if(t != nil)
			*t = '\0';
	}else if(strcmp(c->req.meth, "GET") != 0 && strcmp(c->req.meth, "HEAD") != 0){
		hunallowed(c, "GET, HEAD, PUT");
		exits("unallowed");
	}else
		s = c->req.search;
	if(s == nil){
		hfail(c, HNoData, "save");
		exits("failed");
	}

	if(strlen(s) > MaxLog)
		s[MaxLog] = '\0';
	n = snprint(c->xferbuf, HBufSize, "at %ld %s\n", time(0), s);


	nfn = strlen(c->req.uri) + 64;
	fn = halloc(c, nfn);

	/*
	 * open file descriptors & write log line
	 */
	snprint(fn, nfn, "/usr/web/save/%s.html", c->req.uri);
	htmlfd = open(fn, OREAD);
	if(htmlfd < 0 || (dir = dirfstat(htmlfd)) == nil){
		hfail(c, HNotFound, c->req.uri);
		exits("failed");
		return;
	}

	snprint(fn, nfn, "/usr/web/save/%s.data", c->req.uri);
	datafd = openLocked(fn, OWRITE);
	if(datafd < 0){
		errstr(c->xferbuf, sizeof c->xferbuf);
		if(strstr(c->xferbuf, "locked") != nil)
			hfail(c, HTempFail, c->req.uri);
		else
			hfail(c, HNotFound, c->req.uri);
		exits("failed");
	}
	seek(datafd, 0, 2);
	write(datafd, c->xferbuf, n);
	close(datafd);

	sendfd(c, htmlfd, dir, hmkcontent(c, "text", "html", nil), nil);

	exits(nil);
}
