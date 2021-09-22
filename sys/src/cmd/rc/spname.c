/*
 * spelling correction for cd
 */
#include "rc.h"
#include "exec.h"
#include "io.h"
#include "fns.h"

#define EQ(a, b) (strcmp(a, b) == 0)

enum {					/* comparison scores; lower is better */
	Identical,
	Interchange2,
	Differby1,
	Unalike,
};

static int
spdist(char *s, char *t)
{
	char *s1, *s2, *t1, *t2;
	Rune sr, tr, s1r, t1r;

	while (*s++ == *t)
		if (*t++ == '\0')
			return Identical;
	tr = sr = 0;
	t1 = t;
	if (*t != '\0')
		t1 = t + chartorune(&tr, t);
	if (*--s != '\0') {
		s1 = s + chartorune(&sr, s);
		if (tr != '\0') {
			s2 = s1 + chartorune(&s1r, s1);
			t2 = t1 + chartorune(&t1r, t1);
			if (s1r && t1r && sr == t1r && tr == s1r && EQ(s2, t2))
				return Interchange2;
			if (EQ(s1, t1))
				return Differby1;
		}
		if (EQ(s1, t))
			return Differby1;
	}
	if (tr != '\0' && EQ(s, t1))
		return Differby1;
	return Unalike;
}

/*
 * returns malloced next component of file name *namep, which is advanced
 * past it.
 */
static char *
nextcomp(char **namep)
{
	char *p, *name, *next;

	name = *namep;
	p = strchr(name, '/');
	if (p) {
		*p = '\0';
		next = strdup(name);
		*p = '/';
		*namep = p;
	} else {				/* last input component */
		next = strdup(name);
		*namep += strlen(name);		/* *namep -> \0 now */
	}
	return next;
}

static char *
correct(char *curdir, char *comp, int *score)
{
	int dirf, d, n, nd;
	char *best;
	Dir *ep, *dp;

	d = Unalike;				/* pessimism */
	*score = d;
	best = nil;
	if ((dirf = open(*curdir? curdir: ".", OREAD)) < 0)
		return nil;
	for (dp = nil; d > Identical && (n = dirread(dirf, &dp)) > 0; free(dp))
		while (--n >= 0) {
			ep = &dp[n];
			nd = spdist(ep->name, comp);
			if (nd < d) {
				free(best);
				best = strdup(ep->name);
				d = nd;
				if (d == Identical)
					break;
			}
		}
	close(dirf);
	*score = d;
	return best;
}

/*
 * returns pointer to correctly spelled name,
 * or 0 if no reasonable name is found;
 * uses a static pointer to the corrected name,
 * so copy it if you want to call the routine again.
 * score records how good the match was; ignore if nil return.
 */
char *
spname(char *name, int *score)
{
	int d;
	char *fixed, *best, *comp, *newname;
	static char *lastnew;

	if (lastnew)
		free(lastnew);
	lastnew = best = nil;
	/*
	 * allocate buffer for corrected path.  allow inserting a char per
	 * component to correct; 2* is a gross overestimate but safe.
	 */
	newname = emalloc(2*strlen(name) + 4);
	/*
	 * advance through name by component, copying best guesses into
	 * fixed (newname) and advancing fixed past each new (corrected)
	 * component.
	 */
	*score = 0;
	for (fixed = newname; ; fixed += strlen(fixed)) {
		/* copy then ignore any run of slashes */
		while (*name == '/')
			*fixed++ = *name++;
		*fixed = '\0';
		if (*name == '\0') {		/* processed all of name? */
			lastnew = newname;	/* return best-guess path */
			break;	
		}

		comp = nextcomp(&name);		/* advances name past comp */
		if (comp == nil)
			break;
		best = correct(newname, comp, &d);
		free(comp);
		if (d == Unalike)
			break;
		*score += d;

		/* append fixed best guess to newname */
		if (best) {
			strcpy(fixed, best);
			free(best);
			best = nil;
		}
	}
	free(best);
	if (lastnew == nil)		/* failed? */
		free(newname);
	else if (lastnew != newname)
		pstr(err, "spname: blown assertion: lastnew != newname\n");
	return lastnew;
}
