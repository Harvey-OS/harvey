#include <u.h>
#include <libc.h>
#include <bio.h>
#include "httpd.h"
#include "httpsrv.h"

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
static	HContents		suffixclass(char*);
static	char*			towhite(char*);

int
updateQid(int fd, Qid *q)
{
	Dir *dir;
	Qid dq;

	dir = dirfstat(fd);
	if(dir == nil)
		sysfatal("can't dirfstat");
	dq = dir->qid;
	free(dir);
	if(q->path == dq.path && q->vers == dq.vers && q->type == dq.type)
		return 0;
	*q = dq;
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
	if(fields[0] == nil)
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
HContents
uriclass(HConnect *hc, char *name)
{
	HContents conts;
	Suffix *s;
	HContent *type, *enc;
	char *buf, *p;

	type = nil;
	enc = nil;
	if((p = strrchr(name, '/')) != nil)
		name = p + 1;
	buf = hstrdup(hc, name);
	while((p = strrchr(buf, '.')) != nil){
		for(s = suffixes; s; s = s->next){
			if(strcmp(p, s->suffix) == 0){
				if(s->generic != nil && type == nil)
					type = hmkcontent(hc, s->generic, s->specific, nil);
				if(s->encoding != nil && enc == nil)
					enc = hmkcontent(hc, s->encoding, nil, nil);
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
HContents
dataclass(HConnect *hc, char *buf, int n)
{
	HContents conts;
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
	conts.type = hmkcontent(hc, "text", "plain", nil);
	conts.encoding = nil;
	return conts;
}
