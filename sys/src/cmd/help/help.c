#include <u.h>
#include <libc.h>
#include <libg.h>
#include <frame.h>
#include "dat.h"
#include "fns.h"

int	mouseslave;
int	kbdslave;
int	cfd;		/* client end of pipe */
int	sfd;		/* service end of pipe */
int	clockfd;	/* /dev/time */
int	timefd;		/* /dev/cputime */
int	kbdc;
Proc	*pmouse;
Proc	*pkbd;
Rune	genbuf[4096];
Bitmap	*lightgrey;
Bitmap	*darkgrey;
Proc	*wakemouse;


int	mkslave(char*, int, int);
void	killslaves(void);
void	kbdctl(void);
void	mousectl(void);

void
main(int argc, char *argv[])
{
	char *fn;
	int fd, p[2];
	Dir d;
	Rectangle r;


	if(access("/mnt/help/new", 0) == 0){
		fprint(2, "help: already running\n");
		exits("running");
	}
	fn = 0;
	ARGBEGIN{
	case 'f':
		fn = ARGF();
		break;
	}ARGEND

	binit(error, fn, "help");
	if(fn){
		fd = create("/env/font", 1, 0664);
		if(fd >= 0){
			write(fd, fn, strlen(fn));
			close(fd);
		}
	}
	textureinit();
	dirfstat(bitbltfd, &d);
	r = screen.r;
	if(d.type != 'b')
		r = inset(r, 4);
	bitblt(&screen, r.min, &screen, r, 0);
	bflush();

	/*
	 * File server
	 */
	if((clockfd=open("/dev/time", OREAD)) < 0)
		error("/dev/time");
	if((timefd=open("/dev/cputime", OREAD)) < 0)
		error("/dev/cputime");
	if((fd=open("/dev/user", OREAD)) < 0)
		error("/dev/user");
	read(fd, user, NAMELEN-1);
	close(fd);
	if(pipe(p) < 0)
		error("pipe");
	cfd = p[0];
	sfd = p[1];

	/*
	 * Slaves
	 */
	mouseslave = mkslave("/dev/mouse", sizeof(mouse.data), 0);
	kbdslave = mkslave("/dev/cons", 1, 1);
	atexit(killslaves);

	/*
	 * Do something
	 */
	rootpage = newpage(0, 0);
	rootpage->visible = 1;
	rootpage->r = inset(screen.r, 3);
	pagedraw(rootpage, inset(screen.r, 3), 1);
	textinsert(&rootpage->tag, L"\thelp/Boot\tExit", 0, 0, 1);
	pagesplit(rootpage, 0);
	pageadd(rootpage, 0);
	pagesplit(rootpage->down, 1);
	pagesplit(rootpage->down->next, 1);/**/

	/*
	 * Start processes
	 */
	run(newproc(control, 0));
	pkbd = newproc(kbdctl, 0);
	pmouse = newproc(mousectl, 0);
	proc.p = pmouse;
	longjmp(pmouse->label, 1);
}

int
mkslave(char *file, int count, int raw)
{
	int fd1, fd2, fd3, n, pid;
	char buf[64];

	fd1 = open(file, OREAD);
	if(fd1 < 0)
		error(file);
	switch(pid = rfork(RFENVG|RFNAMEG|RFNOTEG|RFFDG|RFPROC|RFNOWAIT)){
	case 0:
		close(sfd);
		close(clockfd);
		close(timefd);
		bclose();
		if(raw){
			fd3 = open("/dev/consctl", OWRITE);
			if(fd3 < 0)
				error("slave consctl open");
			write(fd3, "rawon", 5);
		}
		if(mount(cfd, "/dev", MBEFORE, "slave", "") < 0)
			error("slave mount");
		close(cfd);
		fd2 = open(file, OWRITE);
		if(fd2 < 0)
			error("slave open");
		while((n=read(fd1, buf, count)) >= 0)
			if(write(fd2, buf, n) != n)
				error("slave write");
		fprint(2, "%s slave shut down", file);
		exits(0);

	case -1:
		error("fork");
	}
	close(fd1);
	return pid;
}

void
killslaves(void)
{
	char buf[32];
	int fd;

	sprint(buf, "/proc/%d/note", mouseslave);
	fd = open(buf, OWRITE);
	if(fd > 0){
		write(fd, "die", 3);
		close(fd);
	}
	sprint(buf, "/proc/%d/note", kbdslave);
	fd = open(buf, OWRITE);
	if(fd > 0){
		write(fd, "die", 3);
		close(fd);
	}
}

void
kbdctl(void)
{
	static char buf[8];
	static int nkbdc;
	int n;
	Rune r;

	for(;;){
		clicktime = 0;
		buf[nkbdc++] = kbdc;
		if(fullrune(buf, nkbdc)){
			n = chartorune(&r, buf);
			type(r);
			memmove(buf, buf+n, nkbdc-n);
			nkbdc -= n;
		}
		sched();
	}
}

void
mousectl(void)
{
	static int i;
	uchar *d = mouse.data;

	/*
	 * Mouse is bootstrap process.  Just call sched to get things going.
	 */

	for(;;){
		sched();
		if(d[0] != 'm')
			error("mouse not m");
		if(d[1] & 0x80){
			screen.r = bscreenrect(&screen.clipr);
			pagedraw(rootpage, inset(screen.r, 3), 1);
		}
		mouse.buttons = d[1] & 7;
		mouse.xy.x = BGLONG(d+2);
		mouse.xy.y = BGLONG(d+6);
		mouse.msec = BGLONG(d+10);
		if(wakemouse){
			run(wakemouse);
			wakemouse->mouse = mouse;
			wakemouse = 0;
		}
	}
}

void
frgetmouse(void)
{
	bflush();
	wakemouse = proc.p;
	sched();
}
