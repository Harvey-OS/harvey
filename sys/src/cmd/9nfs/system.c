#include <u.h>
#include <libc.h>

Waitmsg*
system(char *name, char **argv)
{
	char err[ERRMAX];
	Waitmsg *w;
	int pid;

	switch(pid = fork()){	/* assign = */
	case -1:
		return nil;
	case 0:
		exec(name, argv);
		errstr(err, sizeof err);
		_exits(err);
	}
	for(;;){
		w = wait();
		if(w == nil)
			break;
		if(w->pid == pid)
			return w;
		free(w);
	}
	return nil;
}

Waitmsg*
systeml(char *name, ...)
{
	return system(name, &name+1);
}
