/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#include "stdinc.h"
#include "dat.h"
#include "fns.h"

char TraceDisk[] = "disk";
char TraceLump[] = "lump";
char TraceBlock[] = "block";
char TraceProc[] = "proc";
char TraceWork[] = "work";
char TraceQuiet[] = "quiet";
char TraceRpc[] = "rpc";

void
trace(char *level, char *fmt, ...)
{
	char buf[512];
	va_list arg;

	if(level == nil || !ventilogging)
		return;
	va_start(arg, fmt);
	vsnprint(buf, sizeof buf, fmt, arg);
	va_end(arg);
	vtlog(level, "<font size=-1>%T %s:</font> %s<br>\n",
			threadgetname(), buf);
	vtlog("all", "<font size=-1>%T <font color=#777777>%s</font> %s:</font> %s<br>\n",
			level, threadgetname(), buf);
}

void
traceinit(void)
{
}

void
settrace(char *trace)
{
	USED(trace);
}
