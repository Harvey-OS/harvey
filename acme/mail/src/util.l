#include <alef.h>
#include <bio.h>
#include "dat.h"
#include "fns.h"

void
aleferror(byte *line, byte *mesg)
{
	error("Mail: %s: %s\n", line, mesg);
}

void
error(byte *s, ...)
{
	byte buf[256];

	if(s){
		doprint(buf, buf+sizeof buf+1, s, ...);
		fprint(2, "mail: %s\n", buf);
	}
	postnote(PNGROUP, getpid(), "kill");
	exits(buf);
}

/*
 *  try opening a lock file.  If it doesn't exist try creating it.
 */
int
openlockfile(byte *path)
{
	int fd, try;
	Dir d;

	try = 0;
	for(;;){
		fd = open(path, OWRITE);
		if(fd>=0 || ++try>3)
			return fd;
		if(++try > 3)
			return fd;
		if(dirstat(path, &d) >= 0)
			sleep(1000);
		else{
			fd = create(path, OWRITE, CHEXCL|0666);
			if(fd >= 0){
				if(dirfstat(fd, &d) >= 0){
					d.mode |= CHEXCL|0666;
					dirfwstat(fd, &d);
				}
				return fd;
			}
			break;
		}
	}
	return -1;
}

int
tryopen(byte *s, int mode)
{
	int try, fd;

	for(try=0; try<3; try++){
		fd = open(s, mode);
		if(fd >= 0)
			return fd;
		sleep(1000);
	}
	return -1;
}

enum
{
	Nargchar	= 2048,
	Narg		= 200
};


void
run(byte **argv, chan(int) c, int p[2])
{
	byte buf[64];
	byte argchar[Nargchar];
	byte *arg[Narg];
	int na, nc, n;

	rfork(RFENVG|RFFDG|RFNOTEG|RFMEM);
	c <-= getpid();
	nc = 0;
	for(na=0; na<Narg; na++){	/* copy to stack so we don't leak */
		if(argv[na] == nil){
			arg[na] = nil;
			for(n=0; n<na; n++)
				free(argv[n]);
			free(argv);
			argv = arg;
			break;
		}
		n = strlen(argv[na])+1;
		if(nc+n > Nargchar)
			break;
		arg[na] = argchar+nc;
		memmove(argchar+nc, argv[na], n);
		nc += n;
	}
	close(p[1]);
	dup(p[0], 0);
	exec(argv[0], argv);
	sprint(buf, "/bin/%s", argv[0]);
	exec(buf, argv);
	exits("can't exec");
}
