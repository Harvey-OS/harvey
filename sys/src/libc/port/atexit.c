#include <u.h>
#include <libc.h>

#define	NEXIT	33
static	void	(*onex[NEXIT])(void);

atexit(void (*f)(void))
{
	int i;

	for(i=0; i<NEXIT; i++)
		if(onex[i] == 0) {
			onex[i] = f;
			return 1;
		}
	return 0;
}

void
atexitdont(void (*f)(void))
{
	int i;

	for(i=0; i<NEXIT; i++)
		if(onex[i] == f)
			onex[i] = 0;
}

void
exits(char *s)
{
	int i;
	void (*f)(void);

	for(i = NEXIT-1; i >= 0; i--)
		if(f = onex[i]) {
			onex[i] = 0;
			(*f)();
		}
	_exits(s);
}
