#include	"u.h"
#include	"../port/lib.h"
#include	"mem.h"
#include	"dat.h"
#include	"fns.h"
#include	"io.h"
#include	"ureg.h"
#include	"pool.h"
#include	"../port/error.h"
#include	"../port/netif.h"
#include	"dosfs.h"

enum {
	Bufsize = 8192,
};

/*
 *  grab next element from a path, return the pointer to unprocessed portion of
 *  path.
 */
char *
nextelem(char *path, char *elem)
{
	int i;

	while(*path == '/')
		path++;
	if(*path==0 || *path==' ')
		return 0;
	for(i=0; *path!='\0' && *path!='/' && *path!=' '; i++){
		if(i==NAMELEN){
			print("name component too long\n");
			return 0;
		}
		*elem++ = *path++;
	}
	*elem = '\0';
	return path;
}

int
fswalk(Bootfs *fs, char *path, File *f)
{
	char element[NAMELEN];

	*f = fs->root;
	if(BADPTR(fs->walk))
		panic("fswalk bad pointer fs->walk");

	f->path = path;
	while(path = nextelem(path, element)){
		switch(fs->walk(f, element)){
		case -1:
			return -1;
		case 0:
			return 0;
		}
	}
	return 1;
}

/*
 *  boot
 */
int
fsboot(Bootfs *fs, char *path, Boot *b)
{
	long n;
	char *buf;
	File file;

	memset(&file, 0, sizeof file);
	switch(fswalk(fs, path, &file)){
	case -1:
		print("error walking to %s\n", path);
		return -1;
	case 0:
		print("%s not found\n", path);
		return -1;
	case 1:
		print("found %s\n", path);
		break;
	}
	buf = smalloc(Bufsize);
	while((n = fsread(&file, buf, Bufsize)) > 0) {
		if(bootpass(b, buf, n) != MORE)
			break;
	}

	bootpass(b, nil, 0);	/* tries boot */
	free(buf);
	return -1;
}

int
fsread(File *file, void *a, long n)
{
	if(BADPTR(file->fs))
		panic("bad pointer file->fs in fsread");
	if(BADPTR(file->fs->read))
		panic("bad pointer file->fs->read in fsread");
	return file->fs->read(file, a, n);
}
