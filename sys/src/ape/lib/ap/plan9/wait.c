#include "lib.h"
#include <stdlib.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>
#include <stdio.h>
#include "sys9.h"

pid_t
wait(int *stat_loc)
{
	return waitpid(-1, stat_loc, 0);
}

pid_t
waitpid(int pid, int *stat_loc, int options)
{
	int n, i, wfd, r, t, wpid;
	char *bp, *ep, pname[50];
	struct stat buf;
	Waitmsg *w;

	if(options&WNOHANG){
		sprintf(pname, "/proc/%d/wait", getpid());
		i = stat(pname, &buf);
		if(i >=0 && buf.st_size==0)
			return 0;
	}
	n = 0;
	while(n==0){
		w = _WAIT();
		if(w == 0){
			_syserrno();
		}else{
			wpid = w->pid;
			if(pid>0 && wpid!=pid){
				free(w);
				continue;
			}
			n = wpid;
			if(stat_loc){
				r = 0;
				t = 0;
				if(w->msg[0]){
					/* message is 'prog pid:string' */
					bp = w->msg;
					while(*bp){
						if(*bp++ == ':')
							break;
					}
					if(*bp == 0)
						bp = w->msg;
					r = strtol(bp, &ep, 10);
					if(*ep == 0){
						if(r < 0 || r >= 256)
							r = 1;
					}else{
						t = _stringsig(bp);
						if(t == 0)
							r = 1;
					}
				}
				*stat_loc = (r << 8) | t;
			}
			free(w);
		}
	}
	return n;
}

