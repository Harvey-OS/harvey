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
	if(a->postid)
		free(a->postid);
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
	int i,n;
	Biobuf *b;
	char *p;
	char *field[20];

	memset(a, 0, sizeof(Acctbio));
	b = Bopen(file, OREAD);
	if(b == 0)
		return;
	while(p = Brdline(b, '\n')){
		p[Blinelen(b)-1] = 0;
		n = getfields(p, field, nelem(field), 0, "|");
		if(n < 4)
			continue;
		if(strcmp(field[0], user) != 0)
			continue;

		clrbio(a);

		a->postid = strdup(field[1]);
		a->name = strdup(field[2]);
		a->dept = strdup(field[3]);
		if(n-4 >= Nemail)
			n = Nemail-4;
		for(i = 4; i < n; i++)
			a->email[i-4] = strdup(field[i]);
	}
	a->user = strdup(user);
	Bterm(b);
}
