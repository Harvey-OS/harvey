#include "lib9.h"
#include <sys/types.h>
#include <sys/stat.h>

#define ISTYPE(s, t)	(((s)->st_mode&S_IFMT) == t)

static char nullstring[] = "";
static char Enovmem[] = "out of memory";

static Dir*
statconv(struct stat *s, char *name)
{
	Dir *dir;

#ifdef NO
	extern char* GetNameFromID(int);

	uid = GetNameFromID(s->st_uid), NAMELEN);
	gid = GetNameFromID(s->st_gid), NAMELEN);
#endif
	dir = malloc(sizeof(Dir));
	if(dir == nil){
		werrstr(Enovmem);
		return nil;
	}
	dir->name = name;
	dir->uid = dir->gid = dir->muid = nullstring;
	dir->qid.type = ISTYPE(s, _S_IFDIR)? QTDIR: QTFILE;
	dir->qid.path = s->st_ino;
	dir->qid.vers = s->st_mtime;
	dir->mode = (dir->qid.type<<24)|(s->st_mode&0777);
	dir->atime = s->st_atime;
	dir->mtime = s->st_mtime;
	dir->length = s->st_size;
	dir->dev = s->st_dev;
	dir->type = ISTYPE(s, _S_IFIFO)? '|': 'M';
	return dir;
}

Dir*
dirfstat(int fd)
{
	struct stat sbuf;

	if(fstat(fd, &sbuf) < 0)
		return nil;
	return statconv(&sbuf, nullstring);
}

Dir*
dirstat(char *f)
{
	struct stat sbuf;
	char *p;

	if(stat(f, &sbuf) < 0)
		return nil;
	p = strrchr(f, '/');
	if(p)
		p++;
	else
		p = nullstring;
	return statconv(&sbuf, p);
}
