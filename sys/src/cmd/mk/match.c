#include	"mk.h"

mninlist(char *name, Word *list, char *stem)
{
	for(; list; list = list->next){
		if(match(name, list->s, stem))
			return(1);
	}
	return(0);
}
/*
 *	true when arg contains no '.' and no '/' 
 */
static int
isatomic(char *cp)
{
	Rune r;

	while(*cp) {
		cp += chartorune(&r, cp);
		if (r == '.' || r == '/')
			return 0;
	}
	return 1;
}

match(char *name, char *template, char *stem)
{
	register char *p;
	Rune r;
	int n;

	while (*name && *template) {
		n = chartorune(&r, template);
		if (PERCENT(r))
			break;
		while (n--)
			if(*name++ != *template++)
				return 0;
	}
	if(!PERCENT(*template))
		return 0;
	n = strlen(name)-strlen(template+1);
	p = name+n;
	if (strcmp(template+1, p))
			return 0;
	strncpy(stem, name, n);
	stem[n] = 0;
	if(*template == '&')
		return isatomic(stem);
	return(1);
}

void
subst(char *stem, char *template, char *dest)
{
	Rune r;
	char *s;
	int n;

	while(*template){
		n = chartorune(&r, template);
		if (PERCENT(r)) {
			template += n;
			for (s = stem; *s; s++)
				*dest++ = *s;
		} else
			while (n--)
				*dest++ = *template++;
	}
	*dest = 0;
}
