#include <unistd.h>
#define	NONEXIT	34
extern void (*_atexitfns[NONEXIT])(void);

int
atexit(void (*f)(void))
{
	int i;
	for(i=0; i<NONEXIT; i++)
		if(!_atexitfns[i]){
			_atexitfns[i] = f;
			return(0);
		}
	return(1);
}
