/*
 * Accept new wiki pages or modifications to existing ones via POST method.
 *
 * Talks to the server at /srv/wiki.service.
 */
#include <u.h>
#include <libc.h>
#include <bio.h>
#include "httpd.h"
#include "httpsrv.h"

#define LOG "wiki"

HConnect *hc;
HSPriv *hp;


/* go from possibly-latin1 url with escapes to utf */
char *
_urlunesc(char *s)
{
	char *t, *v, *u;
	Rune r;
	int c, n;

	/* unescape */
	u = halloc(hc, strlen(s)+1);
	for(t = u; c = *s; s++){
		if(c == '%'){
			n = s[1];
			if(n >= '0' && n <= '9')
				n = n - '0';
			else if(n >= 'A' && n <= 'F')
				n = n - 'A' + 10;
			else if(n >= 'a' && n <= 'f')
				n = n - 'a' + 10;
			else
				break;
			r = n;
			n = s[2];
			if(n >= '0' && n <= '9')
				n = n - '0';
			else if(n >= 'A' && n <= 'F')
				n = n - 'A' + 10;
			else if(n >= 'a' && n <= 'f')
				n = n - 'a' + 10;
			else
				break;
			s += 2;
			c = r*16+n;
		}
		*t++ = c;
	}
	*t = 0;

	/* latin1 heuristic */
	v = halloc(hc, UTFmax*strlen(u) + 1);
	s = u;
	t = v;
	while(*s){
		/* in decoding error, assume latin1 */
		if((n=chartorune(&r, s)) == 1 && r == 0x80)
			r = *s;
		s += n;
		t += runetochar(t, &r);
	}
	*t = 0;

	return v;
}

enum
{
	MaxLog		= 100*1024,		/* limit on length of any one log request */
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

char*
unhttp(char *s)
{
	char *p, *r, *w;

	if(s == nil)
		return nil;

	for(p=s; *p; p++)
		if(*p=='+')
			*p = ' ';
	s = _urlunesc(s);

	for(r=w=s; *r; r++){
		if(*r != '\r')
			*w++ = *r;
	}
	*w = '\0';
	return s;
}

void
mountwiki(HConnect *c, char *service)
{
	char buf[64];
	int fd;

	snprint(buf, sizeof buf, "/srv/wiki.%s", service);
	if((fd = open(buf, ORDWR)) < 0){
		syslog(0, LOG, "%s open %s failed: %r", buf, hp->remotesys);
		hfail(c, HNotFound);
		exits("failed");
	}
	if(mount(fd, -1, "/mnt/wiki", MREPL, "") < 0){
		syslog(0, LOG, "%s mount /mnt/wiki failed: %r", hp->remotesys);
		hfail(c, HNotFound);
		exits("failed");
	}
	close(fd);
}

char*
dowiki(HConnect *c, char *title, char *author, char *comment, char *base, ulong version, char *text)
{
	int fd, l, n, err;
	char *p, tmp[256];
int i;

	if((fd = open("/mnt/wiki/new", ORDWR)) < 0){
		syslog(0, LOG, "%s open /mnt/wiki/new failed: %r", hp->remotesys);
		hfail(c, HNotFound);
		exits("failed");
	}

i=0;
	if((i++,fprint(fd, "%s\nD%lud\nA%s (%s)\n", title, version, author, hp->remotesys) < 0)
	|| (i++,(comment && comment[0] && fprint(fd, "C%s\n", comment) < 0))
	|| (i++,fprint(fd, "\n") < 0)
	|| (i++,(text[0] && write(fd, text, strlen(text)) != strlen(text)))){
		syslog(0, LOG, "%s write failed %d %ld fd %d: %r", hp->remotesys, i, strlen(text), fd);
		hfail(c, HInternal);
		exits("failed");
	}

	err = write(fd, "", 0);
	if(err)
		syslog(0, LOG, "%s commit failed %d: %r", hp->remotesys, err);

	seek(fd, 0, 0);
	if((n = read(fd, tmp, sizeof(tmp)-1)) <= 0){
		if(n == 0)
			werrstr("short read");
		syslog(0, LOG, "%s read failed: %r", hp->remotesys);
		hfail(c, HInternal);
		exits("failed");
	}

	tmp[n] = '\0';

	p = halloc(c, l=strlen(base)+strlen(tmp)+40);
	snprint(p, l, "%s/%s/%s.html", base, tmp, err ? "werror" : "index");
	return p;
}


void
main(int argc, char **argv)
{
	Hio *hin, *hout;
	char *s, *t, *p, *f[10];
	char *text, *title, *service, *base, *author, *comment, *url;
	int i, nf;
	ulong version;

	hc = init(argc, argv);
	hp = hc->private;

	if(dangerous(hc->req.uri)){
		hfail(hc, HSyntax);
		exits("failed");
	}

	if(hparseheaders(hc, HSTIMEOUT) < 0)
		exits("failed");
	hout = &hc->hout;
	if(hc->head.expectother){
		hfail(hc, HExpectFail, nil);
		exits("failed");
	}
	if(hc->head.expectcont){
		hprint(hout, "100 Continue\r\n");
		hprint(hout, "\r\n");
		hflush(hout);
	}

	s = nil;
	if(strcmp(hc->req.meth, "POST") == 0){
		hin = hbodypush(&hc->hin, hc->head.contlen, hc->head.transenc);
		if(hin != nil){
			alarm(15*60*1000);
			s = hreadbuf(hin, hin->pos);
			alarm(0);
		}
		if(s == nil){
			hfail(hc, HBadReq, nil);
			exits("failed");
		}
		t = strchr(s, '\n');
		if(t != nil)
			*t = '\0';
	}else{
		hunallowed(hc, "GET, HEAD, PUT");
		exits("unallowed");
	}

	if(s == nil){
		hfail(hc, HNoData, "wiki");
		exits("failed");
	}

	text = nil;
	title = nil;
	service = nil;
	author = "???";
	comment = "";
	base = nil;
	version = ~0;
	nf = getfields(s, f, nelem(f), 1, "&");
	for(i=0; i<nf; i++){
		if((p = strchr(f[i], '=')) == nil)
			continue;
		*p++ = '\0';
		if(strcmp(f[i], "title")==0)
			title = p;
		else if(strcmp(f[i], "version")==0)
			version = strtoul(unhttp(p), 0, 10);
		else if(strcmp(f[i], "text")==0)
			text = p;
		else if(strcmp(f[i], "service")==0)
			service = p;
		else if(strcmp(f[i], "comment")==0)
			comment = p;
		else if(strcmp(f[i], "author")==0)
			author = p;
		else if(strcmp(f[i], "base")==0)
			base = p;
	}

	syslog(0, LOG, "%s post s %s t '%s' v %ld a %s c %s b %s t 0x%p",
		hp->remotesys, service, title, (long)version, author, comment, base, text);

	title = unhttp(title);
	comment = unhttp(comment);
	service = unhttp(service);
	text = unhttp(text);
	author = unhttp(author);
	base = unhttp(base);

	if(title==nil || version==~0 || text==nil || text[0]=='\0' || base == nil 
	|| service == nil || strchr(title, '\n') || strchr(comment, '\n')
	|| dangerous(service) || strchr(service, '/') || strlen(service)>20){
		syslog(0, LOG, "%s failed dangerous", hp->remotesys);
		hfail(hc, HSyntax);
		exits("failed");
	}

	syslog(0, LOG, "%s post s %s t '%s' v %ld a %s c %s",
		hp->remotesys, service, title, (long)version, author, comment);

	if(strlen(text) > MaxLog)
		text[MaxLog] = '\0';

	mountwiki(hc, service);
	url = dowiki(hc, title, author, comment, base, version, text);
	hredirected(hc, "303 See Other", url);
	exits(nil);
}
