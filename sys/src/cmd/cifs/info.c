#include <u.h>
#include <libc.h>
#include <auth.h>
#include <fcall.h>
#include <thread.h>
#include <9p.h>
#include "cifs.h"


struct {
	char	*name;
	int	(*func)(Fmt *f);
	char	*buf;
	int	len;
} Infdir[] = {
	{ "Users",	userinfo },	
	{ "Groups",	groupinfo },	
	{ "Shares",	shareinfo },	
	{ "Connection",	conninfo },	
	{ "Sessions",	sessioninfo },	
	{ "Dfsroot",	dfsrootinfo },	
	{ "Dfscache",	dfscacheinfo },	
	{ "Domains",	domaininfo },	
	{ "Openfiles",	openfileinfo },	
	{ "Workstations", workstationinfo },	
	{ "Filetable",	filetableinfo },	
};

int
walkinfo(char *name)
{
	int i;

	for(i = 0; i < nelem(Infdir); i++)
		if(strcmp(Infdir[i].name, name) == 0)
			return(i);
	return -1;
}

int
numinfo(void)
{
	return nelem(Infdir);
}

int
dirgeninfo(int slot, Dir *d)
{
	if(slot < 0 || slot > nelem(Infdir))
		return -1;

	memset(d, 0, sizeof(Dir));
	d->type = 'N';
	d->dev = 99;
	d->name = estrdup9p(Infdir[slot].name);
	d->uid = estrdup9p("other");
	d->muid = estrdup9p("other");
	d->gid = estrdup9p("other");
	d->mode = 0666;
	d->atime = time(0);
	d->mtime = d->atime;
	d->qid = mkqid(Infdir[slot].name, 0, 1, Pinfo, slot);
	d->qid.vers = 1;
	d->qid.path = slot;
	d->qid.type = 0;
	return 0;
}

int
makeinfo(int path)
{
	Fmt f;

	if(path < 0 || path > nelem(Infdir))
		return -1;
	if(Infdir[path].buf != nil)
		return 0;
	fmtstrinit(&f);
	if((*Infdir[path].func)(&f) == -1l)
		return -1;
	Infdir[path].buf = fmtstrflush(&f);
	Infdir[path].len = strlen(Infdir[path].buf);
	return 0;
}

int
readinfo(int path, char *buf, int len, int off)
{
	if(path < 0 || path > nelem(Infdir))
		return -1;
	if(off > Infdir[path].len)
		return 0;
	if(len + off > Infdir[path].len)
		len = Infdir[path].len - off;
	memmove(buf, Infdir[path].buf + off, len);
	return len;
}

void
freeinfo(int path)
{
	if(path < 0 || path > nelem(Infdir))
		return;
	free(Infdir[path].buf);
	Infdir[path].buf = nil;
}
