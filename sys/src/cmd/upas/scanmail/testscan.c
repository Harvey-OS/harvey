#include "common.h"

#define	whitespace(c)	((c) == ' ' || (c) == '\t')

enum{
	HoldHeader	= 1,
	SaveLine,
	Hold,
	Dump,

	Nhash		= 128,
	MaxHtml		= 256,

	regexp		= 1,
	string		= 2,

	Hdrsize = 512,		//amount of header checked
	Bufsize = 1024,		//max combined header and body checked
	Bodysize = 512,		//minimum amount of body checked
	Maxread		= 128*1024,
};

struct word{
	char	*string;
	int	n;
};

struct	keyword{
	char	*string;
	int	value;
};

typedef struct spat Spat;
struct	spat
{
	char*	string;
	int	len;
	int	c1;
	Spat*	next;
	Spat*	alt;
};

struct	pattern{
	struct	pattern *next;
	int	treatment;
	int	type;
	Spat*	alt;
	union{
		Reprog*	pat;
		Spat*	spat[Nhash];
	};
};

typedef struct pattern Pattern;
typedef struct keyword Keyword;
typedef struct word Word;

Pattern *patterns;
Keyword	keywords[] =
{
	"header",	HoldHeader,
	"line",		SaveLine,
	"hold",		Hold,
	"dump",		Dump,
	0,		0,
};

Word	htmlcmds[] =
{
	"html",		4,
	"!doctype html", 13,
	0,

};

Word	hrefs[] =
{
	"a href=",	7,
	"img src=",	8,
	"img border=",	11,
	0,

};

Word	hdrwords[] =
{
	"cc:",			3,
	"bcc:", 		4,
	"to:",			3,
	0,			0,

};

int debug;
Biobuf	bin, bout;
int	html;
char	patfile[128], header[Bufsize+2];

char*	canon(Biobuf*, char*, char*, int*);
int	extract(char*);
int	findkey(char*);
Word*	isword(Word*, char*, int);
int	matchpat(Pattern*, char*, Resub*);
void	parsealt(Biobuf*, char*, Spat**);
void	parsepats(Biobuf*);
void	xprint(char*, Resub*);

void
usage(void)
{
	fprint(2, "missing or bad arguments to qer\n");
	exits("usage");
}

void *
Malloc(long n)
{
	void *p;

	p = malloc(n);
	if(p == 0)
		exits("malloc");
	return p;
}

void
dumppats(void)
{
	int i;
	Pattern *p;
	Spat *s, *q;

	for (p = patterns; p; p = p->next){
		if(p->type == regexp){
			print("Pattern = <REGEXP>\n");
			for(s = p->alt; s; s = s->next)
				print("\t%s\n", s->string);
		} else {
			for(i = 0; i < Nhash; i++){
				for(s = p->spat[i]; s; s = s->next){
					print("Pattern %s\n", s->string);
					for(q = s->alt; q; q = q->next)
						print("\t%s\n", q->string);

				}
			}
		}	
			
	}
}

void
main(int argc, char *argv[])
{
	int fd, n, aflag, vflag;
	char body[Bufsize+2], *cp, *raw;
	Resub match[1];
	Pattern *p;
	Biobuf *bp;

	sprint(patfile, "%s/patterns", UPASLIB);
	aflag = -1;
	vflag = 0;
	ARGBEGIN {
	case 'a':
		aflag = 1;
		break;
	case 'v':
		vflag = 1;
		break;
	case 'd':
		debug++;
		break;
	case 'p':
		strcpy(patfile,ARGF());
		break;
	} ARGEND

	bp = Bopen(patfile, OREAD);
	if(bp){
		parsepats(bp);
		Bterm(bp);
	}

	if(argc >= 1){
		fd = open(*argv, OREAD);
		if(fd < 0){
			fprint(2, "can't open %s\n", *argv);
			exits("open");
		}
		Binit(&bin, fd, OREAD);
	} else 
		Binit(&bin, 0, OREAD);

	*body = 0;
	*header = 0;
	for(;;){
		raw = canon(&bin, header+1, body+1, &n);
		if(raw == 0)
			break;
		if(vflag){
			fprint(2, "\t**** Header ****\n\n%s\n", header+1);
			fprint(2, "\t**** Body ****\n\n%s\n", body+1);
		}
		if(aflag == 0)
			continue;
		if(aflag < 0)
			aflag = 0;
		if(0)
			print("%s\n", body+1);

		for(p = patterns; p; p = p->next){
			cp = header+1;
			while(*cp && matchpat(p, cp, match))
				cp = match[0].ep;
			if(p->treatment == HoldHeader)
				continue;
			cp = body+1;
			while(*cp && matchpat(p, cp, match))
				cp = match[0].ep;
		}
	}
	exits(0);
}

int
hash(int c)
{
	return c & 127;
}

Spat*
isalt(char *message, Spat *alt)
{
	while(alt){
		if(strstr(header+1, alt->string))
			break;
		if(strstr(message, alt->string))
			break;
		alt = alt->next;
	}
	return alt;
}

int
matchpat(Pattern *p, char *message, Resub *m)
{
	Spat *spat;
	char *s;
	int c, c1;

	if(p->type == string){
		c1 = *message;
		for(s=message; c=c1; s++){
			c1 = s[1];
			for(spat=p->spat[hash(c)]; spat; spat=spat->next){
				if(c1 == spat->c1)
				if(memcmp(s, spat->string, spat->len) == 0)
				if(isalt(message, spat->alt) == 0){
					m->sp = s;
					m->ep = s + spat->len;
					goto Match;
				}
			}
		}
		return 0;
	}
	m->sp = m->ep = 0;
	if (regexec(p->pat, message, m, 1) == 0)
		return 0;
	if(isalt(message, p->alt))
		return 0;

Match:
	switch(p->treatment){
	case SaveLine:
		xprint("LINE:", m);
		break;
	case HoldHeader:
	case Hold:
		xprint("HOLD:", m);
		break;
	case Dump:
		xprint("DUMP:", m);
		break;
	default:
		break;
	}
	return 1;
}

void
parsepats(Biobuf *bp)
{
	Pattern *p, *q, *new;
	char *cp, *qp;
	int type, treatment, n, h;
	Spat *spat;

	p = 0;
	for(;;){
		cp = Brdline(bp, '\n');
		if(cp == 0)
			break;
		cp[Blinelen(bp)-1] = 0;
		while(*cp == ' ' || *cp == '\t')
			cp++;
		if(*cp == '#' || *cp == 0)
			continue;
		type = regexp;
		if(*cp == '*'){
			type = string;
			cp++;
		}
		qp = strchr(cp, ':');
		if(qp == 0)
			continue;
		*qp = 0;
		if(debug)
			fprint(2, "treatment = %s", cp);
		treatment = findkey(cp);
		cp = qp+1;
		n = extract(cp);
		if(debug)
			fprint(2, " Pattern: `%s'\n", cp);
		if(n <= 0 || *cp == 0)
			continue;
		qp = strstr(cp, "~~");
		if(qp){
			*qp = 0;
			n = strlen(cp);
		}

		if(type == regexp) {
			new = Malloc(sizeof(Pattern));
			new->alt = 0;
			new->pat = regcomp(cp);
			if(new->pat == 0){
				free(new);
				continue;
			}
			if(qp)
				parsealt(bp, qp+2, &new->alt);
		}else{
			spat = Malloc(sizeof(*spat));
			spat->next = 0;
			spat->alt = 0;
			spat->len = n;
			spat->string = Malloc(n+1);
			spat->c1 = cp[1];
			strcpy(spat->string, cp);

			if(qp)
				parsealt(bp, qp+2, &spat->alt);

			h = hash(*spat->string);
			for(q=patterns; q; q=q->next) {
				if(q->type == type && q->treatment == treatment) {
					spat->next = q->spat[h];
					q->spat[h] = spat;
					break;
				}
			}
			if(q)
				continue;
			new = Malloc(sizeof(Pattern));
			new->alt = 0;
			memset(new, 0, sizeof(*new));
			new->spat[h] = spat;
		}
		new->treatment = treatment;
		new->type = type;
		new->next = 0;
		if(p)
			p->next = new;
		else
			patterns = new;
		p = new;
	}
}

void
parsealt(Biobuf *bp, char *cp, Spat** head)
{
	char *p;
	Spat *alt;

	while(cp){
		if(*cp == 0){		/*escaped newline*/
			do{
				cp = Brdline(bp, '\n');
				if(cp == 0)
					return;
				cp[Blinelen(bp)-1] = 0;
			} while(extract(cp) <= 0 || *cp == 0);
		}

		p = cp;
		cp = strstr(p, "~~");
		if(cp){
			*cp = 0;
			cp += 2;
		}
		if(strlen(p)){
			alt = Malloc(sizeof(*alt));
			alt->string = strdup(p);
			alt->next = *head;
			*head = alt;
		}
	}
}

int
extract(char *cp)
{
	int c;
	char *p, *q, *r;

	p = q = r = cp;
	while(whitespace(*p))
		p++;
	while(c = *p++){
		if (c == '#')
			break;
		if(c == '"'){
			while(*p && *p != '"'){
				if(*p == '\\' && p[1] == '"')
					p++;
				if('A' <= *p && *p <= 'Z')
					*q++ = *p++ + ('a'-'A');
				else
					*q++ = *p++;
			}
			if(*p)
				p++;
			r = q;		/* never back up over a quoted string */
		} else {
			if('A' <= c && c <= 'Z')
				c += ('a'-'A');
			*q++ = c;
		}
	}
	while(q > r && whitespace(q[-1]))
		q--;
	*q = 0;
	return q-cp;
}

char*
canon(Biobuf *bp, char *header, char *body, int *n)
{
	int i, c, lastc, nch, len, ateoh, hsize, nhtml;
	char *p;
	Word *hp;

	static char *buf;
	static int bufsize;

	*body = 0;
	*header = 0;
	hsize = 0;
	if(buf == 0){				/* first time - read the header */
		ateoh = 0;
		for(;;){
			p = Brdline(bp, '\n');
			i = Blinelen(bp);
			if(i == 0){			/* eof */
				if(buf == 0)		/* empty file */
					return 0;
				break;
			}
			buf = realloc(buf,bufsize+i+1);
			if(buf == 0){
				fprint(2, "malloc\n");
				exits("malloc");
			}
			if(p)
				strncpy(buf+bufsize, p, i);		/* regular line */
			else
				i = Bread(bp, buf+bufsize, i);		/* super-long line */
			hsize = bufsize;
			bufsize += i;
			buf[bufsize] = 0;

				/* check for botched header */
			if(ateoh){
				if(isword(hdrwords, buf+hsize, 0) == 0)	/* good header */
					break;
				ateoh = 0;		/* botched header - parse some more */
			}
			hsize = bufsize;
			if(p && i == 1)			/* end of header - read one more line*/
				ateoh = 1;
			if(bufsize >= Maxread)		/* super long header */
				break;
		}
			/* look at first Hdrsize bytes of header */
		lastc = ' ';
		len = hsize;
		if(len > Hdrsize)
			len = Hdrsize;
		for(nch = 0; nch < len; nch++){
			c = buf[nch];
			switch(c){
			case 0:
			case '\r':
				continue;
			case '\t':
			case ' ':
			case '\n':
				if(lastc == ' ')
					continue;
				c = ' ';
				break;
			default:
				if('A' <= c && c <= 'Z')
					c += ('a'-'A');
				break;
			}
			*header++ = c;
			lastc = c;
		}
		*header = 0;
		if(p == 0){
			*n = bufsize;
			return buf;
		}
				/* make room for body */
		nch = bufsize;
		if(hsize+Bodysize < Bufsize)
			bufsize = Bufsize;
		else
			bufsize += Bodysize-(nch-hsize);
		buf = realloc(buf, bufsize);
		if(buf == 0){
			fprint(2, "realloc body %d\n", bufsize);
			exits("realloc");
		}
	} else {
		nch = 0;
		if(bufsize > Bufsize)		/* limit subsequent reads of the body */
			bufsize = Bufsize;
	}

	/* read next chunk of body and convert it */
	len = Bread(bp, buf+nch, bufsize-nch);
	if (len <= 0)
		if(nch == 0)
			return 0;
		else {
			*n = nch;
			return buf;
		}

		/* now convert the body */
	lastc = ' ';
	len += nch;
	nch = hsize;
	while(nch < len){
		p = buf+nch++;
		c = *p++;
		switch(c){
		case 0:
		case '\r':
			continue;
		case '\t':
		case ' ':
		case '\n':
			if(lastc == ' ')
				continue;
			c = ' ';
			break;
		case '=':		/* check for some stupid escape seqs. */
			if(*p == '\n'){
				nch++;
				continue;
			}
			if(*p == '2'){
				if(tolower(p[1]) == 'e'){
					nch += 2;
					c = '.';
				}else
				if(tolower(p[1]) == 'f'){
					nch += 2;
					c = '/';
				}else
				if(p[1] == '0'){
					nch += 2;
					if(lastc == ' ')
						continue;
					c = ' ';
				}
			} else
			if(*p == '3' && tolower(p[1]) == 'd'){
				nch += 2;
				c = '=';
			}
			break;
		case '<':
			if(html == 0){
				hp = isword(htmlcmds, p, '>');
				if(hp == 0)		/* not htmp header; handle char normally */
					break;
				nch += hp->n;
				html = 1;
			} else {
				hp = isword(hrefs, p, '>');
				if(hp){
					nch += hp->n;	/* extract HREF address  */
					break;
				}
			}
			if (nch+MaxHtml > len)
				nhtml = len;		/* consume rest of chars */
			else
				nhtml = nch+MaxHtml;	/* consume MaxHtml chars */
			for (i = nch; i < nhtml; i++)	/* consume HTML */
				if(buf[i] == '>')
					break;
			if(i >= nhtml)			/* too long or partial */
				break;
			nch = i+1;			/* bump over HTML command */
			continue;
		default:
			if('A' <= c && c <= 'Z')
				c += ('a'-'A');
			break;
		}
		*body++ = c;
		lastc = c;
	}
	*body = 0;
	*n = nch;
	return buf;
}

Word
*isword(Word *wp, char *cp, int termchar)
{
	int i, c, lastc;
	char buf[1024];

	i = lastc = 0;
	while (*cp && *cp != termchar && i < sizeof(buf)){
		c = *cp++;
		if(c == '\n' || c == ' ' || c == '\t'){
			if(lastc == ' ')
				continue;
			c = ' ';
		} else if('A' <= c && c <= 'Z')
			c += ('a'-'A');
		buf[i++] = lastc = c;
	}
	for(;wp->string; wp++)
		if(i >= wp->n && strncmp(buf, wp->string, wp->n) == 0)
			return wp;
	return 0;
}

int
findkey(char *val)
{
	Keyword *kp;

	for(kp = keywords; kp->string; kp++)
		if(strcmp(val, kp->string) == 0)
				break;
	return kp->value;
}

void
xprint(char *type, Resub *m)
{
	char *p, *q;
	int i;

	if(m->sp == 0 || m->ep == 0)
		return;

		/* back up approx 30 characters to whitespace */
	for(p = m->sp, i = 0; *p && i < 30; i++, p--)
			;
	while(*p && *p != ' ')
		p--;
	p++;

		/* grab about 30 more chars beyond the end of the match */
	for(q = m->ep, i = 0; *q && i < 30; i++, q++)
			;
	while(*q && *q != ' ')
		q++;

	fprint(2, "%s %.*s~%.*s~%.*s\n", type, (int)(m->sp-p), p, (int)(m->ep-m->sp), m->sp, (int)(q-m->ep), m->ep);
}
