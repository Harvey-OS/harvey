#include "lib.h"
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include "sys9.h"

extern char **environ;

/*
 * BUG: to protect environment, need a new pgrp for now
 */

int
execve(const char *name, const char *argv[], const char *envp[])
{
	int n, f, i;
	char **e, *ss, *se;
	Fdinfo *fi;
	char nam[NAMELEN+5];
	char buf[1000];

	/*
	 * To pass _fdinfo[] across exec, put lines like
	 *   fd flags oflags
	 * in $_fdinfo (for open fd's)
	 */

	f = _CREATE("#e/_fdinfo", OWRITE, 0666);
	ss = buf;
	for(n = 0; n<OPEN_MAX; n++){
		fi = &_fdinfo[n];
		if(fi->flags&FD_CLOEXEC){
			_CLOSE(n);
			fi->flags = 0;
			fi->oflags = 0;
		}else if(fi->flags&FD_ISOPEN){
			ss = _ultoa(ss, n);
			*ss++ = ' ';
			ss = _ultoa(ss, fi->flags);
			*ss++ = ' ';
			ss = _ultoa(ss, fi->oflags);
			*ss++ = '\n';
			if(ss-buf < sizeof(buf)-50){
				_WRITE(f, buf, ss-buf);
				ss = buf;
			}
		}
	}
	if(ss > buf)
		_WRITE(f, buf, ss-buf);
	_CLOSE(f);
	if(envp && envp != environ){
		strcpy(nam, "#e/");
		for(e = envp; (ss = *e); e++) {
			se = strchr(ss, '=');
			if(!se || ss==se)
				continue;	/* what is name? value? */
			n = se-ss;
			if(n > NAMELEN-1)
				n = NAMELEN-1;
			memcpy(nam+3, ss, n);
			nam[3+n] = 0;
			f = _CREATE(nam, OWRITE, 0666);
			if(f < 0)
				continue;
			se++; /* past = */
			n = strlen(se);
			/* temporarily decode nulls (see _envsetup()) */
			for(i=0; i < n; i++)
				if(se[i] == 1)
					se[i] = 0;
			_WRITE(f, se, n);
			/* put nulls back */
			for(i=0; i < n; i++)
				if(se[i] == 0)
					se[i] = 1;
			_CLOSE(f);
		}
	}
	n = _EXEC(name, argv);
	_syserrno();
	return n;
}
