#include <u.h>
#include <libc.h>
#include <libg.h>
#include <ctype.h>
#include <bio.h>
#include "proof.h"

char	fname[NFONT][20];		/* font names */
Font	*fonttab[NFONT][NSIZE];	/* pointers to fonts */
int	fmap[NFONT];		/* what map to use with this font */

static void	bufchar(Point, Subfont *, uchar *);
static void	loadfont(int, int);
static void	fontlookup(int, char *);
static void	buildmap(Biobuf*);
static void	buildtroff(char *);
static void	addmap(int, char *, int);
static uchar	*map(uchar *, int);
static void	scanstr(char *, char *, char **);

int	specfont;	/* somehow, number of special font */

void
dochar(uchar *c)
{
	uchar *s;
	Font *f;
	Point p;
	int fontno;

	fontno = curfont;
	if ((s = map(c, curfont)) == 0) {		/* not on current font */
		if ((s = map(c, specfont)) != 0) {	/* on special font */
			fontno = specfont;
		} else {
			/* no such char; use name itself on defont */
			/* this is NOT a general solution */
			p.x = hpos/DIV + xyoffset.x + offset.x;
			p.y = vpos/DIV + xyoffset.y + offset.y;
			p.y -= font->ascent;
			string(&screen, p, font, (char *) c, S|D);
			return;
		}
	}
	p.x = hpos/DIV + xyoffset.x + offset.x;
	p.y = vpos/DIV + xyoffset.y + offset.y;
	while ((f = fonttab[fontno][cursize]) == 0)
		loadfont(fontno, cursize);
	p.y -= f->ascent;
	dprint(2, "putting %s at %d,%d font %d, size %d\n", c, p.x, p.y, fontno, cursize);

	string(&screen, p, f, (char *) s, S|D);
}


static void
loadfont(int n, int s)
{
	char file[100];
	int i, t, fd, deep;
	static char *try[3] = {"", "times/R.", "pelm/"};
	Subfont *f;
	Font *ff;

	try[0] = fname[n];
	for (t = 0; t < 3; t++) {
		i = s * mag * 2 / 3;	/* 2/3 is a fudge factor, not yet settled */
		if (i < MINSIZE)
			i = MINSIZE;
		dprint(2, "size %d, i %d, mag %g\n", s, i, mag);
		for (; i >= MINSIZE; i--)
		  for (deep = screen.ldepth; deep >= 0; deep--) {
			sprint(file, "%s/%s%d.%d", libfont, try[t], i, 1 /*deep*/);
			dprint(2, "trying %s for %d\n", file, i);
			if ((fd = open(file, 0)) >= 0) {
				f = rdsubfontfile(fd, 0);
				if (f == 0) {
					fprint(2, "can't rdsubfontfile %s: %r\n", file);
					exits("rdsubfont");
				}
				close(fd);
				ff = mkfont(f, 0);
				if(ff == 0){
					fprint(2, "can't mkfont %s: %r\n", file);
					exits("rdsubfont");
				}
				fonttab[n][s] = ff;
				dprint(2, "using %s for font %d %d\n", file, n, s);
				return;
			}
		}
	}
	fprint(2, "can't find font %s.%d or substitute, quitting\n", fname[n], s);
	exits("no font");
}

void
loadfontname(int n, char *s)
{
	int i;
	Font *f, *g = 0;

	if (strcmp(s, fname[n]) == 0)
		return;
	fontlookup(n, s);
	for (i = 0; i < NSIZE; i++)
		if (f = fonttab[n][i]) {
			if (f != g) {
				ffree(f);
				g = f;
			}
			fonttab[n][i] = 0;
		}
}

void
allfree(void)
{
	int i;

	for (i = 0; i < NFONT; i++)
		loadfontname(i, "??");
}


/* read a map file and create mapping info */

#define	eq(s,t)	strcmp((char *) s, (char *) t) == 0

int	curmap	= -1;	/* what map are we working on */

typedef struct link {	/* temporarily, link names together */
	uchar		*name;
	int		val;
	struct link	*next;
} Link;

typedef struct map {	/* holds a mapping from uchar name to index */
	uchar	ascii[256];	/* ascii values get special treatment */
	Link	*nonascii;	/* other stuff goes into a link list */
} Map;

Map	charmap[50];	/* enough maps for now? */

typedef struct fontmap {	/* holds mapping from troff name to filename */
	char	*troffname;
	char	*prefix;
	int	map;		/* which charmap to use for this font */
} Fontmap;

Fontmap	fontmap[100];
int	nfontmap	= 0;	/* how many are there */


void
readmapfile(char *file)
{
	Biobuf *fp;
	char *p, cmd[100];

	if ((fp = Bopen(file, OREAD)) == 0) {
		fprint(2, "proof: can't open map file %s\n", file);
		exits("urk");
	}
	while ((p = Brdline(fp, '\n')) != 0) {
		p[Blinelen(fp)-1] = 0;
		scanstr(p, cmd, 0);
		if (p[0] == '\0' || eq(cmd, "#"))	/* skip comments, empty */
			continue;
		else if (eq(cmd, "map"))
			buildmap(fp);
		else if (eq(cmd, "special"))
			buildtroff(p);
		else if (eq(cmd, "troff"))
			buildtroff(p);
		else
			fprint(2, "weird map line %s\n", p);
	}
	Bclose(fp);
}

static void
buildmap(Biobuf *fp)	/* map goes from char name to value to print via *string() */
{
	uchar *p, *line, ch[100];
	int val;

	curmap++;
	while ((line = Brdline(fp, '\n'))!= 0) {
		if (line[0] == '\n')
			return;
		line[Blinelen(fp)-1] = 0;
		scanstr((char *) line, (char *) ch, (char **) &p);
		if (ch[0] == '\0') {
			fprint(2, "bad map file line '%s'\n", line);
			continue;
		}
		val = strtol((char *) p, 0, 10);
dprint(2, "buildmap %s (%x %x) %s %d\n", ch, ch[0], ch[1], p, val);
		if (ch[1] == 0) {
			charmap[curmap].ascii[ch[0]] = val;
		} else
			addmap(curmap, strdup((char *) ch), val);	/* put somewhere else */
	}
}

static void
addmap(int n, char *s, int val)	/* stick a new link on */
{
	Link *p = (Link *) malloc(sizeof(Link));
	Link *prev = charmap[n].nonascii;

	if (p == 0)
		exits("out of memory in addmap");
	p->name = (uchar *) s;
	p->val = val;
	p->next = prev;;
	charmap[n].nonascii = p;
}

static void
buildtroff(char *buf)	/* map troff names into bitmap filenames */
{				/* e.g., R -> times/R., I -> times/I., etc. */
	char *p, cmd[100], name[200], prefix[400];

	scanstr(buf, cmd, &p);
	scanstr(p, name, &p);
	scanstr(p, prefix, &p);
	fontmap[nfontmap].troffname = strdup(name);
	fontmap[nfontmap].prefix = strdup(prefix);
	fontmap[nfontmap].map = curmap;
	dprint(2, "troff name %s is bitmap %s map %d in slot %d\n", name, prefix, curmap, nfontmap);
	nfontmap++;
}

static void
fontlookup(int n, char *s)	/* map troff name of s into position n */
{
	int i;

	for (i = 0; i < nfontmap; i++)
		if (eq(s, fontmap[i].troffname)) {
			strcpy(fname[n], fontmap[i].prefix);
			fmap[n] = fontmap[i].map;
			if (eq(s, "S"))
				specfont = n;
			dprint(2, "font %d %s is %s\n", n, s, fname[n]);
			return;
		}
	/* god help us if this font isn't there */
}


static uchar *
map(uchar *c, int font)	/* figure out mapping for c in this font */
{
	static uchar s[100];
	Link *p;

dprint(2, "into map %s [%x %x]\n", c, c[0], c[1]);
	s[0] = c[0];
	s[1] = 0;
	if (c[1] == 0) {	/* ascii */
		s[0] = charmap[fmap[font]].ascii[c[0]];
		if (s[0] == 0) {	/* not there */
			dprint(2, "didn't find %c%c font# %d\n", c[0], c[1], font);
			return 0;
		}
	} else {		/* non-ascii to look up... */
		for (p = charmap[fmap[font]].nonascii; p; p = p->next) {
			if (eq(c, p->name)) {
				s[0] = p->val;
				dprint(2, "map %s to pos %d font# %d\n", c, s[0], font);
				return s;
			}
		}
		dprint(2, "didn't find %s font# %d\n", c, font);
		return 0;	/* if we didn't find it */
	}
	dprint(2, "map %s to %d font# %d\n", c, s[0], font);
	return s;
}

static void
scanstr(char *s, char *ans, char **ep)
{
	char *oans;

	/* dprint(2, "scanning [%s]\n", s); */
	for (; isspace((uchar) *s); s++)
		;
	oans = s;
	s = oans;			/* to shut compiler up */
	for (; !isspace((uchar) *s); )
		*ans++ = *s++;
	*ans = 0;
	if (ep)
		*ep = s;
	/* dprint(2, "returning [%s]\n", oans); */
}

