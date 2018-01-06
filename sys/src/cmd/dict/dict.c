/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#include <u.h>
#include <libc.h>
#include <bio.h>
#include <regexp.h>
#include <ctype.h>
#include "dict.h"

/*
 * Assumed index file structure: lines of form
 * 	[^\t]+\t[0-9]+
 * First field is key, second is byte offset into dictionary.
 * Should be sorted with args -u -t'	' +0f -1 +0 -1 +1n -2
 */
typedef struct Addr Addr;

struct Addr {
	int	n;		/* number of offsets */
	int	cur;		/* current position within doff array */
	int	maxn;		/* actual current size of doff array */
	uint32_t	doff[1];	/* doff[maxn], with 0..n-1 significant */
};

Biobuf	binbuf;
Biobuf	boutbuf;
Biobuf	*bin = &binbuf;		/* user cmd input */
Biobuf	*bout = &boutbuf;	/* output */
Biobuf	*bdict;			/* dictionary */
Biobuf	*bindex;		/* index file */
int32_t	indextop;		/* index offset at end of file */
int	lastcmd;		/* last executed command */
Addr	*dot;			/* "current" address */
Dict	*dict;			/* current dictionary */
int	linelen;
int	breaklen = 60;
int	outinhibit;
int	debug;

void	execcmd(int);
int	getpref(char*, Rune*);
Entry	getentry(int);
int	getfield(Rune*);
int32_t	locate(Rune*);
int	parseaddr(char*, char**);
int	parsecmd(char*);
int	search(char*, int);
int32_t	seeknextline(Biobuf*, int32_t);
void	setdotnext(void);
void	setdotprev(void);
void	sortaddr(Addr*);
void	usage(void);

enum {
	Plen=300,	/* max length of a search pattern */
	Fieldlen=200,	/* max length of an index field */
	Aslots=10,	/* initial number of slots in an address */
};

void
main(int argc, char **argv)
{
	int i, cmd, kflag;
	char *line, *p;

	Binit(&binbuf, 0, OREAD);
	Binit(&boutbuf, 1, OWRITE);
	kflag = 0;
	line = 0;
	dict = 0;
	for(i=0; dicts[i].name; i++){
		if(access(dicts[i].path, 0)>=0 && access(dicts[i].indexpath, 0)>=0){
			dict = &dicts[i];
			break;
		}
	}
	ARGBEGIN {
		case 'd':
			p = ARGF();
			dict = 0;
			if(p) {
				for(i=0; dicts[i].name; i++)
					if(strcmp(p, dicts[i].name)==0) {
						dict = &dicts[i];
						break;
					}
			}
			if(!dict)
				usage();
			break;
		case 'c':
			line = ARGF();
			if(!line)
				usage();
			break;
		case 'k':
			kflag++;
			break;
		case 'D':
			debug++;
			break;
		default:
			usage();
	ARGEND }
	if(dict == 0){
		err("no dictionaries present on this system");
		exits("nodict");
	}

	if(kflag) {
		(*dict->printkey)();
		exits(0);
	}
	if(argc > 1)
		usage();
	else if(argc == 1) {
		if(line)
			usage();
		p = argv[0];
		line = malloc(strlen(p)+5);
		sprint(line, "/%s/P\n", p);
	}
	bdict = Bopen(dict->path, OREAD);
	if(!bdict) {
		err("can't open dictionary %s", dict->path);
		exits("nodict");
	}
	bindex = Bopen(dict->indexpath, OREAD);
	if(!bindex) {
		err("can't open index %s", dict->indexpath);
		exits("noindex");
	}
	indextop = Bseek(bindex, 0L, 2);

	dot = malloc(sizeof(Addr)+(Aslots-1)*sizeof(ulong));
	dot->n = 0;
	dot->cur = 0;
	dot->maxn = Aslots;
	lastcmd = 0;

	if(line) {
		cmd = parsecmd(line);
		if(cmd)
			execcmd(cmd);
	} else {
		for(;;) {
			Bprint(bout, "*");
			Bflush(bout);
			line = Brdline(bin, '\n');
			linelen = 0;
			if(!line)
				break;
			cmd = parsecmd(line);
			if(cmd) {
				execcmd(cmd);
				lastcmd = cmd;
			}
		}
	}
	exits(0);
}

void
usage(void)
{
	int i;
	char *a, *b;

	Bprint(bout, "Usage: %s [-d dict] [-k] [-c cmd] [word]\n", argv0);
	Bprint(bout, "dictionaries (brackets mark dictionaries not present on this system):\n");
	for(i = 0; dicts[i].name; i++){
		a = b = "";
		if(access(dicts[i].path, 0)<0 || access(dicts[i].indexpath, 0)<0){
			a = "[";
			b = "]";
		}
		Bprint(bout, "   %s%s\t%s%s\n", a, dicts[i].name, dicts[i].desc, b);
	}
	exits("usage");
}

int
parsecmd(char *line)
{
	char *e;
	int cmd, ans;

	if(parseaddr(line, &e) >= 0)
		line = e;
	else
		return 0;
	cmd = *line;
	ans = cmd;
	if(isupper(cmd))
		cmd = tolower(cmd);
	if(!(cmd == 'a' || cmd == 'h' || cmd == 'p' || cmd == 'r' ||
	     cmd == '\n')) {
		err("unknown command %c", cmd);
		return 0;
	}
	if(cmd == '\n')
		switch(lastcmd) {
			case 0:	ans = 'H'; break;
			case 'H':	ans = 'p'; break;
			default :	ans = lastcmd; break;
		}
	else if(line[1] != '\n' && line[1] != 0)
		err("extra stuff after command %c ignored", cmd);
	return ans;
}

void
execcmd(int cmd)
{
	Entry e;
	int cur, doall;

	if(isupper(cmd)) {
		doall = 1;
		cmd = tolower(cmd);
		cur = 0;
	} else {
		doall = 0;
		cur = dot->cur;
	}

	if(debug && doall && cmd == 'a')
		Bprint(bout, "%d entries, cur=%d\n", dot->n, cur+1);
	for(;;){
		if(cur >= dot->n)
			break;
		if(doall) {
			Bprint(bout, "%d\t", cur+1);
			linelen += 4 + (cur >= 10);
		}
		switch(cmd) {
		case 'a':
			Bprint(bout, "#%lud\n", dot->doff[cur]);
			break;
		case 'h':
		case 'p':
		case 'r':
			e = getentry(cur);
			(*dict->printentry)(e, cmd);
			break;
		}
		cur++;
		if(doall) {
			if(cmd == 'p' || cmd == 'r') {
				Bputc(bout, '\n');
				linelen = 0;
			}
		} else
			break;
	}
	if(cur >= dot->n)
		cur = 0;
	dot->cur = cur;
}

/*
 * Address syntax: ('.' | '/' re '/' | '!' re '!' | number | '#' number) ('+' | '-')*
 * Answer goes in dot.
 * Return -1 if address starts, but get error.
 * Return 0 if no address.
 */
int
parseaddr(char *line, char **eptr)
{
	int delim, plen;
	uint32_t v;
	char *e;
	char pat[Plen];

	if(*line == '/' || *line == '!') {
		/* anchored regular expression match; '!' means no folding */
		if(*line == '/') {
			delim = '/';
			e = strpbrk(line+1, "/\n");
		} else {
			delim = '!';
			e = strpbrk(line+1, "!\n");
		}
		plen = e-line-1;
		if(plen >= Plen-3) {
			err("pattern too big");
			return -1;
		}
		pat[0] = '^';
		memcpy(pat+1, line+1, plen);
		pat[plen+1] = '$';
		pat[plen+2] = 0;
		if(*e == '\n')
			line = e;
		else
			line = e+1;
		if(!search(pat, delim == '/')) {
			err("pattern not found");
			return -1;
		}
	} else if(*line == '#') {
		/* absolute byte offset into dictionary */
		line++;
		if(!isdigit(*line))
			return -1;
		v = strtoul(line, &e, 10);
		line = e;
		dot->doff[0] = v;
		dot->n = 1;
		dot->cur = 0;
	} else if(isdigit(*line)) {
		v = strtoul(line, &e, 10);
		line = e;
		if(v < 1 || v > dot->n)
			err(".%d not in range [1,%d], ignored",
				v, dot->n);
		else
			dot->cur = v-1;
	} else if(*line == '.') {
		line++;
	} else {
		*eptr = line;
		return 0;
	}
	while(*line == '+' || *line == '-') {
		if(*line == '+')
			setdotnext();
		else
			setdotprev();
		line++;
	}
	*eptr = line;
	return 1;
}

/*
 * Index file is sorted by folded field1.
 * Method: find pre, a folded prefix of r.e. pat,
 * and then low = offset to beginning of
 * line in index file where first match of prefix occurs.
 * Then go through index until prefix no longer matches,
 * adding each line that matches real pattern to dot.
 * Finally, sort dot offsets (uniquing).
 * We know pat len < Plen, and that it is surrounded by ^..$
 */
int
search(char *pat, int dofold)
{
	int needre, prelen, match, n;
	Reprog *re;
	int32_t ioff, v;
	Rune pre[Plen];
	Rune lit[Plen];
	Rune entry[Fieldlen];
	char fpat[Plen];

	prelen = getpref(pat+1, pre);
	if(pat[prelen+1] == 0 || pat[prelen+1] == '$') {
		runescpy(lit, pre);
		if(dofold)
			fold(lit);
		needre = 0;
		SET(re);
	} else {
		needre = 1;
		if(dofold) {
			foldre(fpat, pat);
			re = regcomp(fpat);
		} else
			re = regcomp(pat);
	}
	fold(pre);
	ioff = locate(pre);
	if(ioff < 0)
		return 0;
	dot->n = 0;
	Bseek(bindex, ioff, 0);
	for(;;) {
		if(!getfield(entry))
			break;
		if(dofold)
			fold(entry);
		if(needre)
			match = rregexec(re, entry, 0, 0);
		else
			match = (acomp(lit, entry) == 0);
		if(match) {
			if(!getfield(entry))
				break;
			v = runetol(entry);
			if(dot->n >= dot->maxn) {
				n = 2*dot->maxn;
				dot = realloc(dot,
					sizeof(Addr)+(n-1)*sizeof(int32_t));
				if(!dot) {
					err("out of memory");
					exits("nomem");
				}
				dot->maxn = n;
			}
			dot->doff[dot->n++] = v;
		} else {
			if(!dofold)
				fold(entry);
			if(*pre) {
				n = acomp(pre, entry);
				if(n < -1 || (!needre && n < 0))
					break;
			}
			/* get to next index entry */
			if(!getfield(entry))
				break;
		}
	}
	sortaddr(dot);
	dot->cur = 0;
	return dot->n;
}

/*
 * Return offset in index file of first line whose folded
 * first field has pre as a prefix.  -1 if none found.
 */
int32_t
locate(Rune *pre)
{
	int32_t top, bot, mid;
	Rune entry[Fieldlen];

	if(*pre == 0)
		return 0;
	bot = 0;
	top = indextop;
	if(debug>1)
		fprint(2, "locate looking for prefix %S\n", pre);
	for(;;) {
		/*
		 * Loop invariant: foldkey(bot) < pre <= foldkey(top)
		 * and bot < top, and bot,top point at beginning of lines
		 */
		mid = (top+bot) / 2;
		mid = seeknextline(bindex, mid);
		if(debug > 1)
			fprint(2, "bot=%ld, mid=%ld->%ld, top=%ld\n",
				bot, (top+bot) / 2, mid, top);
		if(mid == top || !getfield(entry))
			break;
		if(debug > 1)
			fprint(2, "key=%S\n", entry);
		/*
		 * here mid is strictly between bot and top
		 */
		fold(entry);
		if(acomp(pre, entry) <= 0)
			top = mid;
		else
			bot = mid;
	}
	/*
	 * bot < top, but they don't necessarily point at successive lines
	 * Use linear search from bot to find first line that pre is a
	 * prefix of
	 */
	while((bot = seeknextline(bindex, bot)) <= top) {
		if(!getfield(entry))
			return -1;
		if(debug > 1)
			fprint(2, "key=%S\n", entry);
		fold(entry);
		switch(acomp(pre, entry)) {
		case -2:
			return -1;
		case -1:
		case 0:
			return bot;
		case 1:
		case 2:
			continue;
		}
	}
	return -1;

}

/*
 * Get prefix of non re-metacharacters, runified, into pre,
 * and return length
 */
int
getpref(char *pat, Rune *pre)
{
	int n, r;
	char *p;

	p = pat;
	while(*p) {
		n = chartorune(pre, p);
		r = *pre;
		switch(r) {
		case L'.': case L'*': case L'+': case L'?':
		case L'[': case L']': case L'(': case ')':
		case L'|': case L'^': case L'$':
			*pre = 0;
			return p-pat;
		case L'\\':
			p += n;
			p += chartorune(++pre, p);
			pre++;
			break;
		default:
			p += n;
			pre++;
		}
	}
	return p-pat;
}

int32_t
seeknextline(Biobuf *b, int32_t off)
{
	int32_t c;

	Bseek(b, off, 0);
	do {
		c = Bgetrune(b);
	} while(c>=0 && c!='\n');
	return Boffset(b);
}

/*
 * Get next field out of index file (either tab- or nl- terminated)
 * Answer in *rp, assumed to be Fieldlen long.
 * Return 0 if read error first.
 */
int
getfield(Rune *rp)
{
	int32_t c;
	int n;

	for(n=Fieldlen; n-- > 0; ) {
		if ((c = Bgetrune(bindex)) < 0)
			return 0;
		if(c == '\t' || c == '\n') {
			*rp = L'\0';
			return 1;
		}
		*rp++ = c;
	}
	err("word too long");
	return 0;
}

/*
 * A compare longs function suitable for qsort
 */
static int
longcmp(const void *av, const void *bv)
{
	int32_t v;
	int32_t *a, *b;

	a = av;
	b = bv;

	v = *a - *b;
	if(v < 0)
		return -1;
	else if(v == 0)
		return 0;
	else
		return 1;
}

void
sortaddr(Addr *a)
{
	int i, j;
	int32_t v;

	if(a->n <= 1)
		return;

	qsort(a->doff, a->n, sizeof(int32_t), longcmp);

	/* remove duplicates */
	for(i=0, j=0; j < a->n; j++) {
		v = a->doff[j];
		if(i > 0 && v == a->doff[i-1])
			continue;
		a->doff[i++] = v;
	}
	a->n = i;
}

Entry
getentry(int i)
{
	int32_t b, e, n;
	static Entry ans;
	static int anslen = 0;

	b = dot->doff[i];
	e = (*dict->nextoff)(b+1);
	ans.doff = b;
	if(e < 0) {
		err("couldn't seek to entry");
		ans.start = 0;
		ans.end = 0;
	} else {
		n = e-b;
		if(n+1 > anslen) {
			ans.start = realloc(ans.start, n+1);
			if(!ans.start) {
				err("out of memory");
				exits("nomem");
			}
			anslen = n+1;
		}
		Bseek(bdict, b, 0);
		n = Bread(bdict, ans.start, n);
		ans.end = ans.start + n;
		*ans.end = 0;
	}
	return ans;
}

void
setdotnext(void)
{
	int32_t b;

	b = (*dict->nextoff)(dot->doff[dot->cur]+1);
	if(b < 0) {
		err("couldn't find a next entry");
		return;
	}
	dot->doff[0] = b;
	dot->n = 1;
	dot->cur = 0;
}

void
setdotprev(void)
{
	int tryback;
	int32_t here, last, p;

	if(dot->cur < 0 || dot->cur >= dot->n)
		return;
	tryback = 2000;
	here = dot->doff[dot->cur];
	last = 0;
	while(last == 0) {
		p = here - tryback;
		if(p < 0)
			p = 0;
		for(;;) {
			p = (*dict->nextoff)(p+1);
			if(p < 0)
				return; /* shouldn't happen */
			if(p >= here)
				break;
			last = p;
		}
		if(!last) {
			if(here - tryback < 0) {
				err("can't find a previous entry");
				return;
			}
			tryback = 2*tryback;
		}
	}
	dot->doff[0] = last;
	dot->n = 1;
	dot->cur = 0;
}
