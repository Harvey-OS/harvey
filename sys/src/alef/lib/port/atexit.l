#include <alef.h>

#define	NEXIT	33
intern	void	(*onex[NEXIT])(void);

int
atexit(void (*f)(void))
{
	int i;

	for(i=0; i<NEXIT; i++)
		if(onex[i] == nil) {
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
			onex[i] = nil;
}

void
_exits(char *s)
{
	int i;
	void (*f)(void);

	for(i = NEXIT-1; i >= 0; i--)
		if(f = onex[i]) {
			onex[i] = nil;
			(*f)();
		}
	__exits(s);
}
