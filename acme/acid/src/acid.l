#include <alef.h>
#include <bio.h>
#include "dat.h"
#include "fns.h"

void
srvremove(void)
{
	remove(srvenv);
}

void
srvpipe(void)
{
	int fd, p[2];
	byte buf[32];

	if(pipe(p) < 0)
		error("can't create pipe: %r\n");
	srvfd = p[0];
	sprint(srvenv, "/srv/Acid%d", getpid());
	fd = create(srvenv, 1, 0666);
	sprint(buf, "%d", p[1]);
	if(write(fd, buf, strlen(buf)) != strlen(buf))
		error("can't post %s: %r\n", srvenv);
	close(fd);
	fd = create("/env/Acidsrv", ORDWR, 0666);
	if(fd < 0)
		error("can't create /env/Acidsrv: %r\n");
	if(write(fd, srvenv, strlen(srvenv)) != strlen(srvenv))
		error("can't write /env/Acidsrv: %r\n");
	close(fd);
	atexit(srvremove);
}

void
srvproc(chan(byte*) c)
{
	byte *b;
	int n;

	for(;;){
		b = emalloc(1024+1);
		n = read(srvfd, b, 1024);
		if(n <= 0)
			error("short read on srv pipe: %r\n");
		c <-= b;
	}
}

void
copyproc(int from, int to, int closeme, chan(int) c)
{
	byte b[2048];
	int n;

	rfork(RFFDG);
	c <-= 1;
	close(closeme);
	while((n=read(from, b, sizeof b)) > 0)
		if(write(to, b, n) != n)
			error("short write on stdin: %r\n");
	error(nil);
}

void
main(int argc, byte **argv)
{
	int p[2];
	chan(int) c;
	byte *b;
	Awin *a;

	if(argc <= 1)
		error("usage: Acid pid\n");
	if(pipe(p) < 0)
		error("can't pipe stdin: %r\n");

	alloc c;
	proc run(argv+1, p[0], c);
	<-c;
	rfork(RFNOTEG);

	srvpipe();
	alloc srvc;
	proc srvproc(srvc);

	stdin = p[1];
	proc copyproc(0, stdin, p[0], c);
	<-c;
	close(p[0]);

	for(;;){
		b = <-srvc;
		if(strncmp(b, "New", 3) == 0){
			a = emalloc(sizeof(Awin));
			a->new(b+3);
			fprint(srvfd, "/mnt/8½/%d", a->winid);
		}
		free(b);
	}
	error(nil);
}
