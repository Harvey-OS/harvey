#include <u.h>
#include <libc.h>
#include <bio.h>
#include "httpd.h"
#include "httpsrv.h"

typedef struct Server	Server;
struct Server 
{
	char *key, *addr;
};

/* !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
   ACHTUNG!  Don't add to this list yourself.
   Leave it to ehg or sean, so that we keep a
   consistent level of security.
   !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! */
Server servtab[] = {
	{ "ehg-echo", "tcp!netlib2.cs.bell-labs.com!566" },
	{ "cmd=auth", "tcp!netlib2.cs.bell-labs.com!568" },
	{ "cmd=wavelet", "tcp!netlib2.cs.bell-labs.com!570" },
	{ "cmd=cs_search", "tcp!netlib2.cs.bell-labs.com!572" },
	{ nil, nil },
};

Biobuf	bserv, bservr;

void
main(int argc, char **argv)
{
	HConnect *c;
	Hio *hout;
	int ch, lastnl, datafd, n_to, n_from;
	char *where, *s;
	Server *serv;

	c = init(argc, argv);
	hout = &c->hout;

	if(c->req.search == nil){
		hfail(c, HSyntax);
		exits("failed");
	}
	logit(c, "to %s", c->req.search);

	/* arrange for timeout */
	alarm(120000);

	/* lookup server and connect */
	where = hstrdup(c, c->req.search);
	s = strchr(where,'&');
	if(s!=nil)
		*s = 0;
	for(serv = servtab; ; serv++){
		if(serv->key == nil){
			hfail(c, HBadSearch, where);
			exits("failed");
		}
		if(strcmp(serv->key,where) == 0)
			break;
	}
	datafd = dial(serv->addr, nil, nil, nil);
	if(datafd < 0){
		hfail(c, HBadSearch, serv->addr);
		exits("failed");
	}
	Binit(&bserv, datafd, OWRITE);

	/* copy the incoming request */
	if(c->req.vermaj == 0){
		Bprint(&bserv, "%s /magic/to%s?%s\r\n", c->req.meth, c->req.uri, c->req.search);
		n_to = 13+strlen(c->req.meth)+strlen(c->req.uri)+strlen(c->req.search);
	}else{
		n_to = Bprint(&bserv, "%s /magic/to%s?%s HTTP/%d.%d\r\n", c->req.meth, c->req.uri, c->req.search, c->req.vermaj, c->req.vermin);
		lastnl = 1;
		while((ch = hgetc(&c->hin)) >= 0){
			if((ch&0x7f) == '\r'){
				Bputc(&bserv, ch); n_to++;
				ch = hgetc(&c->hin);
				if(ch < 0)
					break;
			}
			Bputc(&bserv, ch); n_to++;
			if((ch&0x7f) == '\n'){
				if(lastnl)
					break;
				lastnl = 1;
			}else
				lastnl = 0;
		}
	}
	Bflush(&bserv);

	/* copy reply */
	Binit(&bservr, datafd, OREAD);
	n_from = 0;
	while((ch = Bgetc(&bservr)) != Beof){
		hputc(hout, ch);
		n_from++;
	}
	hflush(hout);
	
	logit(c, "to forwarded %d, returned %d", n_to, n_from);
	exits(nil);
}
