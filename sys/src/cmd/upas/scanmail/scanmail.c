#include "common.h"

#define	whitespace(c)	((c) == ' ' || (c) == '\t')

enum{
	Nomatch		= 0,
	HoldHeader,
	SaveLine,
	Hold,
	Dump,
	Lineoff,

	Nhash		= 128,
	MaxHtml		= 256,

	regexp		= 1,
	string		= 2,

	Hdrsize = 512,		//amount of header checked
	Bufsize = 1024,		//max combined header and body checked
	Bodysize = 512,		//minimum amount of body checked
	Maxread = 128*1024,
};

typedef struct word Word;
typedef struct keyword Keyword;
typedef struct spat Spat;
typedef struct pattern Pattern;

struct word
{
	char	*string;
	int	n;
};

struct	keyword
{
	char*	string;
	int	value;
};

struct	spat
{
	char*	string;
	int	len;
	int	c1;
	Spat*	next;
	Spat*	alt;
};

struct	pattern
{
	Pattern*	next;
	int	treatment;
	int	type;
	Spat*	alt;
	union{
		Reprog*	pat;
		Spat*	spat[Nhash];
	};
};


Keyword	keywords[] =
{
	"header",	HoldHeader,
	"line",		SaveLine,
	"hold",		Hold,
	"dump",		Dump,
	"loff",		Lineoff,
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

int	cflag;
int	debug;
int	hflag;
int	nflag;
int	sflag;
int	tflag;
int	vflag;
Biobuf	bin, bout, *cout;

	/* file names */
char	patfile[128];
char	linefile[128];
char	holdqueue[128];
char	copydir[128];

char	header[Hdrsize+2];
Pattern	*lineoff;
Pattern *patterns;
char	**qname;
char	**qdir;
char	*sender;
String	*recips;

char*	canon(Biobuf*, char*, char*, int*);
int	extract(char*);
int	findkey(char*);
Word*	isword(Word*, char*, int);
int	matcher(Pattern*, char*, Resub*);
int	matchpat(Pattern*, char*, Resub*);
void	parsealt(Biobuf*, char*, Spat**);
void	parsepats(Biobuf*);
Biobuf	*opencopy(char*);
Biobuf	*opendump(char*);
char	*qmail(char**, char*, int, Biobuf*);
void	saveline(char*, char*, Resub*);
void	xprint(char*, Resub*);
int	hash(int);

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
main(int argc, char *argv[])
{
	int i, n, nolines;
	char **args, **a, *cp, *buf;
	char body[Bufsize+2], cmd[1024];
	Resub match[1];
	Pattern *p;
	Biobuf *bp;

	a = args = Malloc((argc+1)*sizeof(char*));
	sprint(patfile, "%s/patterns", UPASLIB);
	sprint(linefile, "%s/lines", UPASLOG);
	sprint(holdqueue, "%s/queue.hold", SPOOL);
	sprint(copydir, "%s/copy", SPOOL);

	*a++ = argv[0];
	for(argc--, argv++; argv[0] && argv[0][0] == '-'; argc--, argv++){
		switch(argv[0][1]){
		case 'c':			/* save copy of message */
			cflag = 1;
			break;
		case 'd':			/* debug */
			debug++;
			*a++ = argv[0];
			break;
		case 'h':			/* queue held messages by sender domain */
			hflag = 1;		/* -q flag must be set also */
			break;
		case 'n':			/* NOHOLD mode */
			nflag = 1;
			break;
		case 'p':			/* pattern file */
			if(argv[0][2] || argv[1] == 0)
				usage();
			argc--;
			argv++;
			strcpy(patfile, *argv);
			break;
		case 'q':			/* queue name */
			if(argv[0][2] ||  argv[1] == 0)
				usage();
			*a++ = argv[0];
			argc--;
			argv++;
			qname = a;
			*a++ = argv[0];
			break;
		case 's':			/* save copy of dumped message */
			sflag = 1;
			break;
		case 't':			/* test mode - don't log match
						 * and write message to /dev/null
						 */
			tflag = 1;
			break;
		case 'v':			/* vebose - print matches */
			vflag = 1;
			break;
		default:
			*a++ = argv[0];
			break;
		}
	}

	if(argc < 3)
		usage();

	Binit(&bin, 0, OREAD);
	bp = Bopen(patfile, OREAD);
	if(bp){
		parsepats(bp);
		Bterm(bp);
	}
	qdir = a;
	sender = argv[2];

		/* copy the rest of argv, acummulating the recipients as we go */
	for(i = 0; argv[i]; i++){
		*a++ = argv[i];
		if(i < 4)	/* skip queue, 'mail', sender, dest sys */
			continue;
			/* recipients and smtp flags - skip the latter*/
		if(strcmp(argv[i], "-g") == 0){
			*a++ = argv[++i];
			continue;
		}
		if(recips)
			s_append(recips, ", ");
		else
			recips = s_new();
		s_append(recips, argv[i]);
	}
	*a = 0;
		/* construct a command string for matching */
	snprint(cmd, sizeof(cmd)-1, "%s %s", sender, s_to_c(recips));
	cmd[sizeof(cmd)-1] = 0;
	for(cp = cmd; *cp; cp++)
		*cp = tolower(*cp);

		/* canonicalize a copy of the header and body.
		 * buf points to orginal message and n contains
		 * number of bytes of original message read during
		 * canonicalization.
		 */
	*body = 0;
	*header = 0;
	buf = canon(&bin, header+1, body+1, &n);
	if (buf == 0)
		exits("read");

	nolines = 0;
	if(lineoff)
		nolines = matchpat(lineoff, cmd, match);

	for(p = patterns; p; p = p->next){
			/* don't apply "Line" patterns to excluded domains */
		if(nolines && p->treatment == SaveLine)
			continue;
			/* apply patterns to the command, header and body */
		if(matcher(p, cmd, match))
			goto out;
		if(matcher(p, header+1, match))
			goto out;
		if(p->treatment == HoldHeader)
			continue;
		if(matcher(p, body+1, match))
			goto out;
	}
	if(cflag)
		cout = opencopy(sender);
out:
	exits(qmail(args, buf, n, cout));
}

char*
qmail(char **argv, char *buf, int n, Biobuf *cout)
{
	Waitmsg status;
	int i, pid, ret, pipefd[2];
	char path[512];
	Biobuf *bp;

	pid = 0;
	if(tflag == 0){
		if(pipe(pipefd) < 0)
			exits("pipe");
		pid = fork();
		if(pid == 0){
			dup(pipefd[0], 0);
			for(i = sysfiles(); i >= 3; i--)
				close(i);
			snprint(path, sizeof(path), "%s/qer", UPASBIN);
			*argv=path;
			exec(path, argv);
			exits("exec");
		}
		Binit(&bout, pipefd[1], OWRITE);
		bp = &bout;
	} else
		bp = Bopen("/dev/null", OWRITE);

	while(n > 0){
		Bwrite(bp, buf, n);
		if(cout)
			Bwrite(cout, buf, n);
		n = Bread(&bin, buf, sizeof(buf)-1);
	}
	Bterm(bp);
	if(cout)
		Bterm(cout);
	if(tflag)
		return 0;

	close(pipefd[1]);
	close(pipefd[0]);
	for(;;){
		ret = wait(&status);
		if(ret < 0 || ret == pid)
			break;
	}
	strcpy(buf, status.msg);
	return buf;
}

int
hash(int c)
{
	return c & 127;
}

int
matcher(Pattern *p, char *message, Resub *m)
{
	char *cp;
	String *s;

	for(cp = message; matchpat(p, cp, m); cp = m->ep){
		switch(p->treatment){
		case SaveLine:
			if(vflag)
				xprint("LINE:", m);
			saveline(linefile, sender, m);
			break;
		case HoldHeader:
		case Hold:
			if(nflag)
				continue;
			if(vflag)
				xprint("HOLD:", m);
			*qdir = holdqueue;
			if(hflag && qname){
				cp = strchr(sender, '!');
				if(cp){
					*cp = 0;
					*qname = strdup(sender);
					*cp = '!';
				} else
					*qname = strdup(sender);
			}
			return 1;
		case Dump:
			if(vflag)
				xprint("DUMP:", m);
			*(m->ep) = 0;
			if(!tflag){
				s = s_new();
				s_append(s, sender);
				s = unescapespecial(s);
				syslog(0, "smtpd", "Dumped %s [%s] to %s", s_to_c(s), m->sp,
					s_to_c(s_restart(recips)));
				s_free(s);
			}
			tflag = 1;
			if(sflag)
				cout = opendump(sender);
			return 1;
		default:
			break;
		}
	}
	return 0;
}

Spat*
isalt(char *message, Spat *alt)
{
	while(alt) {
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
				if(!isalt(message, spat->alt)){
					m->sp = s;
					m->ep = s + spat->len;
					return 1;
				}
			}
		}
		return 0;
	}
	m->sp = m->ep = 0;
	if(regexec(p->pat, message, m, 1) == 0)
		return 0;
	if(isalt(message, p->alt))
		return 0;
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
		if(n <= 0 || *cp == 0)
			continue;

		qp = strstr(cp, "~~");
		if(qp){
			*qp = 0;
			n = strlen(cp);
		}
		if(debug)
			fprint(2, " Pattern: `%s'\n", cp);

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
			if(treatment == Lineoff && lineoff){
				spat->next = lineoff->spat[h];
				lineoff->spat[h] = spat;
				continue;
			}
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
		if(treatment == Lineoff)
			lineoff = new;
		else{
			 if(p)
				p->next = new;
			else
				patterns = new;
			p = new;
		}
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
	int i, c, lastc, nch, len, bufsize, html, ateoh, hsize, nhtml;
	char *p, *buf;
	Word *hp;

	*body = 0;
	*header = 0;
	html = 0;
	buf = 0;
	bufsize = 0;
	hsize = 0;
	ateoh = 0;
	for(;;){			/* accumulate the header */
		p = Brdline(bp, '\n');
		i = Blinelen(bp);
		if(i == 0){			/* eof */
			if(buf == 0)		/* empty file */
				return 0;
			break;
		}
		buf = realloc(buf,bufsize+i+1);
		if(buf == 0)
			exits("malloc");
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
			ateoh = 0;			/* botched header - keep parsing */
		}
		hsize = bufsize;
		if(p && i == 1)			/* end of header - check next line*/
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

	/* read next chunk of body and convert it */

	len = Bread(bp, buf+nch, bufsize-nch);
	if (len < 0)
		len = 0;

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
				if(hp == 0)		/* not html header; handle char normally */
					break;
				nch += hp->n;
				html = 1;
			}
			else {
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

void
saveline(char *file, char *sender, Resub *rp)
{
	char *p, *q;
	int i, c;
	Biobuf *bp;

	if(rp->sp == 0 || rp->ep == 0)
		return;
		/* back up approx 20 characters to whitespace */
	for(p = rp->sp, i = 0; *p && i < 20; i++, p--)
			;
	while(*p && *p != ' ')
		p--;
	p++;

		/* grab about 20 more chars beyond the end of the match */
	for(q = rp->ep, i = 0; *q && i < 20; i++, q++)
			;
	while(*q && *q != ' ')
		q++;

	c = *q;
	*q = 0;
	bp = sysopen(file, "al", 0644);
	if(bp){
		Bprint(bp, "%s-> %s\n", sender, p);
		Bterm(bp);
	}
	else if(debug)
		fprint(2, "can't save line: (%s) %s\n", sender, p);
	*q = c;
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

Biobuf*
opendump(char *sender)
{
	int i;
	ulong h;
	char buf[512];
	Biobuf *b;
	char *cp;

	cp = ctime(time(0));
	cp[7] = 0;
	cp[10] = 0;
	if(cp[8] == ' ')
		sprint(buf, "%s/queue.dump/%s%c", SPOOL, cp+4, cp[9]);
	else
		sprint(buf, "%s/queue.dump/%s%c%c", SPOOL, cp+4, cp[8], cp[9]);
	cp = buf+strlen(buf);
	if(access(buf, 0) < 0 && sysmkdir(buf, 0777) < 0)
		return 0;

	h = 0;
	while(*sender)
		h = h*257 + *sender++;
	for(i = 0; i < 50; i++){
		h += lrand();
		sprint(cp, "/%lud", h);
		b = sysopen(buf, "wlc", 0600);
		if(b){
			if(vflag)
				fprint(2, "saving in %s\n", buf);
			return b;
		}
	}
	return 0;
}

Biobuf*
opencopy(char *sender)
{
	int i;
	ulong h;
	char buf[512];
	Biobuf *b;

	h = 0;
	while(*sender)
		h = h*257 + *sender++;
	for(i = 0; i < 50; i++){
		h += lrand();
		sprint(buf, "%s/%lud", copydir, h);
		b = sysopen(buf, "wlc", 0600);
		if(b)
			return b;
	}
	return 0;
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
