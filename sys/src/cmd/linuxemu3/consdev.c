#include <u.h>
#include <libc.h>
#include <ureg.h>
#include "dat.h"
#include "fns.h"
#include "linux.h"

typedef struct Cons Cons;

struct Cons
{
	Ufile;
	void	*bufproc;
};

static int
closecons(Ufile *file)
{
	Cons *cons = (Cons*)file;

	freebufproc(cons->bufproc);

	return 0;
}

static void*
bufproccons(Cons *cons)
{
	if(cons->bufproc == nil)
		cons->bufproc = newbufproc(0);
	return cons->bufproc;
}

static int
pollcons(Ufile *file, void *tab)
{
	Cons *cons = (Cons*)file;
	return pollbufproc(bufproccons(cons), cons, tab);
}

static int
readcons(Ufile *file, void *buf, int len, vlong)
{
	Cons *cons = (Cons*)file;
	int ret;

	if((cons->mode & O_NONBLOCK) || (cons->bufproc != nil)){
		ret = readbufproc(bufproccons(cons), buf, len, 0, (cons->mode & O_NONBLOCK));
	} else {
		if(notifyme(1))
			return -ERESTART;
		ret = read(0, buf, len);
		notifyme(0);
		if(ret < 0)
			ret = mkerror();
	}
	return ret;
}

static int
writecons(Ufile *, void *buf, int len, vlong)
{
	int ret;

	if(notifyme(1))
		return -ERESTART;
	ret = write(1, buf, len);
	notifyme(0);
	if(ret < 0)
		ret = mkerror();
	return ret;
}

static int
ioctlcons(Ufile *file, int cmd, void *arg)
{
	Cons *cons = (Cons*)file;

	switch(cmd){
	default:
		return -ENOTTY;

	case 0x541B:
		{
			int r;

			if(arg == nil)
				return -EINVAL;
			if((r = nreadablebufproc(bufproccons(cons))) < 0){
				*((int*)arg) = 0;
				return r;
			}
			*((int*)arg) = r;
		}
		return 0;
	}
}

static int
opencons(char *path, int mode, int, Ufile **pf)
{
	Cons *file;

	if(strcmp(path, "/dev/cons")!=0)
		return -ENOENT;

	file = mallocz(sizeof(Cons), 1);
	file->ref = 1;
	file->mode = mode;
	file->dev = CONSDEV;
	file->fd = 0;
	file->path = kstrdup(path);
	*pf = file;

	return 0;
}

static int
statcons(char *path, int, Ustat *s)
{
	if(strcmp(path, "/dev/cons")!=0)
		return -ENOENT;

	s->mode = 0666 | S_IFCHR;
	s->uid = current->uid;
	s->gid = current->gid;
	s->size = 0;
	s->ino = hashpath(path);
	s->dev = 0;
	s->rdev = 0;
	return 0;
}

static int
fstatcons(Ufile *f, Ustat *s)
{
	return fsstat(f->path, 0, s);
};

static Udev consdev = 
{
	.open = opencons,
	.read = readcons,
	.write = writecons,
	.poll = pollcons,
	.close = closecons,
	.ioctl = ioctlcons,
	.fstat = fstatcons,
	.stat = statcons,
};

void consdevinit(void)
{
	devtab[CONSDEV] = &consdev;

	fsmount(&consdev, "/dev/cons");
}
