#include <u.h>
#include <libc.h>
#include <tos.h>

int
getpid(void)
{
	char b[20];
	int f, n;

	/* _tos could be unset if we bypassed main9[p].s */
	if (_tos && _tos->pid)
		return _tos->pid;

	/* else kernel screwed up */
	n = 0;
	f = open("#c/pid", 0);
	if(f >= 0) {
		n = read(f, b, sizeof b - 1);
		close(f);
	}
	b[n < 0? 0: n] = '\0';
	return atol(b);
}
