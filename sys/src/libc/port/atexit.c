#include <u.h>
#include <libc.h>

#define	NEXIT	33

static struct
{
	void	(*f)(void);
	int	pid;
}onex[NEXIT];

atexit(void (*f)(void))
{
	int i;

	for(i=0; i<NEXIT; i++)
		if(onex[i].f == 0) {
			onex[i].f = f;
			onex[i].pid = getpid();
			return 1;
		}
	return 0;
}

void
atexitdont(void (*f)(void))
{
	int i, pid;

	pid = getpid();
	for(i=0; i<NEXIT; i++)
		if(onex[i].f == f && onex[i].pid == pid)
			onex[i].f = 0;
}

void
exits(char *s)
{
	int i, pid;
	void (*f)(void);

	pid = getpid();

	for(i = NEXIT-1; i >= 0; i--)
		if((f = onex[i].f) && pid == onex[i].pid) {
			onex[i].f = 0;
			(*f)();
		}
	_exits(s);
}
