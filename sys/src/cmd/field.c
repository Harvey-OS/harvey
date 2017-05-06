#include <u.h>
#include <libc.h>
#include <bio.h>
#include <ctype.h>
#include <regexp.h>

typedef struct Range Range;
typedef struct Slice Slice;
typedef struct Slices Slices;

struct Range {
	int begin;
	int end;
};

struct Slice {
	char *begin;
	char *end;
};

struct Slices {
	uint len;
	uint size;
	Slice *slices;
};

enum {
	NF = 0x7FFFFFFF
};

Biobuf bin;
Biobuf bout;
int zflag;

int guesscollapse(const char *sep);
int Sfmt(Fmt *f);
Slice lex(char **sp);
Slice next(char **sp);
Slice peek(void);
void extend(Slice *slice, char **sp);
int tiseof(Slice *tok);
int tisdelim(Slice *tok);
int tisspace(Slice *tok);
int parseranges(char *src, Range **rv);
Range parserange(char **sp);
int stoi(Slice slice);
int parsenum(char **s);
void process(Biobuf *b, int rc, Range *rv, Reprog *delim, char *sep, int collapse);
void pprefix(char *prefix);
uint split(char *line, Reprog *delim, Slices *ss, int collapse);
void reset(Slices *ss);
void append(Slices *ss, char *begin, char *end);
void usage(void);

void
main(int argc, char *argv[])
{
	Range *rv;
	char *filename, *insep, *outsep;
	Reprog *delim;
	int rc, collapse, eflag, Eflag, oflag;

	insep = "[ \t\v\r]+";
	outsep = " ";
	Binit(&bin, 0, OREAD);
	Binit(&bout, 1, OWRITE);
	fmtinstall('S', Sfmt);

	zflag = 0;
	eflag = 0;
	Eflag = 0;
	oflag = 0;
	ARGBEGIN {
	case '0':
		outsep = "";
		zflag = 1;
		break;
	case 'e':
		eflag = 1;
		break;
	case 'E':
		Eflag = 1;
		break;
	case 'F':
		insep = EARGF(usage());
		break;
	case 'O':
		oflag = 1;
		outsep = EARGF(usage());
		break;
	default:
		usage();
		break;
	} ARGEND;
	if (eflag && Eflag) {
		fprint(2, "flag conflict: -e and -E are mutually exclusive\n");
		usage();
	}
	if (oflag && zflag) {
		fprint(2, "flag conflict: -0 and -O are mutually exclusive\n");
		usage();
	}
	if (argc <= 0)
		usage();
	delim = regcomp(insep);
	if (delim == nil)
		sysfatal("bad input separator regexp '%s': %r", insep);
	rv = nil;
	rc = parseranges(*argv++, &rv);
	if (rc < 0)
		sysfatal("parseranges failed");
	collapse = guesscollapse(insep);
	if (eflag)
		collapse = 0;
	if (Eflag)
		collapse = 1;
	if (*argv == nil) {
		process(&bin, rc, rv, delim, outsep, collapse);
	} else while ((filename = *argv++) != nil) {
		Biobuf *b;
		if (strcmp(filename, "-") == 0) {
			process(&bin, rc, rv, delim, outsep, collapse);
			continue;
		}
		b = Bopen(filename, OREAD);
		if (b == nil)
			sysfatal("failure opening '%s': %r", filename);
		process(b, rc, rv, delim, outsep, collapse);
		Bterm(b);
	}

	exits(0);
}

int
guesscollapse(const char *sep)
{
	int len = utflen(sep);
	return len > 1 && (len != 2 || *sep != '\\');
}

int
Sfmt(Fmt *f)
{
	Slice s = va_arg(f->args, Slice);
	if (s.begin == nil || s.end == nil)
		return 0;
	return fmtprint(f, "%.*s", s.end - s.begin, s.begin);
}

/*
 * The field selection syntax is:
 *
 * fields := range [[delim] fields]
 * range := field | NUM '-' [field]
 * field := NUM | 'NF'
 * delim := ws+ | '|' | ','
 * ws := c such that `isspace(c)` is true.
 */
Slice
lex(char **sp)
{
	char *s;
	Slice slice;

	memset(&slice, 0, sizeof(slice));
	s = *sp;
	slice.begin = s;
	while (isspace(*s))
		s++;
	if (s == *sp) {
		switch (*s) {
		case '\0':
			slice.begin = nil;
			break;
		case '-':
			s++;
			break;
		case 'N':
			if (*++s == 'F')
				s++;
			break;
		case ',':
		case '|':
			s++;
			break;
		default:
			if (!isdigit(*s))
				sysfatal("lexical error, c = %c", *s);
			while (isdigit(*s))
				s++;
			break;
		}
	}
	slice.end = s;
	*sp = s;

	return slice;
}

Slice current;

Slice
peek()
{
	return current;
}

Slice
next(char **sp)
{
	Slice tok = peek();
	current = lex(sp);
	return tok;
}

void
extend(Slice *slice, char **sp)
{
	Slice tok = next(sp);
	slice->end = tok.end;
}

int
stoi(Slice slice)
{
	char *s;
	int n = 0, sign = 1;

	s = slice.begin;
	if (*s == '-') {
		sign = -1;
		s++;
	}
	for (; s != slice.end; s++) {
		if (!isdigit(*s))
			sysfatal("stoi: bad number in '%S', c = %c", slice, *s);
		n = n * 10 + (*s - '0');
	}

	return sign * n;
}

int
tiseof(Slice *tok)
{
	return tok == nil || tok->begin == nil;
}

int
tisdelim(Slice *tok)
{
	return tiseof(tok) || tisspace(tok) || *tok->begin == ',' || *tok->begin == '|';
}

int
tisspace(Slice *tok)
{
	return !tiseof(tok) && isspace(*tok->begin);
}

int
parseranges(char *src, Range **rv)
{
	char *s;
	Range *rs, *t;
	int n, m;
	Slice tok;

	rs = nil;
	m = 0;
	n = 0;
	s = src;
	if (s == nil || *s == '\0')
		return -1;
	next(&s);
	do {
		tok = peek();
		while (tisspace(&tok))
			tok = next(&s);
		Range r = parserange(&s);
		if (n >= m) {
			m = 2*m;
			if (m == 0)
				m = 1;
			t = realloc(rs, sizeof(Range) * m);
			if (t == nil)
				sysfatal("realloc failed parsing ranges");
			rs = t;
		}
		rs[n++] = r;
 		tok = next(&s);
		if (!tisdelim(&tok))
			sysfatal("syntax error in field list");
	} while (!tiseof(&tok));
	*rv = rs;

	return n;
}

int
tokeq(Slice *tok, const char *s)
{
	return !tiseof(tok) && !strncmp(tok->begin, s, tok->end - tok->begin);
}

Range
parserange(char **sp)
{
	Range range;
	Slice tok;

	range.begin = range.end = NF;
	tok = peek();
	if (tokeq(&tok, "NF")) {
		next(sp);
		return range;
	}
	range.begin = range.end = parsenum(sp);
	tok = peek();
	if (tokeq(&tok, "-")) {
		next(sp);
		range.end = NF;
		tok = peek();
		if (tokeq(&tok, "NF")) {
			next(sp);
			return range;
		}
		if (!tiseof(&tok) && !tisdelim(&tok))
			range.end = parsenum(sp);
	}
	return range;
}

int
parsenum(char **sp)
{
	Slice tok;

	tok = next(sp);
	if (tiseof(&tok))
		sysfatal("EOF in number parser");
	if (isdigit(*tok.begin))
		return stoi(tok);
	if (*tok.begin != '-')
		sysfatal("number parse error: unexpected '%S'", tok);
	extend(&tok, sp);
	if (!isdigit(*(tok.begin + 1)))
		sysfatal("negative number parse error: unspected '%S'", tok);
	return stoi(tok);
}

void
process(Biobuf *b, int rc, Range *rv, Reprog *delim, char *outsep, int collapse)
{
	char *line, *prefix;
	const int nulldelim = 1;
	Slice *s;
	Slices ss;

	memset(&ss, 0, sizeof(ss));
	while ((line = Brdstr(b, '\n', nulldelim)) != 0) {
		int printed = 0;
		uint nfields = split(line, delim, &ss, collapse);
		s = ss.slices;
		prefix = nil;
		for (int k = 0; k < rc; k++) {
			int begin = rv[k].begin;
			int end = rv[k].end;
			if (begin == 0) {
				pprefix(prefix);
				prefix = outsep;
				Bprint(&bout, "%s", line);
				printed = 1;
				begin = 1;
			}
			if (begin == NF)
				begin = nfields;
			if (begin < 0)
				begin += nfields + 1;
			begin--;
			if (end < 0)
				end += nfields + 1;
			if (begin < 0 || end < 0 || end < begin || nfields < begin)
				continue;
			for (int f = begin; f < end && f < nfields; f++) {
				pprefix(prefix);
				prefix = outsep;
				Bprint(&bout, "%S", s[f]);
				printed = 1;
			}
		}
		if (rc != 0 && (printed || !collapse))
			Bputc(&bout, '\n');
		free(line);
	}
	free(ss.slices);
}

void
pprefix(char *prefix)
{
	if (prefix == nil)
		return;
	if (zflag)
		Bputc(&bout, '\0');
	else
		Bprint(&bout, "%s", prefix);
}

void
reset(Slices *ss)
{
	ss->len = 0;
}

uint
split(char *line, Reprog *delim, Slices *ss, int collapse)
{
	char *s, *b, *e;
	Resub match[1];

	memset(match, 0, sizeof(match));
	reset(ss);
	b = nil;
	e = nil;
	s = line;
	while (regexec(delim, s, match, nelem(match))) {
		b = s;
		e = match[0].sp;
		s = match[0].ep;
		memset(match, 0, sizeof(match));
		if (collapse && (e == line || b == e))
			continue;
		append(ss, b, e);
	}
	b = s;
	e = b + strlen(s);
	if (!collapse || b != e)
		append(ss, b, e);

	return ss->len;
}

void
append(Slices *ss, char *begin, char *end)
{
	if (ss->len >= ss->size) {
		Slice *s;
		ss->size *= 2;
		if (ss->size == 0)
			ss->size = 1;
		s = realloc(ss->slices, ss->size * sizeof(Slice));
		if (s == nil)
			sysfatal("malloc failed appending slice: %r");
		ss->slices = s;
	}
	ss->slices[ss->len].begin = begin;
	ss->slices[ss->len++].end = end;
}

void
usage()
{
	sysfatal("usage: field [ -E | -e ] [ -F regexp ] [ -0 | -O delimiter ] <field list> [file...]");
}
