#include "lib.h"
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>

int
system(const char *s)
{
	int w, status;
	pid_t pid;

	if(!s)
		return 1; /* a command interpreter is available */
	pid = fork();
	if(pid == 0) {
		execl("/bin/rc", "rc", "-c", s, 0);
		_exit(1);
	}
	if(pid < 0){
		_syserrno();
		return -1;
	}
	for(;;) {
		w = wait(&status);
		if(w == -1 || w == pid)
			break;
	}

	if(w == -1){
		_syserrno();
		return w;
	}
	return status;
}
