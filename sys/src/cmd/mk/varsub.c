#include	"mk.h"

static void	varcopy(char*, Bufblock*);

void
subsub(Word *val, char *ext, Bufblock *buf)
{
	char *s;
	char *a, *b, *c, *d;
	int na, nb, nc, nd;

	/* prepare literals */
	a = ext;
	s = charin(a, "=%&");
	if (!s) {
		na = strlen(a);
		s = a+na;
	} else
		na = s-a;
	b = s;			/* s guaranteed to be on utf boundary */
	if(PERCENT(*s)) {
		b++;
		s = charin(b, "=");
		if (!s) {
			nb = strlen(b);
			s = b+nb;
		} else
			nb = s-b;
	} else
		nb = 0;
	c = s;
	if (*s) {
		c++;		/* skip '=' */
		s = charin(c, "&%");
		if (!s) {
			nc = strlen(c);
			s = c+nc;
		} else
			nc = s-c;
	} else
		nc = 0;
	d = s;
	if(PERCENT(*s))
		nd = strlen(d+1);
	else
		nd = 0;

		/* break into words, do sub 
		 * assume all separators are unique in utf
		 */
	for(; val; val = val->next){
		if (!val->s || !val->s[0])
			continue;
		s = val->s+strlen(val->s);
		/* substitute in val..s */
		if((memcmp(val->s, a, na) == 0) && (memcmp(s-nb, b, nb) == 0)){
			if (nc)
				bufcpy(buf, c, nc);
			if (PERCENT(*d))
				bufcpy(buf, val->s+na, s-nb-val->s+na);
			if (nd)
				bufcpy(buf, d+1, nd);
		} else
			bufcpy(buf, val->s, s-val->s);
		insert(buf, ' ');
	}
	return;
}

void
varmatch(char *var, Bufblock *buf)
{
	Symtab *sym;
	Word *w;

	sym = symlook(var, S_VAR, (char *)0);
	if (sym){
		w = (Word *) sym->value;
		while (w) {
			varcopy(w->s, buf);
			w = w->next;
			if (w && w->s && w->s[0])
				insert(buf, ' ');
		}
	}
}

static void
varcopy(char *s, Bufblock *buf)
{
	Rune r;

	if (!s)
		return;
	while (*s) {
		s += chartorune(&r, s);
			/* prevent further evaluation special chars*/
		if (QUOTE(r) || r == '=') {
			insert(buf, '\'');
			rinsert(buf, r);
			insert(buf, '\'');
		} else
			rinsert(buf, r);
	}
}
