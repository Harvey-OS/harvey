// dervied from /sys/src/cmd/pq.c on 13oct01
#include <u.h>
#include <libc.h>
#include <ctype.h>
#include <bio.h>
#include "httpd.h"
#include "httpsrv.h"

// list here URL prefixes that are valid to outside world
char *urlpref[] = {
	"http://cm.bell-labs.com/",
	"http://plan9.bell-labs.com/",
	"http://www.bell-labs.com/",
	0
};

void bib_fmt(char*,char*);
void index_fmt(char*,char*);
void no_fmt(char*,char*);
int send(HConnect*);

Hio *hout;

char *things[] = {
	"name",
	"bg",
	"id",
	"hpurl",
	"dpt",
	"dbu",
};

typedef struct Entry Entry;
struct Entry
{
	char *thing[nelem(things)];
};

int
postreply(int fd, char *buf, int len)
{
	int n, sofar;
	char buf2[1];

	werrstr("who cares");
	for(sofar = 0; sofar < len;){
		n = read(fd, buf + sofar, len - sofar);
		if(n <= 0){
			hprint(hout, "premature EOF %d: %r\n", n);
			return -1;
		}
		sofar += n;
		if(buf[sofar - 1] == '>'){
			return sofar - 1;
		}
	}

	/* response too long, truncate */
	for(;;){
		n = read(fd, buf2, 1);
		if(n <= 0){
			hprint(hout, "long read & premature EOF\n");
			return -1;
		}
		if(*buf2 == '>')
			break;
	}
	return sofar;
}

int
writepost(int fd, char *string)
{
	char buf[2048];

	if(fprint(fd, "w|%s\n", string) < 0)
		return -1;
	return postreply(fd, buf, sizeof(buf));
}

int
readpost(int fd, char *buf, int len)
{
	if(fprint(fd, "r\n") < 0)
		return -1;
	return postreply(fd, buf, len);
}

Entry*
parse(char *buf)
{
	int i, n;
	char *fields[nelem(things)+2];
	Entry *e;

	n = getfields(buf, fields, nelem(fields), 0, "|");
	if(n != nelem(things)+2)
		return nil;
	e = malloc(sizeof(*e));
	for(i = 0; i < nelem(things); i++)
		e->thing[i] = strdup(fields[i+1]);
	return e;
}

enum
{
	Ttelno,
	Tname,
	Tattr,
	Tid,
	Torg,
};

void
mkquestion(char *q, int n, char *name, int type)
{
	int i;
	char *e;

	e = q + n;
	for(i = 0; i < nelem(things); i++)
		q = seprint(q, e, "%s|", things[i]);
	switch(type){
	case Ttelno:
		seprint(q, e, "tel=%s", name);
		break;
	case Tname:
		if(strchr(name, '.') == nil)
			seprint(q, e, "name=.%s", name);
		else
			seprint(q, e, "name=%s", name);
		break;
	case Torg:
		seprint(q, e, "dpt=%s", name);
		break;
	case Tattr:
		seprint(q, e, "%s", name);
		break;
	case Tid:
		seprint(q, e, "id=%s", name);
		break;
	}
}

char*
f(Entry *e, char *t)
{
	int i;

	for(i = 0; i < nelem(things); i++)
		if(strcmp(things[i], t) == 0)
			return e->thing[i];
	return "???";
}

char*
canon(char *p)
{
	static char name[1024];
	char *q;

	q = strrchr(p, '_');
	if(q != nil){
		*q++ = 0;
		snprint(name, sizeof(name), "%s %s", p, q);
	} else
		snprint(name, sizeof(name), "%s", p);
	for(q = name; *q; q++)
		if(*q == '_')
			*q = ' ';
	return name;
}

int
external_page(char *u)
{
	char **p = urlpref;

	while(*p){
		if(strncmp(u,*p,strlen(*p))==0)
			return 1;
		p++;
	}
	return 0;
}

void
printentry(Entry *e)
{
	char *s;

	hprint(hout, "<BR>%s \n", canon(f(e, "name")));
	s = f(e, "id");
	if(*s != 0)
		hprint(hout, " &lt;<A HREF=\"mailto:%s@lucent.com\">%s@lucent.com</A>&gt;\n", s, s);
	s = f(e, "hpurl");
	if(*s  && external_page(s))
		hprint(hout, "  <A HREF=\"%s\">%s</A>\n", s, s);
	else
		hprint(hout, "  <SMALL>No external web page listed in POST.</SMALL>\n");
}

/*
 *   try a lookup, print answers
 */
try(int fd, char *name, int type, int ignoreidmatch)
{
	int i, n;
	char question[2048];
	char buf[2048];
	Entry *e;

	mkquestion(question, sizeof question, name, type);

	i = 0;
	if(writepost(fd, question) != 0)
		return -1;

	/* read all replies */
	for(;;){
		n = readpost(fd, buf, sizeof(buf));
		if(n == 0)
			break;
		if(n < 0)
			break;
		if(*buf == '?')
			break;
		e = parse(buf);
		if(e)
		if( strcmp("SK32000000",f(e,"dbu")) == 0 )
		if(!ignoreidmatch || strcmp(f(e, "id"), name) != 0){
			printentry(e);
			i++;
		}
		if(i>=10){
			hprint(hout, "<br><b>...</b>\n");
			break;
		}
	}

	return i;
}

void
ask(int fd, char *name)
{
	char buf[2048];

	if(fd < 0){
		hprint(hout, "error dialing post server: %r\n");
		return;
	}
		
	/* get initial prompt */
	alarm(15*1000);
	if(postreply(fd, buf, sizeof(buf)) != 0)
		return;
	alarm(0);

	if(try(fd, name, Tid, 0) < 0)
		return;
	if(try(fd, name, Tname, 1) < 0)
		return;
}

static char simplechar[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789.,-";

int
send(HConnect *c)
{
	int fd, n;
	char *s;
	HSPairs *q;

	if(strcmp(c->req.meth, "GET") != 0 && strcmp(c->req.meth, "HEAD") != 0)
		return hunallowed(c, "GET, HEAD");
	if(c->head.expectother || c->head.expectcont)
		return hfail(c, HExpectFail, nil);
	if(c->req.search == nil || !*c->req.search)
		return hfail(c, HNoData, "post_find");
	s = c->req.search;
	for(q = hparsequery(c, hstrdup(c, c->req.search)); q; q = q->next)
		if(strcmp(q->s, "name") == 0)
			s = q->t;

	if(c->req.vermaj){
		hokheaders(c);
		hprint(hout, "Content-type: text/html\r\n");
		hprint(hout, "\r\n");
	}
	if(strcmp(c->req.meth, "HEAD") == 0){
		writelog(c, "Reply: 200 post_find 0\n");
		hflush(hout);
		exits(nil);
	}

	// discourage malicous queries
	if(*s==0 || (n=strlen(s))>50 || strspn(s,simplechar)!=n){
		hprint(hout, "query not acceptable");
		writelog(c, "Reply: 200 post_find badquery %s\n", s);
		hflush(hout);
		exits(nil);
	}

	hprint(hout, "<HEAD><TITLE>%s</TITLE></HEAD>\r\n<BODY>\r\n", s);
	hprint(hout, "Bell Labs directory search for: %s<BR>\r\n", s);

	unmount(0, "/net.alt");			/* only look inside */
	fd = dial("tcp!$post!411", 0, 0, 0);
	logit(c, "post_find %s", s);
	ask(fd, s);
	close(fd);

	hprint(hout, "<BR><BR><SMALL><A HREF=\"mailto:webmaster@plan9.bell-labs.com\">webmaster</A>\r\n");
	hprint(hout, "<BR>Please do not ask us about people not in this directory; we have no other information to give out.");
	hprint(hout, "</SMALL>\r\n</BODY>\r\n");
	hflush(hout);
	writelog(c, "Reply: 200 post_find %ld %ld\n", hout->seek, hout->seek);
	return 1;
}

/********** main() calls httpheadget() calls send() ************/
void
main(int argc, char **argv)
{
	HConnect *c;

	c = init(argc, argv);
	hout = &c->hout;
	if(hparseheaders(c, HSTIMEOUT) >= 0)
		send(c);
	exits(nil);
}

