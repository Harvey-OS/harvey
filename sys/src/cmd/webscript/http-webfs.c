#include <u.h>
#include <libc.h>
#include <bio.h>
#include <draw.h>
#include <regexp.h>
#include <html.h>
#include <ctype.h>
#include "dat.h"

Bytes*
httpget(char *url, char *post)
{
	int ctlfd, fd, n;
	Waitmsg *w;
	Bytes *b;
	char buf[4096], *base, *mtpt;
	
	if(verbose){
		fprint(2, "%s %s\n", post ? "post" : "get", url);
		if(post)
			fprint(2, "%s\n", post);
	}
	
	if((ctlfd = open("/mnt/web/clone", ORDWR)) < 0)
		sysfatal("open /mnt/web/clone: %r");
	if((n = read(ctlfd, buf, sizeof buf-1)) < 0)
		sysfatal("reading /mnt/web/clone: %r");
	if(n == 0)
		sysfatal("short read on /mnt/web/clone");
	buf[n] = 0;
	n = atoi(buf);
	
	if(fprint(ctlfd, "url %s", url) <= 0)
		sysfatal("url ctl write: %r");
	
	if(post){
		snprint(buf, sizeof buf, "/mnt/web/%d/postbody", n);
		if((fd = open(buf, OWRITE)) < 0)
			sysfatal("open %s: %r", buf);
		if(write(fd, post, strlen(post)) < 0)
			sysfatal("write postbody: %r");
		close(fd);
	}
	
	snprint(buf, sizeof buf, "/mnt/web/%d/body", n);
	if((fd = open(buf, OREAD)) < 0)
		sysfatal("open body: %r");

	b = emalloc(sizeof(Bytes));
	while((n = read(fd, buf, sizeof buf)) > 0)
		growbytes(b, buf, n);
	if(n < 0)
		sysfatal("reading body: %r");
	close(fd);
	return b;
}

