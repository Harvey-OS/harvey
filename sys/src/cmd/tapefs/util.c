#include <u.h>
#include <libc.h>
#include <auth.h>
#include <fcall.h>
#include <tapefs.h>
#include <bio.h>

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
	char *suffix;
	Ram *dir, *ent;
	Fileinf f;

	if (*fi.name=='\0')
		return 0;
	if (suffix=strrchr(fi.name, '/')){
		*suffix = 0;
		suffix++;
		if (*suffix=='\0'){
			fi.mode |= CHDIR;
			return poppath(fi, 1);
		}
		f = fi;
		f.size = 0;
		f.addr = 0;
		f.mode = 0555|CHDIR;
		dir = poppath(f, 0);
		if (dir==0)
			dir = ram;
	} else {
		suffix = fi.name;
		dir = ram;
		if (strcmp(suffix, ".")==0)
			return dir;
	}
	ent = lookup(dir, suffix);
	fi.mode |= 0400;			/* at least user read */
	if (ent){
		if ((fi.mode&CHDIR) != (ent->qid.path&CHDIR)){
			fprint(2, "%s/%s directory botch\n", fi.name, suffix);
			exits("");
		}
		if (new)  {
			ent->ndata = fi.size;
			ent->data = fi.addr;
			ent->perm = fi.mode;
			ent->mtime = fi.mdate;
			ent->user = mapid(uidmap, fi.uid);
			ent->group = mapid(gidmap, fi.gid);
		}
	} else {
		fi.name = suffix;
		ent = popfile(dir, fi);
	}
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
	ent->qid.path = ++path | fi.mode&CHDIR;
	ent->qid.vers = 0;
	ent->perm = fi.mode;
	strncpy(ent->name, fi.name, NAMELEN);
	ent->atime = ent->mtime = fi.mdate;
	ent->user = mapid(uidmap, fi.uid);
	ent->group = mapid(gidmap, fi.gid);
	ent->ndata = fi.size;
	ent->data = fi.addr;
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
