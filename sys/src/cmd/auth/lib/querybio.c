#include <u.h>
#include <libc.h>
#include <bio.h>
#include <ctype.h>
#include "authcmdlib.h"


#define TABLEN 8

static char*
defreadln(char *prompt, char *def, int must, int *changed)
{
	char pr[512];
	char reply[256];

	do {
		if(def && *def){
			if(must)
				snprint(pr, sizeof pr, "%s[return = %s]: ", prompt, def);
			else
				snprint(pr, sizeof pr, "%s[return = %s, space = none]: ", prompt, def);
		} else
			snprint(pr, sizeof pr, "%s: ", prompt);
		readln(pr, reply, sizeof(reply), 0);
		switch(*reply){
		case ' ':
			break;
		case 0:
			return def;
		default:
			*changed = 1;
			if(def)
				free(def);
			return strdup(reply);
		}
	} while(must);

	if(def){
		*changed = 1;
		free(def);
	}
	return 0;
}

/*
 *  get bio from stdin
 */
int
querybio(char *file, char *user, Acctbio *a)
{
	int i;
	int changed;

	rdbio(file, user, a);
	a->postid = defreadln("Post id", a->postid, 0, &changed);
	a->name = defreadln("User's full name", a->name, 1, &changed);
	a->dept = defreadln("Department #", a->dept, 1, &changed);
	a->email[0] = defreadln("User's email address", a->email[0], 1, &changed);
	a->email[1] = defreadln("Sponsor's email address", a->email[1], 0, &changed);
	for(i = 2; i < Nemail; i++){
		if(a->email[i-1] == 0)
			break;
		a->email[i] = defreadln("other email address", a->email[i], 0, &changed);
	}
	return changed;
}
