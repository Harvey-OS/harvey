#include <u.h>
#include <libc.h>

#define	NEXIT	33

typedef struct Onex Onex;
struct Onex{
	void	(*f)(void);
	int	pid;
};

static Lock onexlock;
Onex onex[NEXIT];

int
atexit(void (*f)(void))
{
	int i;

	lock(&onexlock);
	for(i=0; i<NEXIT; i++)
		if(onex[i].f == 0) {
			onex[i].pid = getpid();
			onex[i].f = f;
			unlock(&onexlock);
			return 1;
		}
	unlock(&onexlock);
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

#pragma profile off

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

#pragma profile on
