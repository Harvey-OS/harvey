#include "common.h"

/*
 *  expand a path relative to the user's mailbox directory
 *
 *  if the path starts with / or ./, don't change it
 *
 */
extern String *
mboxpath(char *path, char *user, String *to, int dot)
{
	if (*path=='/' || dot) {
		to = s_append(to, path);
	} else {
		to = s_append(to, MAILROOT);
		to = s_append(to, "/box/");
		to = s_append(to, user);
		to = s_append(to, "/");
		to = s_append(to, path);
	}
	return to;
}

/* expand a path relative to some `.' */
extern String *
abspath(char *path, char *dot, String *to)
{
	if (*path == '/') {
		to = s_append(to, path);
	} else {
		to = s_append(to, dot);
		to = s_append(to, "/");
		to = s_append(to, path);
	}
	return to;
}

/* return a pointer to the base component of a pathname */
extern char *
basename(char *path)
{
	char *cp;

	cp = strrchr(path, '/');
	return cp==0 ? path : cp+1;
}

/* append a sub-expression match onto a String */
extern void
append_match(Resub *subexp, String *sp, int se)
{
	register char *cp = subexp[se].sp;
	register char *ep = subexp[se].ep;

	for (; cp < ep; cp++)
		s_putc(sp, *cp);
	s_terminate(sp);
}

/*
 *  check for shell characters in a String
 */
#define CHARS "()<>{};\\'\`^&|\r\n"
extern int
shellchars(char *cp)
{
	char *sp;

	for(sp=CHARS; *sp; sp++)
		if(strchr(cp, *sp))
			return 1;
	return 0;
}
