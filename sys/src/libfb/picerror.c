#include <u.h>
#include <libc.h>
#include <libg.h>
#include <fb.h>
void picerror(char *s){
	if(_PICerror){
		if(s) fprint(2, "%s: ", s);
		fprint(2, "%s\n", _PICerror);
		_PICerror=0;
	}
	else
		perror(s);
}
