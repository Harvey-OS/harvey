#include "lib.h"
#include <stdlib.h>
#include <sys/wait.h>
#include <errno.h>
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
	char *ep;
	Waitmsg w;

	n = 0;
	while(n==0){
		n = _WAIT(&w);
		if(n < 0){
			_syserrno();
		}else{
			wpid = strtol(w.pid, 0, 0);
			_delcpid(wpid);
			if(pid>0 && wpid!=pid)
				continue;
			n = wpid;
			if(stat_loc){
				r = 0;
				t = 0;
				if(w.msg[0]){
					r = strtol(w.msg, &ep, 10);
					if(*ep == 0){
						if(r < 0 || r >= 256)
							r = 1;
					}else{
						t = _stringsig(w.msg);
						if(t == 0)
							r = 1;
					}
				}
				*stat_loc = (r << 8) | t;
			}
		}
	}
	return n;
}

