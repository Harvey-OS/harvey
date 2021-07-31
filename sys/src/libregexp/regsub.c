#include <u.h>
#include <libc.h>
#include "regexp.h"

/* substitute into one string using the matches from the last regexec() */
extern	void
regsub(char *sp,	/* source string */
	char *dp,	/* destination string */
	Resub *mp,	/* subexpression elements */
	int ms)		/* number of elements pointed to by mp */
{
	char *ssp;
	int i;

	while(*sp != '\0'){
		if(*sp == '\\'){
			switch(*++sp){
			case '0':
			case '1':
			case '2':
			case '3':
			case '4':
			case '5':
			case '6':
			case '7':
			case '8':
			case '9':
				i = *sp-'0';
				if(mp[i].sp != 0 && mp!=0 && ms>i)
					for(ssp = mp[i].sp;
					     ssp < mp[i].ep;
					     ssp++)
						*dp++ = *ssp;
				break;
			case '\\':
				*dp++ = '\\';
				break;
			case '\0':
				sp--;
				break;
			default:
				*dp++ = *sp;
				break;
			}
		}else if(*sp == '&'){				
			if(mp[0].sp != 0 && mp!=0 && ms>0)
			if(mp[0].sp != 0)
				for(ssp = mp[0].sp;
				     ssp < mp[0].ep; ssp++)
					*dp++ = *ssp;
		}else
			*dp++ = *sp;
		sp++;
	}
	*dp = '\0';
}
