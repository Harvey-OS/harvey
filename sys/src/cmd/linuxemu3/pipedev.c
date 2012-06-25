#include <u.h>
#include <libc.h>
#include <ureg.h>
#include "dat.h"
#include "fns.h"
#include "linux.h"

typedef struct Pipe Pipe;

struct Pipe
{
	Ufile;
	void	*bufproc;
	ulong atime;
	ulong mtime;
	int ino;
};

enum{
	Maxatomic = 64*1024,
};

int
pipewrite(int fd, void *buf, int len)
{
	uchar *p, *e;
	int err, n;

	p = buf;
	e = p + len;
	while(p < e){
		n = e - p;
		if(n > Maxatomic)
			n = Maxatomic;
		if(notifyme(1))
			err = -ERESTART;
		else {
			err = write(fd, p, n);
			notifyme(0);
			if(err < 0)
				err = mkerror();
		}
		if(err < 0){
			if(p == (uchar*)buf)
				return err;
			break;
		}
		p += err;
	}
	return p - (uchar*)buf;
}

static int
closepipe(Ufile *file)
{
	Pipe *pipe = (Pipe*)file;

	close(pipe->fd);
	freebufproc(pipe->bufproc);

	return 0;
}

static void*
bufprocpipe(Pipe *pipe)
{
	if(pipe->bufproc == nil)
		pipe->bufproc = newbufproc(pipe->fd);
	return pipe->bufproc;
}

static int
pollpipe(Ufile *file, void *tab)
{
	Pipe *pipe = (Pipe*)file;

	return pollbufproc(bufprocpipe(pipe), pipe, tab);
}

static int
readpipe(Ufile *file, void *buf, int len, vlong)
{
	Pipe *pipe = (Pipe*)file;
	int ret;

	if((pipe->mode & O_NONBLOCK) || (pipe->bufproc != nil)){
		ret = readbufproc(bufprocpipe(pipe), buf, len, 0, (pipe->mode & O_NONBLOCK));
	} else {
		if(notifyme(1))
			return -ERESTART;
		ret = read(pipe->fd, buf, len);
		notifyme(0);
		if(ret < 0)
			ret = mkerror();
	}
	if(ret > 0)
		pipe->atime = time(nil);
	return ret;
}

static int
writepipe(Ufile *file, void *buf, int len, vlong)
{
	Pipe *pipe = (Pipe*)file;
	int ret;

	ret = pipewrite(pipe->fd, buf, len);
	if(ret > 0)
		pipe->mtime = time(nil);
	return ret;
}

static int
ioctlpipe(Ufile *file, int cmd, void *arg)
{
	Pipe *pipe = (Pipe*)file;

	switch(cmd){
	default:
		return -ENOTTY;
	case 0x541B:
		{
			int r;

			if(arg == nil)
				return -EINVAL;
			if((r = nreadablebufproc(bufprocpipe(pipe))) < 0){
				*((int*)arg) = 0;
				return r;
			}
			*((int*)arg) = r;
		}
		return 0;
	}
}

int sys_pipe(int *fds)
{
	Pipe *file;
	int p[2];
	int i, fd;
	static int ino = 0x1234;

	trace("sys_pipe(%p)", fds);

	if(pipe(p) < 0)
		return mkerror();

	for(i=0; i<2; i++){
		file = kmallocz(sizeof(Pipe), 1);
		file->ref = 1;
		file->mode = O_RDWR;
		file->dev = PIPEDEV;
		file->fd =  p[i];
		file->ino = ino++;
		file->atime = file->mtime = time(nil);
		if((fd = newfd(file, 0)) < 0){
			if(i > 0)
				sys_close(fds[0]);
			close(p[0]);
			close(p[1]);
			return fd;
		}
		fds[i] = fd;
	}
	return 0;
}

static void
fillstat(Pipe *pipe, Ustat *s)
{
	s->ino = pipe->ino;
	s->mode = 0666 | S_IFIFO;
	s->uid = current->uid;
	s->gid = current->gid;
	s->atime = pipe->atime;
	s->mtime = pipe->mtime;
	s->size = 0;
}

static int
fstatpipe(Ufile *file, Ustat *s)
{
	Pipe *pipe = (Pipe*)file;
	fillstat(pipe, s);
	return 0;
};

static Udev pipedev = 
{
	.read = readpipe,
	.write = writepipe,
	.poll = pollpipe,
	.close = closepipe,
	.ioctl = ioctlpipe,
	.fstat = fstatpipe,
};

void pipedevinit(void)
{
	devtab[PIPEDEV] = &pipedev;
}
