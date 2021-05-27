#include <u.h>
#include <libc.h>

static Lock	privlock;
static int	privinit;
static u32int	privmap;

extern void	**_privates;
extern int	_nprivates;

void **
privalloc(void)
{
	void **p;
	int i;

	lock(&privlock);
	for(i = 0; i < 32 && i < _nprivates; i++){
		if((privmap & (1<<i)) == 0){
			privmap |= 1<<i;
			unlock(&privlock);
			p = &_privates[i];
			*p = nil;
			return p;
		}
	}
	unlock(&privlock);
	return nil;
}

void
privfree(void **p)
{
	int i;

	if(p != nil){
		i = p - _privates;
		if(i < 0 || i > _nprivates || (privmap & (1<<i)) == 0)
			abort();
		lock(&privlock);
		privmap &= ~(1<<i);
		unlock(&privlock);
	}
}
