/*
 * Accept new wiki pages or modifications to existing ones via POST method.
 *
 * Talks to the server at /srv/wiki.service.
 */
#include <u.h>
#include <libc.h>
#include <bio.h>
#include "httpd.h"

#define LOG "wiki"

/* go from possibly-latin1 url with escapes to utf */
char *
_urlunesc(char *s)
{
	char *t, *v, *u;
	Rune r;
	int c, n;

	/* unescape */
	u = halloc(strlen(s)+1);
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
	v = halloc(UTFmax*strlen(u) + 1);
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
mountwiki(Connect *c, char *service)
{
	char buf[2*NAMELEN];
	int fd;

	snprint(buf, sizeof buf, "/srv/wiki.%s", service);
	if((fd = open(buf, ORDWR)) < 0){
		syslog(0, LOG, "%s open %s failed: %r", buf, c->remotesys);
		fail(c, NotFound);
		exits("failed");
	}
	if(mount(fd, "/mnt/wiki", MREPL, "") < 0){
		syslog(0, LOG, "%s mount /mnt/wiki failed: %r", c->remotesys);
		fail(c, NotFound);
		exits("failed");
	}
	close(fd);
}

char*
dowiki(Connect *c, char *title, char *author, char *comment, char *base, ulong version, char *text)
{
	int fd, l, n, err;
	char *p, tmp[40];

	if((fd = open("/mnt/wiki/new", ORDWR)) < 0){
		syslog(0, LOG, "%s open /mnt/wiki/new failed: %r", c->remotesys);
		fail(c, NotFound);
		exits("failed");
	}

	if(fprint(fd, "%s\nD%lud\nA%s (%s)\n", title, version, author, c->remotesys) < 0
	|| (comment && comment[0] && fprint(fd, "C%s\n", comment) < 0)
	|| fprint(fd, "\n") < 0
	|| (text[0] && write(fd, text, strlen(text)) != strlen(text))){
		syslog(0, LOG, "%s write failed: %r", c->remotesys);
		fail(c, Internal);
		exits("failed");
	}

	err = write(fd, "", 0);
	if(err)
		syslog(0, LOG, "%s commit failed %d: %r", c->remotesys, err);

	seek(fd, 0, 0);
	if((n = read(fd, tmp, sizeof(tmp)-1)) <= 0){
		syslog(0, LOG, "%s read failed: %r", c->remotesys);
		fail(c, Internal);
		exits("failed");
	}

	tmp[n] = '\0';
	n = atoi(tmp);

	p = halloc(l=strlen(base)+40);
	snprint(p, l, "%s/%d/%s.html", base, n, err ? "werror" : "index");
	return p;
}


void
main(int argc, char **argv)
{
	Connect *c;
	Hio *hin, *hout;
	char *s, *t, *p, *f[10];
	char *text, *title, *service, *base, *author, *comment, *url;
	int i, nf;
	ulong version;

	c = init(argc, argv);

	if(dangerous(c->req.uri)){
		fail(c, Syntax);
		exits("failed");
	}

	if(httpheaders(c) < 0)
		exits("failed");
	hout = &c->hout;
	if(c->head.expectother){
		fail(c, ExpectFail, nil);
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
			alarm(15*60*1000);
			s = hreadbuf(hin, hin->pos);
			alarm(0);
		}
		if(s == nil){
			fail(c, BadReq, nil);
			exits("failed");
		}
		t = strchr(s, '\n');
		if(t != nil)
			*t = '\0';
	}else{
		unallowed(c, "GET, HEAD, PUT");
		exits("unallowed");

	if(s == nil){
		fail(c, NoData, "wiki");
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
		c->remotesys, service, title, (long)version, author, comment, base, text);

	title = unhttp(title);
	comment = unhttp(comment);
	service = unhttp(service);
	text = unhttp(text);
	author = unhttp(author);
	base = unhttp(base);

	if(title==nil || version==~0 || text==nil || text[0]=='\0' || base == nil 
	|| service == nil || strchr(title, '\n') || strchr(comment, '\n')
	|| dangerous(service) || strchr(service, '/') || strlen(service)>NAMELEN){
		syslog(0, LOG, "%s failed dangerous", c->remotesys);
		fail(c, Syntax);
		exits("failed");
	}

	syslog(0, LOG, "%s post s %s t '%s' v %ld a %s c %s",
		c->remotesys, service, title, (long)version, author, comment);

	if(strlen(text) > MaxLog)
		text[MaxLog] = '\0';

	mountwiki(c, service);
	url = dowiki(c, title, author, comment, base, version, text);
	redirected(c, "303 See Other", url);
	exits(nil);
}
