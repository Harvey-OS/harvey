#include "mk.h"

void
bulkmtime(char *dir)
{
	int n, i, fd;
	Dir db[32];
	char buf[4096];
	char *ss;
	char *s, *cp;

	if (dir) {
		s = dir;
		if (strcmp(dir, "/") == 0) {
			buf[0] = '/';
			cp = buf+1;
		} else {
			sprint(buf, "%s/", dir);
			cp = buf+strlen(buf);
		}
		*cp = 0;
		if((dirstat(buf, db) >= 0) && ((db[0].qid.path&CHDIR) == 0)){
			/* bugger off */
			fprint(2, "mk: %s is not a directory path=%x\n", buf, db[0].qid.path);
			Exit();
		}
	} else {
		s = ".";
		cp = buf;
	}
	if (symlook(s, S_BULKED, 0))
		return;
	ss = strdup(s);
	symlook(ss, S_BULKED, ss);
	if((fd = open(s, OREAD)) >= 0){
		while((n = dirread(fd, db, sizeof db)) > 0){
			n /= sizeof(Dir);
			for(i = 0; i < n; i++){
				strcpy(cp, db[i].name);
				symlook(strdup(buf), S_TIME, (char *)db[i].mtime)->value = (char *) db[i].mtime;
			}
		}
		close(fd);
	}
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

getuid(void)
{
	return 0; /* for now */
}

getgid(void)
{
	return 0; /* for now */
}
