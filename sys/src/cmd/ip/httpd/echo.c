#include <u.h>
#include <libc.h>
#include "httpd.h"

void
main(int argc, char **argv)
{
	Connect *c;
	Hio *hout, *hin;
	char *s;

	c = init(argc, argv);
	httpheaders(c);
	hout = &c->hout;
	if(c->head.expectother)
		fail(c, ExpectFail, nil);
	if(c->head.expectcont){
		hprint(hout, "100 Continue\r\n");
		hprint(hout, "\r\n");
		hflush(hout);
	}
	if(c->req.vermaj){
		okheaders(c);
		hprint(hout, "Content-type: text/html\r\n");
		hprint(hout, "\r\n");
	}
	hprint(hout, "<head><title>Echo</title></head>\r\n");
	hprint(hout, "<body><h1>Echo</h1>\r\n");
	hprint(hout, "You requested a %s, version HTTP/%d.%d on %s", c->req.meth, c->req.vermaj, c->req.vermin, c->req.uri);
	if(c->req.search)
		hprint(hout, " with search string %s", c->req.search);
	hprint(hout, ".\n");

	hprint(hout, "Your client sent the following headers:<p><pre>\n");
	hwrite(hout, c->header, c->hstop - c->header);

	if(strcmp(c->req.meth, "POST") == 0){
		hin = hbodypush(&c->hin, c->head.contlen, c->head.transenc);
		if(hin == nil)
			hprint(hout, "<p>With an invalid or unsupported body length specification.<p>");
		else{
			hprint(hout, "with POSTed data:<p><pre>\n'");
			alarm(15*60*1000);
			for(; s = hreadbuf(hin, hin->pos); hin->pos = hin->stop)
				hwrite(hout, s, hbuflen(hin, s));
			alarm(0);
			hprint(hout, "'\n</pre>");
		}
	}

	hprint(hout, "</pre></body>\n");
	exits(nil);
}
