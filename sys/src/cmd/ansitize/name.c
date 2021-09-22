#include "a.h"

/*
 *	Custom allocators to avoid malloc overheads on small objects.
 * 	We never free these.  (See below.)
 */
typedef struct Stringtab	Stringtab;
struct Stringtab {
	Stringtab *link;
	char *str;
};
static Stringtab*
taballoc(void)
{
	static Stringtab *t;
	static uint nt;

	if(nt == 0){
		t = malloc(64*sizeof(Stringtab));
		nt = 64;
	}
	nt--;
	return t++;
}

static char*
xstrdup(char *s)
{
	char *r;
	int len;
	static char *t;
	static int nt;

	len = strlen(s)+1;
	if(nt < len){
		t = malloc(len+8192);
		nt = len+8192;
	}
	r = t;
	t += len;
	nt -= len;
	strcpy(r, s);
	return r;
}

/*
 *	Return a uniquely allocated copy of a string.
 *	Don't free these -- they stay in the table for the 
 *	next caller who wants that particular string.
 *	String comparison can be done with pointer comparison 
 *	if you know both strings are atoms.
 */
static Stringtab *stab[1024];

static uint
hash(char *s)
{
	uint h;
	uchar *p;

	h = 0;
	for(p=(uchar*)s; *p; p++)
		h = h*37 + *p;
	return h;
}

Name
nameof(char *str)
{
	uint h;
	Stringtab *tab;
	char *s;
	static char *empty = "", *one[256];
	
	if(str == nil)
		return nil;

	/*
	 * Handle empty and one-char strings quickly.
	 */
	if(str[0] == 0)
		return empty;
	if(str[1] == 0){
		s = one[(uchar)str[0]];
		if(s == nil){
			s = xstrdup(str);
			one[(uchar)str[0]] = s;
		}
		return (Name)s;
	}
	
	h = hash(str) % nelem(stab);
	for(tab=stab[h]; tab; tab=tab->link)
		if(strcmp(str, tab->str) == 0)
			return (Name)tab->str;

	tab = taballoc();
	tab->str = xstrdup(str);
	tab->link = stab[h];
	stab[h] = tab;
	return (Name)tab->str;
}
