#include <alef.h>
#include <bio.h>
#include "dat.h"
#include "fns.h"

void
error(byte *s, ...)
{
	byte buf[256];
	int fd;

	if(s){
		doprint(buf, buf+sizeof buf+1, s, ...);
		fprint(2, "acid: %s\n", buf);
	}
	sprint(buf, "/proc/%d/notepg", getpid());
	fd = open(buf, OWRITE);
	rfork(RFNOTEG);
	write(fd, "kill", 4);
	exits(s);
}

void*
emalloc(uint n)
{
	void *p;

	p = malloc(n);
	if(p == nil)
		error("malloc failed: %r");
	return p;
}

void*
erealloc(void *p, uint n)
{
	p = realloc(p, n);
	if(p == nil)
		error("realloc failed: %r");
	return p;
}

void
run(byte **argv, int stdin, chan(int) c)
{
	byte buf[256];

	rfork(RFFDG|RFMEM);
	c <-= getpid();
	dup(stdin, 0);
	close(stdin);
	exec(argv[0], argv);
	sprint(buf, "/bin/%s", argv[0]);
	exec(buf, argv);
	exits("can't exec");
}
