#include <u.h>
#include <libc.h>

static Lock	privlock;
static int	privinit;
static void	**privs;

extern void	**_privates;
extern int	_nprivates;

void **
privalloc(void)
{
	void **p;
	int i;

	lock(&privlock);
	if(!privinit){
		privinit = 1;
		if(_nprivates){
			_privates[0] = 0;
			for(i = 1; i < _nprivates; i++)
				_privates[i] = &_privates[i - 1];
			privs = &_privates[i - 1];
		}
	}
	p = privs;
	if(p != nil){
		privs = *p;
		*p = nil;
	}
	unlock(&privlock);
	return p;
}

void
privfree(void **p)
{
	lock(&privlock);
	if(p != nil && privinit){
		*p = privs;
		privs = p;
	}
	unlock(&privlock);
}
