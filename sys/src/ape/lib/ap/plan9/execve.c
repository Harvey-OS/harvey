#include "lib.h"
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <signal.h>
#include "sys9.h"

extern char **environ;

int
execve(const char *name, const char *argv[], const char *envp[])
{
	int n, f, i;
	char **e, *ss, *se;
	Fdinfo *fi;
	unsigned long flags;
	char nam[256+5];
	char buf[1000];

	_RFORK(RFCENVG);
	/*
	 * To pass _fdinfo[] across exec, put lines like
	 *   fd flags oflags
	 * in $_fdinfo (for open fd's)
	 */

	f = _CREATE("#e/_fdinfo", OWRITE, 0666);
	ss = buf;
	for(n = 0; n<OPEN_MAX; n++){
		fi = &_fdinfo[n];
		flags = fi->flags;
		if(flags&FD_CLOEXEC){
			_CLOSE(n);
			fi->flags = 0;
			fi->oflags = 0;
		}else if(flags&FD_ISOPEN){
			ss = _ultoa(ss, n);
			*ss++ = ' ';
			ss = _ultoa(ss, flags);
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
	/*
	 * To pass _sighdlr[] across exec, set $_sighdlr
	 * to list of blank separated fd's that have
	 * SIG_IGN (the rest will be SIG_DFL).
	 * We write the variable, even if no signals
	 * are ignored, in case the current value of the
	 * variable ignored some.
	 */
	f = _CREATE("#e/_sighdlr", OWRITE, 0666);
	if(f >= 0){
		ss = buf;
		for(i = 0; i <=MAXSIG && ss < &buf[sizeof(buf)]-5; i++) {
			if(_sighdlr[i] == SIG_IGN) {
				ss = _ultoa(ss, i);
				*ss++ = ' ';
			}
		}
		_WRITE(f, buf, ss-buf);
		_CLOSE(f);
	}
	if(envp){
		strcpy(nam, "#e/");
		for(e = (char **)envp; (ss = *e); e++) {
			se = strchr(ss, '=');
			if(!se || ss==se)
				continue;	/* what is name? value? */
			n = se-ss;
			if(n >= sizeof(nam)-3)
				n = sizeof(nam)-3-1;
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
