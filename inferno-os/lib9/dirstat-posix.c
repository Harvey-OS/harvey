#include "lib9.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <pwd.h>
#include <grp.h>

static char nullstring[] = "";
static char Enovmem[] = "out of memory";

static Dir*
statconv(struct stat *s, char *name)
{
	struct passwd *p;
	struct group *g;
	Dir *dir;
	int str;
	char *n;

	str = 0;
	p = getpwuid(s->st_uid);
	if(p)
		str += strlen(p->pw_name)+1;
	g = getgrgid(s->st_gid);
	if(g)
		str += strlen(g->gr_name)+1;
	dir = malloc(sizeof(Dir)+str);
	if(dir == nil){
		werrstr(Enovmem);
		return nil;
	}
	n = (char*)dir+sizeof(Dir);
	dir->name = name;
	dir->uid = dir->gid = dir->muid = nullstring;
	if(p){
		dir->uid = n;
		strcpy(n, p->pw_name);
		n += strlen(p->pw_name)+1;
	}
	if(g){
		dir->gid = n;
		strcpy(n, g->gr_name);
	}
	dir->qid.type = S_ISDIR(s->st_mode)? QTDIR: QTFILE;
	dir->qid.path = s->st_ino;
	dir->qid.vers = s->st_mtime;
	dir->mode = (dir->qid.type<<24)|(s->st_mode&0777);
	dir->atime = s->st_atime;
	dir->mtime = s->st_mtime;
	dir->length = s->st_size;
	dir->dev = s->st_dev;
	dir->type = S_ISFIFO(s->st_mode)? '|': 'M';
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
