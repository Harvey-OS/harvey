#include <u.h>
#include <libc.h>
#include <bio.h>
#include <ip.h>
#include <plumb.h>
#include <thread.h>
#include <fcall.h>
#include <9p.h>
#include "dat.h"
#include "fns.h"

char PostContentType[] = "application/octet-stream";
int httpdebug;

typedef struct HttpState HttpState;
struct HttpState
{
	int fd;
	Client *c;
	char *location;
	char *setcookie;
	char *netaddr;
	Ibuf	b;
};

static void
location(HttpState *hs, char *value)
{
	if(hs->location == nil)
		hs->location = estrdup(value);
}

static void
contenttype(HttpState *hs, char *value)
{
	if(hs->c->contenttype == nil)
		hs->c->contenttype = estrdup(value);
}

static void
setcookie(HttpState *hs, char *value)
{
	char *s, *t;
	Fmt f;

	s = hs->setcookie;
	fmtstrinit(&f);
	if(s)
		fmtprint(&f, "%s", s);
	fmtprint(&f, "set-cookie: ");
	fmtprint(&f, "%s", value);
	fmtprint(&f, "\n");
	t = fmtstrflush(&f);
	if(t){
		free(s);
		hs->setcookie = t;
	}
}

struct {
	char *name;									/* Case-insensitive */
	void (*fn)(HttpState *hs, char *value);
} hdrtab[] = {
	{ "location:", location },
	{ "content-type:", contenttype },
	{ "set-cookie:", setcookie },
};

static int
httprcode(HttpState *hs)
{
	int n;
	char *p;
	char buf[256];

	n = readline(&hs->b, buf, sizeof(buf)-1);
	if(n <= 0)
		return n;
	if(httpdebug)
		fprint(2, "-> %s\n", buf);
	p = strchr(buf, ' ');
	if(memcmp(buf, "HTTP/", 5) != 0 || p == nil){
		werrstr("bad response from server");
		return -1;
	}
	buf[n] = 0;
	return atoi(p+1);
}
 
/*
 *  read a single mime header, collect continuations.
 *
 *  this routine assumes that there is a blank line twixt
 *  the header and the message body, otherwise bytes will
 *  be lost.
 */
static int
getheader(HttpState *hs, char *buf, int n)
{
	char *p, *e;
	int i;

	n--;
	p = buf;
	for(e = p + n; ; p += i){
		i = readline(&hs->b, p, e-p);
		if(i < 0)
			return i;

		if(p == buf){
			/* first line */
			if(strchr(buf, ':') == nil)
				break;		/* end of headers */
		} else {
			/* continuation line */
			if(*p != ' ' && *p != '\t'){
				unreadline(&hs->b, p);
				*p = 0;
				break;		/* end of this header */
			}
		}
	}

	if(httpdebug)
		fprint(2, "-> %s\n", buf);
	return p-buf;
}

static int
httpheaders(HttpState *hs)
{
	char buf[2048];
	char *p;
	int i, n;

	for(;;){
		n = getheader(hs, buf, sizeof(buf));
		if(n < 0)
			return -1;
		if (n == 0)
			return 0;
//print("http header: '%.*s'\n", n, buf);
		for(i = 0; i < nelem(hdrtab); i++){
			n = strlen(hdrtab[i].name);
			if(cistrncmp(buf, hdrtab[i].name, n) == 0){
				/* skip field name and leading white */
				p = buf + n;
				while(*p == ' ' || *p == '\t')
					p++;
				(*hdrtab[i].fn)(hs, p);
				break;
			}
		}
	}
	return 0;
}

int
httpopen(Client *c, Url *url)
{
	int fd, code, redirect;
	char *cookies;
	Ioproc *io;
	HttpState *hs;

	if(httpdebug)
		fprint(2, "httpopen\n");
	io = c->io;
	hs = emalloc(sizeof(*hs));
	hs->c = c;
	hs->netaddr = estrdup(netmkaddr(url->host, 0, url->scheme));
	c->aux = hs;
	if(httpdebug)
		fprint(2, "dial %s\n", hs->netaddr);
	fd = io->dial(io, hs->netaddr, 0, 0, 0, url->ischeme==UShttps);
	if(fd < 0){
	Error:
		if(httpdebug)
			fprint(2, "io->dial: %r\n");
		free(hs->netaddr);
		free(hs);
		close(hs->fd);
			hs->fd = -1;
		c->aux = nil;
		return -1;
	}
	hs->fd = fd;
	if(httpdebug)
		fprint(2, "<- %s %s HTTP/1.0\n<- Host: %s\n",
			c->havepostbody? "POST": " GET", url->http.page_spec, url->host);
	io->print(io, fd, "%s %s HTTP/1.0\r\nHost: %s\r\n",
		c->havepostbody? "POST" : "GET", url->http.page_spec, url->host);
	if(httpdebug)
		fprint(2, "<- User-Agent: %s\n", c->ctl.useragent);
	if(c->ctl.useragent)
		io->print(io, fd, "User-Agent: %s\r\n", c->ctl.useragent);
	if(c->ctl.sendcookies){
		/* should we use url->page here?  sometimes it is nil. */
		cookies = httpcookies(url->host, url->http.page_spec, 0);
		if(cookies && cookies[0])
			io->print(io, fd, "%s", cookies);
		if(httpdebug)
			fprint(2, "<- %s", cookies);
		free(cookies);
	}
	if(c->havepostbody){
		io->print(io, fd, "Content-type: %s\r\n", PostContentType);
		io->print(io, fd, "Content-length: %ud\r\n", c->npostbody);
		if(httpdebug){
			fprint(2, "<- Content-type: %s\n", PostContentType);
			fprint(2, "<- Content-length: %ud\n", c->npostbody);
		}
	}
	io->print(io, fd, "\r\n");
	if(c->havepostbody)
		if(io->write(io, fd, c->postbody, c->npostbody) != c->npostbody)
			goto Error;

	c->havepostbody = 0;
	redirect = 0;
	initibuf(&hs->b, io, fd);
	code = httprcode(hs);

	switch(code){
	case -1:	/* connection timed out */
		goto Error;

/*
	case Eof:
		werrstr("EOF from HTTP server");
		goto Error;
*/

	case 200:	/* OK */
	case 201:	/* Created */
	case 202:	/* Accepted */
	case 204:	/* No Content */
#ifdef NOT_DEFINED
		if(ofile == nil && r->start != 0)
			sysfatal("page changed underfoot");
#endif
		break;

	case 206:	/* Partial Content */
		werrstr("Partial Content (206)");
		goto Error;

	case 301:	/* Moved Permanently */
	case 302:	/* Moved Temporarily */
		redirect = 1;
		break;

	case 304:	/* Not Modified */
		break;

	case 400:	/* Bad Request */
		werrstr("Bad Request (400)");
		goto Error;

	case 401:	/* Unauthorized */
	case 402:	/* ??? */
		werrstr("Unauthorized (401,402)");
		goto Error;

	case 403:	/* Forbidden */
		werrstr("Forbidden by server (403)");
		goto Error;

	case 404:	/* Not Found */
		werrstr("Not found on server (404)");
		goto Error;

	case 500:	/* Internal server error */
		werrstr("Server choked (500)");
		goto Error;

	case 501:	/* Not implemented */
		werrstr("Server can't do it (501)");
		goto Error;

	case 502:	/* Bad gateway */
		werrstr("Bad gateway (502)");
		goto Error;

	case 503:	/* Service unavailable */
		werrstr("Service unavailable (503)");
		goto Error;
	
	default:
		/* Bogus: we should treat unknown code XYZ as code X00 */
		werrstr("Unknown response code %d", code);
		goto Error;
	}

	if(httpheaders(hs) < 0)
		goto Error;
	if(c->ctl.acceptcookies && hs->setcookie)
		httpsetcookie(hs->setcookie, url->host, url->path);
	if(redirect){
		if(!hs->location){
			werrstr("redirection without Location: header");
			return -1;
		}
		c->redirect = hs->location;
		hs->location = nil;
	}
	return 0;
}

int
httpread(Client *c, Req *r)
{
	char *dst;
	HttpState *hs;
	int n;
	long rlen, tot, len;

	hs = c->aux;
	dst = r->ofcall.data;
	len = r->ifcall.count;
	tot = 0;
	while (tot < len){
		rlen = len - tot;
		n = readibuf(&hs->b, dst + tot, rlen);
		if(n == 0)
			break;
		else if(n < 0){
			if(tot == 0)
				return -1;
			else
				return tot;
		}
		tot += n;
	}
	r->ofcall.count = tot;
	return 0;
}

void
httpclose(Client *c)
{
	HttpState *hs;

	hs = c->aux;
	c->io->close(c->io, hs->fd);
	hs->fd = -1;
	free(hs->location);
	free(hs->setcookie);
	free(hs->netaddr);
	free(hs);
	c->aux = nil;
}

