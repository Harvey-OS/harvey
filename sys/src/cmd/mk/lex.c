#include	"mk.h"

static int initdone = 0;

static int bquote(Biobuf *, Bufblock *);
static int assquote(Biobuf *, Bufblock *, int);
static int assdquote(Biobuf *, Bufblock *);
static char *squote(char*);
static long nextrune(Biobuf*, int);
static int expandvar(Biobuf *, Bufblock *);
static Bufblock *varname(Biobuf*);
static int varsub(Biobuf *, Bufblock *);
static int whitespace(Biobuf*);

/*
 *	Assemble a line skipping blank lines, comments, and eliding
 *	escaped newlines
 */
int
assline(Biobuf *bp, Bufblock *buf)
{
	int c;
	int lastc;

	initdone = 0;
	buf->current=buf->start;
	while ((c = nextrune(bp, 1)) >= 0){
		switch(c) {
		case '\n':
			if (buf->current != buf->start) {
				insert(buf, 0);
				return 1;
			}
			break;		/* skip empty lines */
		case '\'':
			rinsert(buf, c);
			if (!assquote(bp, buf, 1))
				Exit();
			break;
		case '`':
			if (!bquote(bp, buf))
				Exit();
			break;
		case '$':
			if (!varsub(bp, buf))
				Exit();
			break;
		case '#':
			lastc = '#';
			while ((c = Bgetc(bp)) != '\n') {
				if (c < 0)
					goto eof;
				lastc = c;
			}
			inline++;
			if (lastc == '\\')
				break;		/* propagate escaped newlines??*/
			if (buf->current != buf->start) {
				insert(buf, 0);
				return 1;
			}
			break;
		default:
			rinsert(buf, c);
			break;
		}
	}
eof:
	insert(buf, 0);
	return *buf->start != 0;
}
/*
 *	Assemble a token surrounded by single quotes
 */
static int
assquote(Biobuf *bp, Bufblock *buf, int preserve)
{
	int c, line;

	line = inline;
	while ((c = nextrune(bp, 0)) >= 0) {
		if (c == '\'') {
			if (preserve)
				rinsert(buf, c);
			c = Bgetrune(bp);
			if (c < 0)
				break;
			if (c != '\'') {
				Bungetrune(bp);
				return 1;
			}
		}
		rinsert(buf, c);
	}
	SYNERR(line); fprint(2, "missing closing '\n");
	return 0;
}
/*
 *	assemble a back-quoted shell command
 */
static int
bquote(Biobuf *bp, Bufblock *buf)
{
	int c, line;
	int start;
	char *end;

	line = inline;
	c = whitespace(bp);
	if(c != '{') {
		SYNERR(line);
		fprint(2, "missing opening { after `\n");
		return 0;
	}
	start = buf->current-buf->start;
	while ((c = nextrune(bp, 0)) > 0) {
		if (c == '\n')
			break;
		if (c == '}') {
			c = Bgetrune(bp);
			if (c != '`')	/* for backward compatibility */
				Bungetrune(bp);
			insert(buf, '\n');
			insert(buf,0);
			end = buf->current-1;
			buf->current = buf->start+start;
			if(initdone == 0){
				execinit();
				initdone = 1;
			}
			rcexec(buf->current, end, buf);
			return 1;
		} else if (c == '\'') {
			insert(buf, c);
			if (!assquote(bp, buf, 1))
				return 0;
			continue;
		}
		rinsert(buf, c);
	}
	SYNERR(line);
	fprint(2, "missing closing } after `{\n");
	return 0;
}

static int
varsub(Biobuf *bp, Bufblock *buf)
{
	int c;
	Bufblock *buf2;

	c = Bgetrune(bp);
	if(c == '{')		/* either ${name} or ${name: A%B==C%D}*/
		return expandvar(bp, buf);
	Bungetrune(bp);
	buf2 = varname(bp);
	if (!buf2)
		return 0;
	varmatch(buf2->start, buf);
	freebuf(buf2);
	return 1;
}

static int
expandvar(Biobuf *bp, Bufblock *buf)
{
	Bufblock *buf2;
	int c;
	int start;
	Symtab *sym;

	buf2 = varname(bp);
	if (!buf2)
		return 0;
	c = Bgetrune(bp);
	if (c == '}') {				/* ${name} variant*/
		varmatch(buf2->start, buf);
		freebuf(buf2);
		return 1;
	}
	if (c != ':') {
		SYNERR(-1);
		fprint(2, "bad variable name <%s>\n", buf2->start);
		freebuf(buf2);
		return 0;
	}
	start = buf2->current-buf2->start;
	while ((c = Bgetrune(bp)) != '}') {	/* ${name:A%B=C%D} variant */
		switch(c)
		{
		case 0:
		case '\n':
			SYNERR(-1);
			fprint(2, "missing '}'\n");
			freebuf(buf2);
			return 0;
		case '\'':
			if (!assquote(bp, buf2, 0)) {
				freebuf(buf2);
				return 0;
			}
			break;
		case '`':
			if (!bquote(bp, buf2)) {
				freebuf(buf2);
				return 0;
			}
			break;
		case '$':
			if (!varsub(bp, buf2)) {
				freebuf(buf2);
				return 0;
			}
			break;
		case ' ':
		case '\t':
			break;
		default:
			rinsert(buf2, c);
			break;
		}
	}
	insert(buf2, 0);
	sym = symlook(buf2->start, S_VAR, 0);
	if (!sym || !sym->value)
		bufcpy(buf, buf2->start, start-2);
	else subsub((Word *) sym->value, buf2->start+start, buf);
	freebuf(buf2);
	return 1;
}
/*
 *	extract a variable name
 */
static Bufblock *
varname(Biobuf *bp)
{
	Bufblock *buf;
	int c;

	buf = newbuf();
	c = Bgetrune(bp);
	while (WORDCHR(c)) {
		rinsert(buf, c);
		c = Bgetrune(bp);
	}
	if (c < 0 || buf->current == buf->start) {
		SYNERR(-1);
		fprint(2, "bad variable name <%s>\n", buf->start);
		freebuf(buf);
		return 0;
	}
	Bungetrune(bp);
	insert(buf, 0);
	return buf;
}
/*
 *	search a string for unescaped characters in a pattern set
 */
char *
charin(char *cp, char *pat)
{
	Rune r;
	int n;

	while (*cp) {
		n = chartorune(&r, cp);
		if (r == '\'') {	/* skip quoted string */
			cp = squote(cp);	/* n must = 1 */
			if (!cp)
				return 0;
		} else
			if (utfrune(pat, r))
				return cp;
		cp += n;
	}
	return 0;
}
/*
 *	skip a token in single quotes.
 */
static char *
squote(char *cp)
{
	Rune r;
	int n;

	cp++;			/* skip opening quote */
	while (*cp) {
		n = chartorune(&r, cp);
		if (r == '\'') {
			n += chartorune(&r, cp+n);
			if (r != '\'')
				return(cp);
		}
		cp += n;
	}
	SYNERR(-1);		/* should never occur */
	fprint(2, "missing closing '\n");
	return 0;
}
/*
 *	get next character stripping escaped newlines
 *	the flag specifies whether escaped newlines are to be elided or
 *	replaced with a blank.
 */
static long
nextrune(Biobuf *bp, int elide)
{
	int c;

	for (;;) {
		c = Bgetrune(bp);
		if (c == '\\') {
			if (Bgetrune(bp) == '\n') {
				inline++;
				if (elide)
					continue;
				return ' ';
			}
			Bungetrune(bp);
		}
		if (c == '\n')
			inline++;
		return c;
	}
	return 0;
}

static int
whitespace(Biobuf *bp)
{
	int c;

	while ((c = Bgetrune(bp)) == ' ' || c == '\t')
			;
	return c;
}
