/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#include <u.h>
#include <libc.h>
#include <auth.h>
#include <fcall.h>
#include <bio.h>
#include "tapefs.h"

Idmap *
getpass(char *file)
{
	Biobuf *bp;
	char *cp;
	Idmap *up;
	int nid, maxid;
	char *line[4];

	if ((bp = Bopen(file, OREAD)) == 0)
		error("Can't open passwd/group");
	up = emalloc(1*sizeof(Idmap));
	maxid = 1;
	nid = 0;
	while ((cp = Brdline(bp, '\n'))) {
		int nf;
		cp[Blinelen(bp)-1] = 0;
		nf = getfields(cp, line, 3, 0, ":\n");
		if (nf<3) {
			fprint(2, "bad format in %s\n", file);
			break;
		}
		if (nid>=maxid) {
			maxid *= 2;
			up = (Idmap *)erealloc(up, maxid*sizeof(Idmap));
		}
		up[nid].id = atoi(line[2]);
		up[nid].name = strdup(line[0]);
		nid++;
	}
	Bterm(bp);
	up[nid].name = 0;
	return up;
}

char *
mapid(Idmap *up, int id)
{
	char buf[16];

	if (up)
		while (up->name){
			if (up->id==id)
				return strdup(up->name);
			up++;
		}
	sprint(buf, "%d", id);
	return strdup(buf);
}

Ram *
poppath(Fileinf fi, int new)
{
	char *suffix, *origname;
	Ram *dir, *ent;
	Fileinf f;

	if (*fi.name=='\0')
		return 0;
	origname = estrdup(fi.name);
	if (suffix=strrchr(fi.name, '/')){
		*suffix = 0;
		suffix++;
		if (*suffix=='\0'){
			fi.mode |= DMDIR;
			free(origname);
			return poppath(fi, 1);
		}
		/*
		 * create parent directory of suffix;
		 * may recurse, thus shortening fi.name even further.
		 */
		f = fi;
		f.size = 0;
		f.addr = 0;
		f.mode = 0555|DMDIR;
		dir = poppath(f, 0);
		if (dir==0)
			dir = ram;
	} else {
		suffix = fi.name;
		dir = ram;
		if (strcmp(suffix, ".")==0) {
			free(origname);
			return dir;
		}
	}
	ent = lookup(dir, suffix);
	fi.mode |= 0400;			/* at least user read */
	if (ent){
		if (((fi.mode&DMDIR)!=0) != ((ent->qid.type&QTDIR)!=0)){
			fprint(2,
		"%s file type changed; probably due to union dir.; ignoring\n",
				origname);
			free(origname);
			return ent;
		}
		if (new)  {
			ent->ndata = fi.size;
			ent->addr = fi.addr;
			ent->data = fi.data;
			ent->perm = fi.mode;
			ent->mtime = fi.mdate;
			ent->user = mapid(uidmap, fi.uid);
			ent->group = mapid(gidmap, fi.gid);
		}
	} else {
		fi.name = suffix;
		ent = popfile(dir, fi);
	}
	free(origname);
	return ent;
}

Ram *
popfile(Ram *dir, Fileinf fi)
{
	Ram *ent = (Ram *)emalloc(sizeof(Ram));
	if (*fi.name=='\0')
		return 0;
	ent->busy = 1;
	ent->open = 0;
	ent->parent = dir;
	ent->next = dir->child;
	dir->child = ent;
	ent->child = 0;
	ent->qid.path = ++path;
	ent->qid.vers = 0;
	if(fi.mode&DMDIR)
		ent->qid.type = QTDIR;
	else
		ent->qid.type = QTFILE;
	ent->perm = fi.mode;
	ent->name = estrdup(fi.name);
	ent->atime = ent->mtime = fi.mdate;
	ent->user = mapid(uidmap, fi.uid);
	ent->group = mapid(gidmap, fi.gid);
	ent->ndata = fi.size;
	ent->data = fi.data;
	ent->addr = fi.addr;
	ent->replete |= replete;
	return ent;
}

Ram *
lookup(Ram *dir, char *name)
{
	Ram *r;

	if (dir==0)
		return 0;
	for (r=dir->child; r; r=r->next){
		if (r->busy==0 || strcmp(r->name, name)!=0)
			continue;
		return r;
	}
	return 0;
}
