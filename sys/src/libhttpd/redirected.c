/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#include <u.h>
#include <libc.h>
#include <bin.h>
#include <httpd.h>

int
hredirected(HConnect *c, char *how, char *uri)
{
	Hio *hout;
	char *s, *ss, *scheme, *host;
	char sayport[NETPATHLEN];
	int n;

	scheme = c->scheme? c->scheme: "http";
	host = c->head.host;
	if(strchr(uri, ':') != nil)
		host = "";
	else if(uri[0] != '/'){
		s = strrchr(c->req.uri, '/');
		if(s != nil)
			*s = '\0';
		ss = halloc(c, strlen(c->req.uri) + strlen(uri) + 2 + UTFmax);
		sprint(ss, "%s/%s", c->req.uri, uri);
		uri = ss;
		if(s != nil)
			*s = '/';
	}

	if((strcmp(scheme, "http") == 0 && atoi(c->port) == 80) ||
	   (strcmp(scheme, "https") == 0 && atoi(c->port) == 443) ||
	    strchr(host, ':') != nil)
		sayport[0] = '\0';
	else
		snprint(sayport, sizeof sayport, ":%s", c->port);

	n = snprint(c->xferbuf, HBufSize, 
			"<head><title>Redirection</title></head>\r\n"
			"<body><h1>Redirection</h1>\r\n"
			"Your selection can be found <a href=\"%U\"> here</a>.<p></body>\r\n", uri);

	hout = &c->hout;
	hprint(hout, "%s %s\r\n", hversion, how);
	hprint(hout, "Date: %D\r\n", time(nil));
	hprint(hout, "Server: Plan9\r\n");
	hprint(hout, "Content-type: text/html\r\n");
	hprint(hout, "Content-Length: %d\r\n", n);
	if(host == nil || host[0] == 0)
		hprint(hout, "Location: %U\r\n", uri);
	else
		hprint(hout, "Location: %s://%U%s%U\r\n",
			scheme, host, sayport, uri);
	if(c->head.closeit)
		hprint(hout, "Connection: close\r\n");
	else if(!http11(c))
		hprint(hout, "Connection: Keep-Alive\r\n");
	hprint(hout, "\r\n");

	if(strcmp(c->req.meth, "HEAD") != 0)
		hwrite(hout, c->xferbuf, n);

	if(c->replog) {
		if(host == nil || host[0] == 0)
			c->replog(c, "Reply: %s\nRedirect: %U\n", how, uri);
		else
			c->replog(c, "Reply: %s\nRedirect: %s://%U%s%U\n",
				how, scheme, host, sayport, uri);
	}
	return hflush(hout);
}

int
hmoved(HConnect *c, char *uri)
{
	return hredirected(c, "301 Moved Permanently", uri);
}
