#include <u.h>
#include <libc.h>
#include <bio.h>
#include <ctype.h>
#include <auth.h>
#include "authsrv.h"

void
clrbio(Acctbio *a)
{
	int i;

	if(a->user)
		free(a->user);
	if(a->name)
		free(a->name);
	if(a->dept)
		free(a->dept);
	for(i = 0; i < Nemail; i++)
		if(a->email[i])
			free(a->email[i]);
	memset(a, 0, sizeof(Acctbio));
}

void
rdbio(char *file, char *user, Acctbio *a)
{
	Biobuf *b;
	char *p, *cp, *next;
	int ne, ulen;

	memset(a, 0, sizeof(Acctbio));
	b = Bopen(file, OREAD);
	if(b == 0)
		return;
	ulen = strlen(user);
	while(p = Brdline(b, '\n')){
		if(strncmp(p, user, ulen) != 0)
			continue;
		if(p[ulen] && p[ulen] != ' ' && p[ulen] != '\t')
			continue;

		p[Blinelen(b)-1] = 0;
		p += ulen;
		clrbio(a);

		/* get name */
		while(*p == ' ' || *p == '\t')
			p++;
		for(cp = p; *cp; cp++){
			if(isdigit(*cp) || *cp == '<'){
				while(cp > p && *(cp-1) != ' ' && *(cp-1) != '\t')
					cp--;
				break;
			}
		}
		next = cp;
		while(cp > p && (*(cp-1) == ' ' || *(cp-1) == '\t'))
			cp--;
		a->name = malloc(cp - p + 1);
		strncpy(a->name, p, cp - p);
		a->name[cp - p] = 0;
		p = next;

		/* get dept */
		for(cp = p; *cp; cp++){
			if(*cp == '<')
				break;
		}
		next = cp;
		while(cp > p && (*(cp-1) == ' ' || *(cp-1) == '\t'))
			cp--;
		a->dept = malloc(cp - p + 1);
		strncpy(a->dept, p, cp - p);
		a->dept[cp - p] = 0;
		p = next;

		/* get emails */
		ne = 0;
		for(cp = p; *cp && ne < Nemail;){	
			if(*cp != '<'){
				cp++;
				continue;
			}
			p = ++cp;
			while(*cp && *cp != '>')
				cp++;
			if(cp == p)
				break;
			a->email[ne] = malloc(cp - p + 1);
			strncpy(a->email[ne], p, cp - p);
			a->email[ne][cp-p] = 0;
			ne++;
		}
	}
	a->user = strdup(user);
	Bterm(b);
}
