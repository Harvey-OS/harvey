#include <u.h>
#include <libc.h>
#include <auth.h>
#include "httpd.h"
#include "httpsrv.h"

static char*	readfile(char*);

/*
 * these should be done better; see the reponse codes in /lib/rfc/rfc2616 for
 * more info on what should be included.
 */
#define UNAUTHED	"You are not authorized to see this area.\n"

/*
 * check for authorization for some parts of the server tree
 * the user name supplied with the authorization request is ignored;
 * instead, we authenticate as the realm's user.
 *
 * authorization should be done before opening any files so that
 * unauthorized users don't get to validate file names.
 *
 * returns 1 if authorized, 0 if unauthorized, -1 for io failure.
 */
int
authorize(HConnect *c, char *file)
{
	char *p, *p0;
	Hio *hout;
	char *buf;
	int i, n;
	char *t[257];

	p0 = halloc(c, strlen(file)+STRLEN("/.httplogin")+1);
	strcpy(p0, file);
	for(;;){
		p = strrchr(p0, '/');
		if(p == nil)
			return hfail(c, HInternal);
		if(*(p+1) != 0)
			break;

		/* ignore trailing '/'s */
		*p = 0;
	}
	strcpy(p, "/.httplogin");

	buf = readfile(p0);
	if(buf == nil){
		return 1;
	}
	n = tokenize(buf, t, nelem(t));
	
	if(c->head.authuser != nil && c->head.authpass != 0){
		for(i = 1; i+1 < n; i += 2){
			if(strcmp(t[i], c->head.authuser) == 0
			&& strcmp(t[i+1], c->head.authpass) == 0){
				free(buf);
				return 1;
			}
		}
	}

	hout = &c->hout;
	hprint(hout, "%s 401 Unauthorized\r\n", hversion);
	hprint(hout, "Server: Plan9\r\n");
	hprint(hout, "Date: %D\r\n", time(nil));
	hprint(hout, "WWW-Authenticate: Basic realm=\"%s\"\r\n", t[0]);
	hprint(hout, "Content-Type: text/html\r\n");
	hprint(hout, "Content-Length: %d\r\n", STRLEN(UNAUTHED));
	if(c->head.closeit)
		hprint(hout, "Connection: close\r\n");
	else if(!http11(c))
		hprint(hout, "Connection: Keep-Alive\r\n");
	hprint(hout, "\r\n");
	if(strcmp(c->req.meth, "HEAD") != 0)
		hprint(hout, "%s", UNAUTHED);
	writelog(c, "Reply: 401 Unauthorized\n");
	free(buf);
	return hflush(hout);
}

static char*
readfile(char *file)
{
	Dir *d;
	int fd;
	char *buf;
	int n, len;

	fd = open(file, OREAD);
	if(fd < 0)
		return nil;
	d = dirfstat(fd);
	if(d == nil){		/* shouldn't happen */
		close(fd);
		return nil;
	}
	len = d->length;
	free(d);

	buf = malloc(len+1);
	if(buf == 0){
		close(fd);
		return nil;
	}

	n = readn(fd, buf, len);
	close(fd);
	if(n <= 0){
		free(buf);
		return nil;
	}
	buf[n] = '\0';
	return buf;
}
