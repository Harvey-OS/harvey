/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#include "stdinc.h"

#include "9.h"

typedef struct {
	int8_t*	argv0;
	int	(*cmd)(int, int8_t*[]);
} Cmd;

static struct {
	VtLock*	lock;
	Cmd*	cmd;
	int	ncmd;
	int	hi;
} cbox;

enum {
	NCmdIncr	= 20,
};

int
cliError(int8_t* fmt, ...)
{
	int8_t *p;
	va_list arg;

	va_start(arg, fmt);
	p = vsmprint(fmt, arg);
	vtSetError("%s", p);
	free(p);
	va_end(arg);

	return 0;
}

int
cliExec(int8_t* buf)
{
	int argc, i, r;
	int8_t *argv[20], *p;

	p = vtStrDup(buf);
	if((argc = tokenize(p, argv, nelem(argv)-1)) == 0){
		vtMemFree(p);
		return 1;
	}
	argv[argc] = 0;

	if(argv[0][0] == '#'){
		vtMemFree(p);
		return 1;
	}

	vtLock(cbox.lock);
	for(i = 0; i < cbox.hi; i++){
		if(strcmp(cbox.cmd[i].argv0, argv[0]) == 0){
			vtUnlock(cbox.lock);
			if(!(r = cbox.cmd[i].cmd(argc, argv)))
				consPrint("%s\n", vtGetError());
			vtMemFree(p);
			return r;
		}
	}
	vtUnlock(cbox.lock);

	consPrint("%s: - eh?\n", argv[0]);
	vtMemFree(p);

	return 0;
}

int
cliAddCmd(int8_t* argv0, int (*cmd)(int, int8_t*[]))
{
	int i;
	Cmd *opt;

	vtLock(cbox.lock);
	for(i = 0; i < cbox.hi; i++){
		if(strcmp(argv0, cbox.cmd[i].argv0) == 0){
			vtUnlock(cbox.lock);
			return 0;
		}
	}
	if(i >= cbox.hi){
		if(cbox.hi >= cbox.ncmd){
			cbox.cmd = vtMemRealloc(cbox.cmd,
					(cbox.ncmd+NCmdIncr)*sizeof(Cmd));
			memset(&cbox.cmd[cbox.ncmd], 0, NCmdIncr*sizeof(Cmd));
			cbox.ncmd += NCmdIncr;
		}
	}

	opt = &cbox.cmd[cbox.hi];
	opt->argv0 = argv0;
	opt->cmd = cmd;
	cbox.hi++;
	vtUnlock(cbox.lock);

	return 1;
}

int
cliInit(void)
{
	cbox.lock = vtLockAlloc();
	cbox.cmd = vtMemAllocZ(NCmdIncr*sizeof(Cmd));
	cbox.ncmd = NCmdIncr;
	cbox.hi = 0;

	return 1;
}
