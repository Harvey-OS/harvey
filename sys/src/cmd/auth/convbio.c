#include <u.h>
#include <libc.h>
#include <bio.h>
#include <ctype.h>
#include "authcmdlib.h"

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

int
ordbio(Biobuf *b, Acctbio *a)
{
	char *p, *cp, *next;
	int ne;

	clrbio(a);
	while(p = Brdline(b, '\n')){
		if(*p == '\n')
			continue;

		p[Blinelen(b)-1] = 0;

		/* get user */
		for(cp = p; *cp && *cp != ' ' && *cp != '\t'; cp++)
			;
		a->user = malloc(cp - p + 1);
		strncpy(a->user, p, cp - p);
		a->user[cp - p] = 0;
		p = cp;

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
		return 0;
	}
	return -1;
}

void
nwrbio(Biobuf *b, Acctbio *a)
{
	int i;

	if(a->postid == 0)
		a->postid = "";
	if(a->name == 0)
		a->name = "";
	if(a->dept == 0)
		a->dept = "";
	if(a->email[0] == 0)
		a->email[0] = strdup(a->user);

	Bprint(b, "%s|%s|%s|%s|%s", a->user, a->user, a->name, a->dept, a->email[0]);
	for(i = 1; i < Nemail; i++){
		if(a->email[i] == 0)
			break;
		Bprint(b, "|%s", a->email[i]);
	}
	Bprint(b, "\n");
}

void
main(void)
{
	Biobuf in, out;
	Acctbio a;

	Binit(&in, 0, OREAD);
	Binit(&out, 1, OWRITE);
	while(ordbio(&in, &a) == 0)
		nwrbio(&out, &a);
	Bterm(&in);
	Bterm(&out);
}
