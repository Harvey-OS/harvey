#include <alef.h>
#include <bio.h>
#include "httpd.h"

aggr Suffix
{
	Suffix	*next;
	byte	*suffix;
	byte	*generic;
	byte	*specific;
	byte	*encoding;
};

Suffix	*suffixes = nil;

intern	Suffix*		parsesuffix(byte*, Suffix*);
intern	byte*		skipwhite(byte*);
intern	(Content*,Content*)	suffixclass(byte*);
intern	byte*		towhite(byte*);

void
contentinit(void)
{
	Biobuf *b;
	byte *s;

	if(suffixes != nil)
		return;
	b = Bopen("/sys/lib/http", OREAD);
	if(b == nil){
		logit("can't open /sys/lib/http");
		return;
	}
	while((s = b->rdline('\n')) != nil){
		s[b->linelen() - 1] = 0;
		suffixes = parsesuffix(s, suffixes);
	}
	b->term();
}

(Content*, Content*)
classify(byte *name)
{
	Content *type, *enc;

	(type, enc) = suffixclass(name);
	if(type == nil){
		logit("can't classify %s", name);
		type = mkcontent("application", "octet-stream", nil);
	}
	return (type, enc);
}

intern (Content*, Content*)
suffixclass(byte *name)
{
	Suffix *s;
	Content *type, *enc;
	byte buf[NAMELEN+1], *p;

	type = nil;
	enc = nil;
	if((p = strrchr(name, '/')) != nil)
		name = p + 1;
	strncpy(buf, name, NAMELEN);
	buf[NAMELEN] = 0;
	while((p = strrchr(buf, '.')) != nil){
		for(s = suffixes; s; s = s->next){
			if(strcmp(p, s->suffix) == 0){
				if(s->generic)
					type = mkcontent(s->generic, s->specific, type);
				if(s->encoding)
					enc = mkcontent(s->encoding, "", enc);
			}
		}
		*p = 0;
	}
	return (type, enc);
}

intern Suffix*
parsesuffix(byte *line, Suffix *suffix)
{
	Suffix *s;
	byte *suff, *gen, *spec, *enc, *p;

	p = strchr(line, '#');
	if(p != nil)
		*p = 0;
	suff = skipwhite(line);
	p = towhite(suff);
	*p++ = 0;
	gen = skipwhite(p);
	p = towhite(gen);
	*p++ = 0;
	spec = skipwhite(p);
	p = towhite(spec);
	*p++ = 0;
	enc = skipwhite(p);
	p = towhite(enc);
	*p = 0;
	if(*gen == '-')
		*gen = 0;
	if(*spec == '-')
		*spec = 0;
	if(*enc == '-')
		*enc = 0;
	if((*gen == 0 || *spec == 0) && *enc == 0)
		return suffix;

	s = malloc(sizeof *s);
	s->next = suffix;
	s->suffix = strsave(suff);
	s->generic = nil;
	s->specific = nil;
	if(*gen != 0 && *spec != 0){
		s->generic = strsave(gen);
		s->specific = strsave(spec);
	}
	if(*enc != 0)
		s->encoding = strsave(enc);
	return s;
}

intern byte*
skipwhite(byte *s)
{
	int c;

	for(; c = *s; s++)
		if(c != ' ' && c != '\t')
			break;
	return s;
}

intern byte*
towhite(byte *s)
{
	int c;

	for(; c = *s; s++)
		if(c == ' ' || c == '\t')
			break;
	return s;
}

int
checkcontent(Content *me, Content *oks, byte *list, int size)
{
	Content *ok;

	if(oks == nil || me == nil)
		return 1;
	for(ok = oks; ok != nil; ok = ok->next){
		if((strcmp(ok->generic, me->generic) == 0 || strcmp(ok->generic, "*") == 0)
		&& (strcmp(ok->specific, me->specific) == 0 || strcmp(ok->specific, "*") == 0)){
			if(ok->mxb > 0 && size > ok->mxb)
				return 0;
			return 1;
		}
	}

	if(list);
/*
	logit("%s/%s not found", me->generic, me->specific);
	logcontent(list, oks);
*/
	return 1;
}

Content*
mkcontent(byte *generic, byte *specific, Content *next)
{
	Content *c;

	c = malloc(sizeof(Content));
	c->generic = generic;
	c->specific = specific;
	c->next = next;
	c->q = 1;
	c->mxb = 0;
	return c;
}

void
logcontent(byte *name, Content *c)
{
	byte buf[128];
	int n;

	n = 0;
	buf[0] = 0;
	for(; c != nil; c = c->next)
		n += snprint(buf+n, sizeof(buf)-n, "%s/%s ", c->generic, c->specific);
	logit("%s: %s: %s", client, name, buf);
}
