#include "lib.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "sys9.h"
#include "dir.h"

/*
 * Search /adm/users for line with second field ==  *pname (if
 * not NULL), else with first field == *pnum.  Return non-zero
 * if found, and fill in *pnum, *pname, and *plist to fields
 * 1, 2, and 4
 */

static char *admusers = "/adm/users";
static char holdname[30];
static char holdlist[1000];
static char *holdvec[200];

int
_getpw(int *pnum, char **pname, char **plist)
{
	Dir d;
	int f, n, i, matchnum, m, matched;
	char *eline, *f1, *f2, *f3, *f4;
	static char *au = NULL;

	if(!pname)
		return 0;
	if(au == NULL){
		char cd[DIRLEN];
		if(_STAT(admusers, cd) < 0)
			return 0;
		convM2D(cd, &d);
		if((au = (char *)malloc(d.length+2)) == NULL)
			return 0;
		f = open(admusers, O_RDONLY);
		if(f < 0)
			return 0;
		n = read(f, au, d.length);
		close(f);
		if(n < 0)
			return 0;
		au[n] = 0;
	}
	matchnum = (*pname == NULL);
	matched = 0;
	for(f1 = au, eline = au; *eline; f1 = eline+1){
		eline = strchr(f1, '\n');
		if(!eline)
			eline = strchr(f1, 0);
		if(*f1 == '#' || *f1 == '\n')
			continue;
		n = eline-f1;
		f2 = memchr(f1, ':', n);
		if(!f2)
			continue;
		f2++;
		f3 = memchr(f2, ':', n-(f2-f1));
		if(!f3)
			continue;
		f3++;
		f4 = memchr(f3, ':', n-(f3-f1));
		if(!f4)
			continue;
		f4++;
		if(matchnum){
			i = atoi(f1);
			if(i == *pnum){
				m = (f3-f2)-1;
				if(m > sizeof(holdname)-1)
					m = sizeof(holdname)-1;
				memcpy(holdname, f2, m);
				holdname[m] = 0;
				*pname = holdname;
				matched = 1;
			}
		}else{
			if(memcmp(*pname, f2, (f3-f2)-1)==0){
				*pnum = atoi(f1);
				matched = 1;
			}
		}
		if(matched){
			m = n-(f4-f1);
			if(m > sizeof(holdlist)-1)
				m = sizeof(holdlist)-1;
			memcpy(holdlist, f4, m);
			holdlist[m] = 0;
			*plist = holdlist;
			return 1;
		}
	}
	return 0;
}

/* assume list can be overwritten */

char **
_grpmems(char *list)
{
	char **v;
	char *p;

	p = list;
	v = holdvec;
	while(v< &holdvec[sizeof(holdvec)]-1 && *p){
		*v++ = p;
		p = strchr(p, ',');
		if(p){
			p++;
			*p = 0;
		}else
			break;
	}
	*v = 0;
	return holdvec;
}
