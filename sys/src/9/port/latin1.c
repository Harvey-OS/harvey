#include	"u.h"
#include	"../port/lib.h"
/*
 * The code makes two assumptions: strlen(ld) is 1 or 2; latintab[i].ld can be a
 * prefix of latintab[j].ld only when j<i.
 */
struct cvlist
{
	char	*ld;		/* must be seen before using this conversion */
	char	*si;		/* options for last input characters */
	Rune	*so;		/* the corresponding Rune for each si entry */
} latintab[] = {
#include "../port/latin1.h"
	0,	0,		0
};

/*
 * Given n characters k[0]..k[n-1], find the rune or return -1 for failure.
 */
long
unicode(Rune *k, int n)
{
	long c;
	Rune *r;

	c = 0;
	for(r = &k[1]; r<&k[n]; r++){		/* +1 to skip [Xx] */
		c <<= 4;
		if('0'<=*r && *r<='9')
			c += *r-'0';
		else if('a'<=*r && *r<='f')
			c += 10 + *r-'a';
		else if('A'<=*r && *r<='F')
			c += 10 + *r-'A';
		else
			return -1;
	}
	return c;
}

/*
 * Given n characters k[0]..k[n-1], find the corresponding rune or return -1 for
 * failure, or something < -1 if n is too small.  In the latter case, the result
 * is minus the required n.
 */
long
latin1(Rune *k, int n)
{
	struct cvlist *l;
	int c;
	char* p;

	if(k[0] == 'X')
		if(n>=5)
			return unicode(k, 5);
		else
			return -5;
	if(k[0] == 'x')
		if(n>=UTFmax*2+1)
			return unicode(k, UTFmax*2+1);
		else
			return -(UTFmax+1);
	for(l=latintab; l->ld!=0; l++)
		if(k[0] == l->ld[0]){
			if(n == 1)
				return -2;
			if(l->ld[1] == 0)
				c = k[1];
			else if(l->ld[1] != k[1])
				continue;
			else if(n == 2)
				return -3;
			else
				c = k[2];
			for(p=l->si; *p!=0; p++)
				if(*p == c)
					return l->so[p - l->si];
			return -1;
		}
	return -1;
}
