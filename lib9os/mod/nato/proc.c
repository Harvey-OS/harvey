#include "nat.h"

ulong
thread(void(*f)(void*), void *a)
{
	int pid;
	pid=rfork(RFNOWAIT|RFMEM|RFPROC);
	if(pid < 0)
		exits("rfork failed: %r");
	if(pid != 0)
		return pid;
	(*f)(a);
	return 0; // never reaches here
}

static void
fdclose(void)
{
	int fd, n, i;
	Dir **d, **p;

	SET(d);
	if((fd = open("#d", OREAD)) < 0)
		return;

	while((n = dirread(fd, d)) > 0) {
		for(p = d; n > 0; n -= sizeof(Dir), p++) {
			i = atoi((*p)->name);
			if(i > 2)
				close(i);
		}
	}
}

int
proc(char **argv, int fd0, int fd1, int fd2)
{
	int r, flag;
	char *arg0, file[200];

	arg0 = argv[0];

	strcpy(file, arg0);

	if(access(file, 1) < 0) {
		if(strncmp(arg0, "/", 1)==0
		|| strncmp(arg0, "#", 1)==0
		|| strncmp(arg0, "./", 2)==0
		|| strncmp(arg0, "../", 3)==0)
			return 0;
		sprint(file, "/bin/%s", arg0);
		if(access(file, 1) < 0)
			return 0;
	}

	flag = RFPROC|RFFDG|RFENVG|RFNOWAIT;
	if((r = rfork(flag)) != 0) {
		if(r < 0)
			return 0;
		return r;
	}

	if(fd0 != 0) {
		if(fd1 == 0)
			fd1 = dup(0, -1);
		if(fd2 == 0)
			fd2 = dup(0, -1);
		close(0);
		if(fd0 >= 0)
			dup(fd0, 0);
	}

	if(fd1 != 1) {
		if(fd2 == 1)
			fd2 = dup(1, -1);
		close(1);
		if(fd1 >= 0)
			dup(fd1, 1);
	}

	if(fd2 != 2) {
		close(2);
		if(fd2 >= 0)
			dup(fd2, 2);
	}

	fdclose();

	exec(file, argv);
	exits("proc: exec failed: %r");
	return 0;
}

