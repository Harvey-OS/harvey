#include	"u.h"
#include	"lib.h"
#include	"dat.h"
#include	"fns.h"
#include	"error.h"

long
sysexits(ulong *arg)
{
	char *status;
	char *inval = "invalid exit string";
	char buf[ERRMAX];

	status = (char*)arg[0];
	if(status){
		if(waserror())
			status = inval;
		else{
			validaddr((ulong)status, 1, 0);
			if(vmemchr(status, 0, ERRMAX) == 0){
				memmove(buf, status, ERRMAX);
				buf[ERRMAX-1] = 0;
				status = buf;
			}
		}
		poperror();

	}
	pexit(status, 1);
	return 0;		/* not reached */
}

