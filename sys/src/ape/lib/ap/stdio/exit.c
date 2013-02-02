#include <unistd.h>
#define	NONEXIT	34
void (*_atexitfns[NONEXIT])(void);
void _doatexits(void){
	int i;
	void (*f)(void);
	for(i = NONEXIT-1; i >= 0; i--)
		if(_atexitfns[i]){
			f = _atexitfns[i];
			_atexitfns[i] = 0;	/* self defense against bozos */
			(*f)();
		}
}
void exit(int status)
{
	_doatexits();
	_exit(status);
}
