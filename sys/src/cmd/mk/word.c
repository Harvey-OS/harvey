#include	"mk.h"

static char *nextword(char*, char**, char**);

Word *
newword(char *s)
{
	Word *w = (Word *)Malloc(sizeof(Word));

	w->s = strdup(s);
	w->next = 0;
	return(w);
}

Word *
stow(char *s)
{
	int save;
	char *start, *end;
	Word *head, *w;

	w = head = 0;
	while(*s){
		s = nextword(s, &start, &end);
		if(*start == 0)
			break;
		save = *end;
		*end = 0;
		if (w) {
			w->next = newword(start);
			w = w->next;
		} else
			head = w = newword(start);
		*end = save;
	}
	if (!head)
		head = newword("");
	return(head);
}

char *
wtos(Word *w)
{
	Bufblock *buf;
	char *cp;

	buf = newbuf();
	wtobuf(w, buf);
	insert(buf, 0);
	cp = strdup(buf->start);
	freebuf(buf);
	return(cp);
}

void
wtobuf(Word *w, Bufblock *buf)
{
	char *cp;

	for(; w; w = w->next){
		for(cp = w->s; *cp; cp++)
			insert(buf, *cp);
		if(w->next)
			insert(buf, ' ');
	}
}

void
delword(Word *w)
{
	free(w->s);
	free((char *)w);
}

void
dumpw(char *s, Word *w)
{
	Bprint(&stdout, "%s", s);
	for(; w; w = w->next)
		Bprint(&stdout, " '%s'", w->s);
	Bputc(&stdout, '\n');
}

static char *
nextword(char *s, char **start, char **end)
{
	char *to;
	Rune r, q;
	int n;

	while (*s && SEP(*s))		/* skip leading white space */
		s++;
	to = *start = s;
	while (*s) {
		n = chartorune(&r, s);
		if (SEP(r)) {
			if (to != *start)	/* we have data */
				break;
			s += n;		/* null string - keep looking */
			while (*s && SEP(*s))	
				s++;
			to = *start = s;
		} else if (QUOTE(r)) {
			q = r;
			s += n;		/* skip leading quote */
			while (*s) {
				n = chartorune(&r, s);
				if (r == q) {
					if (s[1] != '\'')
						break;
					s++;	/* embedded quote */
				}
				while (n--)
					*to++ = *s++;
			}
			if (!*s)		/* no trailing quote */
				break;
			s++;			/* skip trailing quote */ 
		} else  {
			while (n--)
				*to++ = *s++;
		}
	}
	*end = to;
	return s;
}
