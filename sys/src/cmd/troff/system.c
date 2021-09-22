/*
 * system - run a shell command (Plan 9 version)
 */

#include <u.h>
#include <libc.h>

int
system(char *s)
{
	long pid = fork();
	Waitmsg *wm;

	if (pid == 0) {
		execl("/bin/rc", "rc", "-c", s, nil);
		_exits("no /bin/rc");
	}
	if (pid < 0)
		return -1;	// "can't fork";
	while ((wm = wait()) != nil && wm->pid != pid)
		continue;
	return wm == nil? -1: wm->msg != 0 && wm->msg[0]? 1: 0;
}
