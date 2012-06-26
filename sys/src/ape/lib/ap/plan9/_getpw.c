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

enum {NAMEMAX = 20, MEMOMAX = 40 };

static char *admusers = "/adm/users";

/* we hold a fixed-length memo list of past lookups, and use a move-to-front
    strategy to organize the list
*/
typedef struct Memo {
	char		name[NAMEMAX];
	int		num;
	char		*glist;
} Memo;

static Memo *memo[MEMOMAX];
static int nmemo = 0;

int
_getpw(int *pnum, char **pname, char **plist)
{
	Dir *d;
	int f, n, i, j, matchnum, m, matched;
	char *eline, *f1, *f2, *f3, *f4;
	Memo *mem;
	static char *au = NULL;
	vlong length;

	if(!pname)
		return 0;
	if(au == NULL){
		d = _dirstat(admusers);
		if(d == nil)
			return 0;
		length = d->length;
		free(d);
		if((au = (char *)malloc(length+2)) == NULL)
			return 0;
		f = open(admusers, O_RDONLY);
		if(f < 0)
			return 0;
		n = read(f, au, length);
		if(n < 0)
			return 0;
		au[n] = 0;
	}
	matchnum = (*pname == NULL);
	matched = 0;
	/* try using memo */
	for(i = 0; i<nmemo; i++) {
		mem = memo[i];
		if(matchnum)
			matched = (mem->num == *pnum);
		else
			matched = (strcmp(mem->name, *pname) == 0);
		if(matched) {
			break;
		}
	}
	if(!matched)
		for(f1 = au, eline = au; !matched && *eline; f1 = eline+1){
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
			if(matchnum)
				matched = (atoi(f1) == *pnum);
			else{
				int length;

				length = f3-f2-1;
				matched = length==strlen(*pname) && memcmp(*pname, f2, length)==0;
			}
			if(matched){
				/* allocate and fill in a Memo structure */
				mem = (Memo*)malloc(sizeof(struct Memo));
				if(!mem)
					return 0;
				m = (f3-f2)-1;
				if(m > NAMEMAX-1)
					m = NAMEMAX-1;
				memcpy(mem->name, f2, m);
				mem->name[m] = 0;
				mem->num = atoi(f1);
				m = n-(f4-f1);
				if(m > 0){
					mem->glist = (char*)malloc(m+1);
					if(mem->glist) {
						memcpy(mem->glist, f4, m);
						mem->glist[m] = 0;
					}
				} else
					mem->glist = 0;
				/* prepare for following move-to-front */
				if(nmemo == MEMOMAX) {
					free(memo[nmemo-1]);
					i = nmemo-1;
				} else {
					i = nmemo++;
				}
			}
		}
	if(matched) {
		if(matchnum)
			*pname = mem->name;
		else
			*pnum = mem->num;
		if(plist)
			*plist = mem->glist;
		if(i > 0) {
			/* make room at front */
			for(j = i; j > 0; j--)
				memo[j] = memo[j-1];
		}
		memo[0] = mem;
		return 1;
	}
	return 0;
}

char **
_grpmems(char *list)
{
	char **v;
	char *p;
	static char *holdvec[200];
	static char holdlist[1000];

	p = list;
	v = holdvec;
	if(p) {
		strncpy(holdlist, list, sizeof(holdlist));
		while(v< &holdvec[sizeof(holdvec)]-1 && *p){
			*v++ = p;
			p = strchr(p, ',');
			if(p){
				p++;
				*p = 0;
			}else
				break;
		}
	}
	*v = 0;
	return holdvec;
}
