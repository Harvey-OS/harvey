#include <u.h>
#include <libc.h>
#include <bio.h>
#include "httpd.h"

typedef struct Suffix	Suffix;
struct Suffix 
{
	Suffix	*next;
	char	*suffix;
	char	*generic;
	char	*specific;
	char	*encoding;
};

Suffix	*suffixes = nil;

static	Suffix*			parsesuffix(char*, Suffix*);
static	char*			skipwhite(char*);
static	Contents		suffixclass(char*);
static	char*			towhite(char*);

int
updateQid(int fd, Qid *q)
{
	Dir dir;

	if(dirfstat(fd, &dir) < 0)
		sysfatal("can't dirfstat");
	if(q->path == dir.qid.path && q->vers == dir.qid.vers)
		return 0;
	q->path = dir.qid.path;
	q->vers = dir.qid.vers;
	return 1;
}

void
contentinit(void)
{
	static Biobuf *b = nil;
	static Qid qid;
	char *file, *s;
	Suffix *this;

	file = "/sys/lib/mimetype";
	if(b == nil){ /* first time */
		b = Bopen(file, OREAD);
		if(b == nil)
			sysfatal("can't read from %s", file);
		qid.path = qid.vers = 0;
	}
	if(updateQid(Bfildes(b), &qid) == 0)
		return;
	Bseek(b, 0, 0);
	while(suffixes!=nil){
		this = suffixes;
		suffixes = suffixes->next;
		free(this->suffix);
		free(this->generic);
		free(this->specific);
		free(this->encoding);
		free(this);
	}

	while((s = Brdline(b, '\n')) != nil){
		s[Blinelen(b) - 1] = 0;
		suffixes = parsesuffix(s, suffixes);
	}
}

static Suffix*
parsesuffix(char *line, Suffix *suffix)
{
	Suffix *s;
	char *p, *fields[5];
	int i, nf;

	p = strchr(line, '#');
	if(p != nil)
		*p = '\0';
	nf = tokenize(line, fields, 5);
	for(i = 0; i < 4; i++)
		if(i >= nf || fields[i][0] == '-')
			fields[i] = nil;

	if(fields[2] == nil)
		fields[1] = nil;
	if(fields[1] == nil && fields[3] == nil)
		return suffix;

	s = ezalloc(sizeof *s);
	s->next = suffix;
	s->suffix = estrdup(fields[0]);
	if(fields[1] != nil){
		s->generic = estrdup(fields[1]);
		s->specific = estrdup(fields[2]);
	}
	if(fields[3] != nil)
		s->encoding = estrdup(fields[3]);
	return s;
}

/*
 * classify by file name extensions
 */
Contents
uriclass(char *name)
{
	Contents conts;
	Suffix *s;
	Content *type, *enc;
	char buf[3*NAMELEN+1], *p;

	type = nil;
	enc = nil;
	if((p = strrchr(name, '/')) != nil)
		name = p + 1;
	strncpy(buf, name, 3*NAMELEN);
	buf[3*NAMELEN] = 0;
	while((p = strrchr(buf, '.')) != nil){
		for(s = suffixes; s; s = s->next){
			if(strcmp(p, s->suffix) == 0){
				if(s->generic != nil && type == nil)
					type = mkcontent(s->generic, s->specific, nil);
				if(s->encoding != nil && enc == nil)
					enc = mkcontent(s->encoding, nil, nil);
			}
		}
		*p = 0;
	}
	conts.type = type;
	conts.encoding = enc;
	return conts;
}

/*
 * classify by initial contents of file
 */
Contents
dataclass(char *buf, int n)
{
	Contents conts;
	Rune r;
	int c, m;

	for(; n > 0; n -= m){
		c = *buf;
		if(c < Runeself){
			if(c < 32 && c != '\n' && c != '\r' && c != '\t' && c != '\v'){
				conts.type = nil;
				conts.encoding = nil;
				return conts;
			}
			m = 1;
		}else{
			m = chartorune(&r, buf);
			if(r == Runeerror){
				conts.type = nil;
				conts.encoding = nil;
				return conts;
			}
		}
		buf += m;
	}
	conts.type = mkcontent("text", "plain", nil);
	conts.encoding = nil;
	return conts;
}

int
checkcontent(Content *me, Content *oks, char *list, int size)
{
	Content *ok;

	if(oks == nil || me == nil)
		return 1;
	for(ok = oks; ok != nil; ok = ok->next){
		if((cistrcmp(ok->generic, me->generic) == 0 || strcmp(ok->generic, "*") == 0)
		&& (me->specific == nil || cistrcmp(ok->specific, me->specific) == 0 || strcmp(ok->specific, "*") == 0)){
			if(ok->mxb > 0 && size > ok->mxb)
				return 0;
			return 1;
		}
	}

	USED(list);
	if(0){
		fprint(2, "list: %s/%s not found\n", me->generic, me->specific);
		for(; oks != nil; oks = oks->next){
			if(oks->specific)
				fprint(2, "\t%s/%s\n", oks->generic, oks->specific);
			else
				fprint(2, "\t%s\n", oks->generic);
		}
	}
	return 0;
}

Content*
mkcontent(char *generic, char *specific, Content *next)
{
	Content *c;

	c = halloc(sizeof(Content));
	c->generic = generic;
	c->specific = specific;
	c->next = next;
	c->q = 1;
	c->mxb = 0;
	return c;
}
