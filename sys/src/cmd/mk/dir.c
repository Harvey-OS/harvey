#include "mk.h"

void
bulkmtime(char *dir)
{
	char buf[4096];
	char *ss;
	char *s;
	Dir db;

	if (dir) {
		s = dir;
		if (strcmp(dir, "/") == 0)
			strcpy(buf, dir);
		else
			sprint(buf, "%s/", dir);
		if((dirstat(dir, &db) >= 0) && ((db.qid.path&CHDIR) == 0)){
			/* bugger off */
			fprint(2, "mk: %s is not a directory path=%x\n", dir, db.qid.path);
			Exit();
		}
	} else {
		s = ".";
		buf[0] = 0;
	}
	if (symlook(s, S_BULKED, 0))
		return;
	ss = strdup(s);
	symlook(ss, S_BULKED, ss);
	dirtime(s, buf);
}

long
mtime(char *name)
{
	Dir sbuf;
	char *s, *ss;
	char carry;

	s = utfrrune(name, '/');
	if (s == name)
		s++;
	if (s) {
		ss = name;
		carry = *s;
		*s = 0;
	} else {
		ss = 0;
		carry = 0;
	}
	bulkmtime(ss);
	if(carry)
		*s = carry;
	if(dirstat(name, &sbuf) < 0)
		return 0;
	return sbuf.mtime;
}

long
filetime(char *name)
{
	Symtab *sym;

	sym = symlook(name, S_TIME, 0);
	if (sym)
		return (long) sym->value;		/* uggh */
	return mtime(name);
}
