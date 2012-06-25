#include <u.h>
#include <libc.h>
#include <ureg.h>
#include <mp.h>
#include <libsec.h>

#include "dat.h"
#include "fns.h"
#include "linux.h"

enum
{
	Mnull,
	Mzero,
	Mfull,
	Mrandom,
	Murandom,
	Mmax,
};

typedef struct Miscfile Miscfile;
struct Miscfile
{
	Ufile;
	int	m;
};

static int
path2m(char *path)
{
	int m;

	m = -1;
	if(strcmp(path, "/dev/null")==0){
		m = Mnull;
	} else if(strcmp(path, "/dev/zero")==0){
		m = Mzero;
	} else if(strcmp(path, "/dev/full")==0){
		m = Mfull;
	} else if(strcmp(path, "/dev/random")==0){
		m = Mrandom;
	} else if(strcmp(path, "/dev/urandom")==0){
		m = Murandom;
	}

	return m;
}

static int
openmisc(char *path, int mode, int, Ufile **pf)
{
	Miscfile *f;
	int m;

	if((m = path2m(path)) < 0)
		return -ENOENT;
	f = kmallocz(sizeof(*f), 1);
	f->ref = 1;
	f->mode = mode;
	f->path = kstrdup(path);
	f->fd = -1;
	f->dev = MISCDEV;
	f->m = m;
	*pf = f;
	return 0;
}

static int
closemisc(Ufile *)
{
	return 0;
}

static int
readmisc(Ufile *f, void *buf, int len, vlong)
{
	switch(((Miscfile*)f)->m){
	case Mnull:
		return 0;
	case Mzero:
		memset(buf, 0, len);
		return len;
	case Mfull:
		return -EIO;
	case Mrandom:
		genrandom(buf, len);
		return len;
	case Murandom:
		prng(buf, len);
		return len;
	default:
		return -EIO;
	}
}

static int
writemisc(Ufile *f, void *, int len, vlong)
{
	switch(((Miscfile*)f)->m){
	case Mnull:
	case Mzero:
	case Mrandom:
	case Murandom:
		return len;
	case Mfull:
		return -ENOSPC;
	default:
		return -EIO;
	}
}

static int
statmisc(char *path, int, Ustat *s)
{
	if(path2m(path) < 0)
		return -ENOENT;

	s->mode = 0666 | S_IFCHR;
	s->uid = current->uid;
	s->gid = current->gid;
	s->size = 0;
	s->ino = hashpath(path);
	s->dev = 0;
	s->rdev = 0;
	s->atime = s->mtime = s->ctime = boottime/1000000000LL;
	return 0;
}

static int
fstatmisc(Ufile *f, Ustat *s)
{
	return fsstat(f->path, 0, s);
};

static Udev miscdev =
{
	.open = openmisc,
	.read = readmisc,
	.write = writemisc,
	.close = closemisc,
	.stat = statmisc,
	.fstat = fstatmisc,
};

void miscdevinit(void)
{
	devtab[MISCDEV] = &miscdev;

	fsmount(&miscdev, "/dev/null");
	fsmount(&miscdev, "/dev/zero");
	fsmount(&miscdev, "/dev/full");
	fsmount(&miscdev, "/dev/random");
	fsmount(&miscdev, "/dev/urandom");

	srand(truerand());
}
