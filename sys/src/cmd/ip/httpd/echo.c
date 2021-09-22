#include <u.h>
#include <libc.h>
#include "httpd.h"
#include "httpsrv.h"

// htmlquote
//   escape < as &lt; to avoid cross-site scripting
//   escape > as &gt; because some browsers "helpfully" insert < when seeing a >
//   <META ... charset=ISO-8859-1"> because UTF-7 has other ways to write <
//   and escape & as &amp; for appearance sake
// caller should free
static char *
hq(char *text)
{
	int textlen = strlen(text), escapedlen = textlen;
	char *escaped, *s, *w;

	for(s = text; *s; s++)
		if(*s=='<' || *s=='>' || *s=='&')
			escapedlen += 4;
	escaped = ezalloc(escapedlen+1);
	for(s = text, w = escaped; *s; s++){
		if(*s == '<'){
			strcpy(w, "&lt;");
			w += 4;
		}else if(*s == '>'){
			strcpy(w, "&gt;");
			w += 4;
		}else if(*s == '&'){
			strcpy(w, "&amp;");
			w += 5;
		}else{
			*w++ = *s;
		}
	}
	return escaped;
}

static void
hqwrite(Hio *hout, char *s, int n)
{
	char *buf = ezalloc(n+1), *esc;

	memcpy(buf, s, n);
	buf[n] = 0;
	esc = hq(buf);
	hwrite(hout, esc, strlen(esc));
	free(esc);
	free(buf);
}

void
main(int argc, char **argv)
{
	HConnect *c;
	HSPriv *hp;
	Hio *hout, *hin;
	char *s;

	c = init(argc, argv);
	if(hparseheaders(c, 15*60*1000) < 0)
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
	if(c->req.vermaj){
		hokheaders(c);
		hprint(hout, "Content-type: text/html\r\n");
		hprint(hout, "\r\n");
	}
	hprint(hout, "<HEAD><META http-equiv=\"Content-Type\" content=\"text/html; charset=ISO-8859-1\"><TITLE>Echo</TITLE></HEAD><BODY><h1>Echo</h1>\r\n");
	hp = c->private;
	hprint(hout, "You connected from sys %s, serv %s\r\n",
		hp->remotesys, hp->remoteserv);
	hprint(hout, "<p>You requested a %s, version HTTP/%d.%d on %s",
		hq(c->req.meth), c->req.vermaj, c->req.vermin, hq(c->req.uri));
	if(c->req.search)
		hprint(hout, " with search string %s", hq(c->req.search));
	hprint(hout, ".\r\n");

	hprint(hout, "<p>Your client sent the following headers:<p><PRE>\r\n");
	hqwrite(hout, (char*)(c->header), c->hstop - c->header);

	if(strcmp(c->req.meth, "POST") == 0){
		hin = hbodypush(&c->hin, c->head.contlen, c->head.transenc);
		if(hin == nil)
			hprint(hout, "<p>With an invalid or unsupported body length specification.<p>");
		else{
			hprint(hout, "with POSTed data:<p><PRE>\n'");
			alarm(15*60*1000);
			for(; s = hreadbuf(hin, hin->pos); hin->pos = hin->stop)
				hqwrite(hout, s, hbuflen(hin, s));
			alarm(0);
			hprint(hout, "'\n</PRE>");
		}
	}

	hprint(hout, "</PRE></BODY>\n");
	hflush(hout);
	writelog(c, "Reply: 200 echo %ld %ld\n", hout->seek, hout->seek);
	exits(nil);
}
