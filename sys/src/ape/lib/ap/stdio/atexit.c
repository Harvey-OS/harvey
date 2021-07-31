#include <unistd.h>
#define	NONEXIT	34
extern int (*_atexitfns[NONEXIT])(void);

int
atexit(int (*f)(void))
{
	int i;
	for(i=0; i<NONEXIT; i++)
		if(!_atexitfns[i]){
			_atexitfns[i] = f;
			return(0);
		}
	return(1);
}
