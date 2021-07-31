#include <u.h>
#include <libc.h>
#include "glob.h"

char *globname;
Glob *globv;
Glob *lastglob;

#define	GLOB	((char)0x01)

/*
 * onebyte(c), twobyte(c), threebyte(c)
 * Is c the first character of a one- two- or three-byte utf sequence?
 */
#define	onebyte(c)	((c&0x80)==0x00)
#define	twobyte(c)	((c&0xe0)==0xc0)
#define	threebyte(c)	((c&0xf0)==0xe0)

static Glob*	newGlob(char*, Glob*);
static int	globsize(char*);
static void	deglob(char*);
static int	globcmp(char**, char**);
static void	globsort(Glob*, Glob*);
static void	globdir(char*, char*);
static int	equtf(char*, char*);
static char*	nextutf(char*);
static int	unicode(char*);
static int	matchfn(char*, char*);
static int	match(char*, char*, int);

/*
 *  return a list of matches
 */
Glob*
glob(char *p)
{
	Glob *svglobv = globv;
	int globlen, isglob;
	char *gp, *tp;

	/* add GLOB's before special characters */
	gp = malloc(2*strlen(p)+1);
	tp = gp;
	isglob = 0;
	globlen = NAMELEN + 1;
	while(*p){
		globlen++;
		switch(*p){
		case '*':
			globlen += NAMELEN;
		case '?':
		case '[':
		case ']':
			isglob = 1;
			*tp++ = GLOB;
			break;
		}
		globlen++;
		*tp++ = *p++;
	}
	*tp = 0;

	if(!isglob){
		globv = newGlob(gp, globv);
		lastglob = globv;
		svglobv = globv;
		globv = 0;
		return svglobv;
	}
	p = gp;
	globname = malloc(globlen);
	globname[0] = '\0';
	globdir(p, globname);
	free(globname);
	if(svglobv == globv){
		deglob(p);
		globv = newGlob(p, globv);
	} else
		globsort(globv, svglobv);
	svglobv = globv;
	for(lastglob = globv; lastglob->next; lastglob = lastglob->next)
		;
	globv = 0;

	return svglobv;
}

/*
 *  free the match list
 */
void
globfree(Glob *g)
{
	Glob *next;

	for(; g; g = next){
		next = g->next;
		free(g->glob);
		free(g);
	}
}

/*
 *  add to the end of the glob list
 */
void
globadd(char *cp)
{
	Glob *p;

	p = newGlob(cp, 0);	
	lastglob->next = p;
	lastglob = p;	
}

static Glob*
newGlob(char *wd, Glob *next)
{
	Glob *p;

	p = malloc(sizeof(Glob));
	p->glob = strdup(wd);
	p->next = next;
	return p;
}

/*
 *  maximum length after globing
 */
/*
 * delete all the GLOB marks from s, in place
 */
static void
deglob(char *s)
{
	char *t = s;

	do{
		if(*t == GLOB)
			t++;
		*s++ = *t;
	}while(*t++);
}
static int
globcmp(char **s, char **t)
{
	return strcmp(*s, *t);
}
static void
globsort(Glob *left, Glob *right)
{
	char **list;
	Glob *a;
	int n = 0;

	for(a = left; a != right; a = a->next)
		n++;
	list = (char **)malloc(n*sizeof(char *));
	for(a = left,n = 0; a != right; a = a->next,n++)
		list[n] = a->glob;
	qsort((char *)list, n, sizeof(char *), globcmp);
	for(a = left,n = 0; a != right; a = a->next,n++)
		a->glob = list[n];
	free((char *)list);
}
/*
 * Push names prefixed by globname and suffixed by a match of p onto the astack.
 * namep points to the end of the prefix in globname.
 */
static void
globdir(char *p, char *namep)
{
	char *t, *newp;
	int f;
	Dir d;

	/* scan the pattern looking for a component with a metacharacter in it */
	if(*p == '\0'){
		globv = newGlob(globname, globv);
		return;
	}
	t = namep;
	newp = p;
	while(*newp){
		if(*newp == GLOB)
			break;
		*t = *newp++;
		if(*t++ == '/'){
			namep = t;
			p = newp;
		}
	}
	/* If we ran out of pattern, append the name if accessible */
	if(*newp == '\0'){
		*t = '\0';
		if(access(globname, 0) == 0)
			globv = newGlob(globname, globv);
		return;
	}
	/* read the directory and recur for any entry that matches */
	*namep = '\0';
	if((f = open(globname[0]?globname:".", OREAD))<0)
		return;
	if(dirfstat(f, &d) < 0 || (d.mode & CHDIR) == 0){
		close(f);
		return;
	}
	while(*newp != '/' && *newp != '\0')
		newp++;
	while(dirread(f, &d, sizeof(d)) > 0){
		strcpy(namep, d.name);
		if(matchfn(namep, p)){
			for(t = namep;*t;t++)
				;
			globdir(newp, t);
		}
	}
	close(f);
}
/*
 * Do p and q point at equal utf codes
 */
static int
equtf(char *p, char *q)
{
	if(*p != *q)
		return 0;
	if(twobyte(*p))
		return p[1] == q[1];
	if(threebyte(*p)){
		if(p[1] != q[1])
			return 0;
		if(p[1] == '\0')
			return 1;	/* broken code at end of string! */
		return p[2] == q[2];
	}
	return 1;
}
/*
 * Return a pointer to the next utf code in the string,
 * not jumping past nuls in broken utf codes!
 */
static char*
nextutf(char *p)
{
	if(twobyte(*p))
		return p[1] == '\0'?p+1:p+2;
	if(threebyte(*p))
		return p[1] == '\0'?p+1:p[2] == '\0'?p+2:p+3;
	return p+1;
}
/*
 * Convert the utf code at *p to a unicode value
 */
static int
unicode(char *p)
{
	int u = *p&0xff;

	if(twobyte(u))
		return ((u&0x1f)<<6)|(p[1]&0x3f);
	if(threebyte(u))
		return (u<<12)|((p[1]&0x3f)<<6)|(p[2]&0x3f);
	return u;
}
/*
 * Does the string s match the pattern p
 * . and .. are only matched by patterns starting with .
 * * matches any sequence of characters
 * ? matches any single character
 * [...] matches the enclosed List of characters
 */
static int
matchfn(char *s, char *p)
{
	if(s[0] == '.' && (s[1] == '\0' || s[1] == '.' && s[2] == '\0') && p[0] != '.')
		return 0;
	return match(s, p, '/');
}
static int
match(char *s, char *p, int stop)
{
	int compl, hit, lo, hi, t, c;

	for(; *p != stop && *p != '\0'; s = nextutf(s),p = nextutf(p)){
		if(*p != GLOB){
			if(!equtf(p, s))
				return 0;
		}
		else switch(*++p){
		case GLOB:
			if(*s != GLOB)
				return 0;
			break;
		case '*':
			for(;;){
				if(match(s, nextutf(p), stop))
					return 1;
				if(!*s)
					break;
				s = nextutf(s);
			}
			return 0;
		case '?':
			if(*s == '\0')
				return 0;
			break;
		case '[':
			if(*s == '\0')
				return 0;
			c = unicode(s);
			p++;
			compl = *p == '~';
			if(compl) p++;
			hit = 0;
			while(*p != ']'){
				if(*p == '\0')
					return 0;		/* syntax error */
				lo = unicode(p);
				p = nextutf(p);
				if(*p != '-')
					hi = lo;
				else{
					p++;
					if(*p == '\0')
						return 0;	/* syntax error */
					hi = unicode(p);
					p = nextutf(p);
					if(hi<lo){
						t = lo;
						lo = hi;
						hi = t;
					}
				}
				if(lo <= c && c <= hi)
					hit = 1;
			}
			if(compl)
				hit = !hit;
			if(!hit)
				return 0;
			break;
		}
	}
	return *s == '\0';
}
