#include	"mk.h"

char *infile;
int inline;
static int rhead(char *, Word **, Word **, int *, char **);
static char *rbody(Biobuf*);
extern Word *target1;

void
parse(char *f, int fd, int varoverride, int ruleoverride)
{
	int hline;
	char *body;
	Word *head, *tail;
	int attr, set;
	char *prog, *inc;
	int newfd;
	Biobuf in;
	Bufblock *buf;

	if(fd < 0){
		perror(f);
		Exit();
	}
	ipush();
	infile = strdup(f);
	inline = 1;
	Binit(&in, fd, OREAD);
	buf = newbuf();
	inc = 0;
	while(assline(&in, buf)){
		hline = inline;
		switch(rhead(buf->start, &head, &tail, &attr, &prog))
		{
		case '<':
			if((tail == 0) || ((inc = wtos(tail)) == 0)){
				SYNERR(-1);
				fprint(2, "missing include file name\n");
				Exit();
			}
			if((newfd = open(inc, 0)) < 0){
				fprint(2, "warning: skipping missing include file: ");
				perror(inc);
			} else
				parse(inc, newfd, 0, 1);
			break;
		case ':':
			body = rbody(&in);
			addrules(head, tail, body, attr, hline, ruleoverride, prog);
			break;
		case '=':
			if(head->next){
				SYNERR(-1);
				fprint(2, "multiple vars on left side of assignment\n");
				Exit();
			}
			if(symlook(head->s, S_OVERRIDE, (char *)0)){
				set = varoverride;
				symdel(head->s, S_OVERRIDE);
			} else {
				set = 1;
				if(varoverride)
					symlook(head->s, S_OVERRIDE, "");
			}
			if(set){
/*
char *cp;
dumpw("tail", tail);
cp = wtos(tail); print("assign %s to %s\n", head->s, cp); free(cp);
*/
				setvar(head->s, (char *) tail);
				symlook(head->s, S_WESET, "");
			}
			if(attr)
				symlook(head->s, S_NOEXPORT, "");
			break;
		default:
			SYNERR(hline);
			fprint(2, "expected one of :<=\n");
			Exit();
			break;
		}
	}
	close(fd);
	freebuf(buf);
	ipop();
}

void
addrules(Word *head, Word *tail, char *body, int attr, int hline, int override, char *prog)
{
	Word *w;

	assert("addrules args", head && body);
	if((target1 == 0) && !(attr&REGEXP))
		frule(head);
	for(w = head; w; w = w->next)
		addrule(w->s, tail, body, head, attr, hline, override, prog);
}

static int
rhead(char *line, Word **h, Word **t, int *attr, char **prog)
{
	char *p;
	char *pp;
	int sep;
	Rune r;
	int n;
	Word *w;

	p = charin(line,":=<");
	if(p == 0)
		return('?');
	sep = *p;
	*p++ = 0;
	*attr = 0;
	*prog = 0;
/*print("SEP: %c\n", sep);*/
	if(sep == '='){
		pp = charin(p, "'= \t");
		if (pp && *pp == '=') {
			while (p != pp) {
				n = chartorune(&r, p);
				switch(r)
				{
				default:
					SYNERR(-1);
					fprint(2, "unknown attribute '%c'\n",*p);
					Exit();
				case 'U':
					*attr = 1;
					break;
				}
				p += n;
			}
			p++;		/* skip trailing '=' */
		}
	}
	if((sep == ':') && *p && (*p != ' ') && (*p != '\t')){
		while (*p) {
			n = chartorune(&r, p);
			if (r == ':')
				break;
			p += n;
			switch(r)
			{
			default:
				SYNERR(-1);
				fprint(2, "unknown attribute '%c'\n", p[-1]);
				Exit();
			case '<':
				*attr |= RED;
				break;
			case 'D':
				*attr |= DEL;
				break;
			case 'E':
				*attr |= NOMINUSE;
				break;
			case 'n':
				*attr |= NOVIRT;
				break;
			case 'N':
				*attr |= NOREC;
				break;
			case 'P':
				pp = charin(p, ":");
				if (pp == 0 || *pp == 0)
					goto eos;
				*pp = 0;
				*prog = strdup(p);
				*pp = ':';
				p = pp;
				break;
			case 'Q':
				*attr |= QUIET;
				break;
			case 'R':
				*attr |= REGEXP;
				break;
			case 'U':
				*attr |= UPD;
				break;
			case 'V':
				*attr |= VIR;
				break;
			}
		}
		if (*p++ != ':') {
	eos:
			SYNERR(-1);
			fprint(2, "missing trailing :\n");
			Exit();
		}
	}
	*h = w = stow(line);
	if(!*w->s) {
		if (sep != '<') {
			SYNERR(inline-1);
			fprint(2, "no var on left side of assignment/rule\n");
			Exit();
		}
	}
	*t = stow(p);
	return(sep);
}

static char *
rbody(Biobuf *in)
{
	Bufblock *buf;
	int r, lastr;
	char *p;

	lastr = '\n';
	buf = newbuf();
	for(;;){
		r = Bgetrune(in);
		if (r < 0)
			break;
		if (lastr == '\n') {
			if (r == '#')
				rinsert(buf, r);
			else if (r != ' ' && r != '\t') {
				Bungetrune(in);
				break;
			}
		} else
			rinsert(buf, r);
		lastr = r;
		if (r == '\n')
			inline++;
	}
	insert(buf, 0);
	p = strdup(buf->start);
	freebuf(buf);
	return p;
}

struct input
{
	char *file;
	int line;
	struct input *next;
};
static struct input *inputs = 0;

void
ipush(void)
{
	struct input *in, *me;

	me = (struct input *)Malloc(sizeof(*me));
	me->file = infile;
	me->line = inline;
	me->next = 0;
	if(inputs == 0)
		inputs = me;
	else {
		for(in = inputs; in->next; )
			in = in->next;
		in->next = me;
	}
}

void
ipop(void)
{
	struct input *in, *me;

	assert("pop input list", inputs != 0);
	if(inputs->next == 0){
		me = inputs;
		inputs = 0;
	} else {
		for(in = inputs; in->next->next; )
			in = in->next;
		me = in->next;
		in->next = 0;
	}
	infile = me->file;
	inline = me->line;
	free((char *)me);
}
