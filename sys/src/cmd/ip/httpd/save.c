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
	char buf[ERRLEN];
	int tries, fd;

	for(tries = 0; tries < LockSecs*2; tries++){
		fd = open(file, mode);
		if(fd >= 0)
			return fd;
		errstr(buf);
		if(strstr(buf, "locked") == nil)
			break;
		sleep(500);
	}
	return -1;
}

void
main(int argc, char **argv)
{
	Connect *c;
	Dir dir;
	Hio *hin, *hout;
	char *s, *t, *fn;
	int n, nfn, datafd, htmlfd;

	c = init(argc, argv);

	if(dangerous(c->req.uri))
		fail(c, Syntax);

	httpheaders(c);
	hout = &c->hout;
	if(c->head.expectother)
		fail(c, ExpectFail, nil);
	if(c->head.expectcont){
		hprint(hout, "100 Continue\r\n");
		hprint(hout, "\r\n");
		hflush(hout);
	}

	s = nil;
	if(strcmp(c->req.meth, "POST") == 0){
		hin = hbodypush(&c->hin, c->head.contlen, c->head.transenc);
		if(hin != nil){
			alarm(15*60*1000);
			s = hreadbuf(hin, hin->pos);
			alarm(0);
		}
		if(s == nil)
			fail(c, BadReq, nil);
		t = strchr(s, '\n');
		if(t != nil)
			*t = '\0';
	}else if(strcmp(c->req.meth, "GET") != 0 && strcmp(c->req.meth, "HEAD") != 0)
		unallowed(c, "GET, HEAD, PUT");
	else
		s = c->req.search;
	if(s == nil)
		fail(c, NoData, "save");

	if(strlen(s) > MaxLog)
		s[MaxLog] = '\0';
	n = snprint(c->xferbuf, BufSize, "at %ld %s\n", time(0), s);


	nfn = strlen(c->req.uri) + 4 * NAMELEN;
	fn = halloc(nfn);

	/*
	 * open file descriptors & write log line
	 */
	snprint(fn, nfn, "/usr/web/save/%s.html", c->req.uri);
	htmlfd = open(fn, OREAD);
	if(htmlfd < 0 || dirfstat(htmlfd, &dir) < 0)
		fail(c, NotFound, c->req.uri);

	snprint(fn, nfn, "/usr/web/save/%s.data", c->req.uri);
	datafd = openLocked(fn, OWRITE);
	if(datafd < 0){
		errstr(c->xferbuf);
		if(strstr(c->xferbuf, "locked") != nil)
			fail(c, TempFail, c->req.uri);
		fail(c, NotFound, c->req.uri);
	}
	seek(datafd, 0, 2);
	write(datafd, c->xferbuf, n);
	close(datafd);

	sendfd(c, htmlfd, &dir);

	exits(nil);
}
