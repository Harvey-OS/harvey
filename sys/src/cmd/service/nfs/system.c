#include <u.h>
#include <libc.h>

int
system(Waitmsg *w, char *name, char **argv)
{
	char err[ERRLEN];
	int pid;

	switch(pid = fork()){	/* assign = */
	case -1:
		return -1;
	case 0:
		exec(name, argv);
		errstr(err);
		_exits(err);
	}
	for(;;){
		if(wait(w) < 0)
			break;
		if(strtol(w->pid, 0, 10) == pid)
			return 0;
	}
	return -1;
}

int
systeml(Waitmsg *w, char *name, ...)
{
	return system(w, name, &name+1);
}
