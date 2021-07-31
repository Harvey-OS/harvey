/* tm.c: split numerical fields */
# include "t.h"

char	*
maknew(char *str)
{
				/* make two numerical fields */
	int	dpoint, c;
	char	*p, *q, *ba;

	p = str;
	for (ba = 0; c = *str; str++)
		if (c == '\\' && *(str + 1) == '&')
			ba = str;
	str = p;
	if (ba == 0) {
		for (dpoint = 0; *str; str++) {
			if (*str == '.' && !ineqn(str, p) && 
			    (str > p && digit(*(str - 1)) || 
			    digit(*(str + 1))))
				dpoint = (int)str;
		}
		if (dpoint == 0)
			for (; str > p; str--) {
				if (digit( *(str - 1) ) && !ineqn(str, p))
					break;
			}
		if (!dpoint && p == str) /* not numerical, don't split */
			return(0);
		if (dpoint) 
			str = (char *)dpoint;
	} else
		str = ba;
	p = str;
	if (exstore == 0 || exstore > exlim) {
		exstore = exspace = chspace();
		exlim = exstore + MAXCHS;
	}
	q = exstore;
	while (*exstore++ = *str++)
		;
	*p = 0;
	return(q);
}


int
ineqn (char *s, char *p)
{
				/* true if s is in a eqn within p */
	int	ineq = 0, c;

	while (c = *p) {
		if (s == p)
			return(ineq);
		p++;
		if ((ineq == 0) && (c == delim1))
			ineq = 1;
		else if ((ineq == 1) && (c == delim2))
			ineq = 0;
	}
	return(0);
}


