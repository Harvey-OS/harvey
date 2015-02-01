/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#include <u.h>
#include <libc.h>

void	usage(void);

void
main(int argc, char *argv[])
{
	ulong flag = 0;
	int qflag = 0;

	ARGBEGIN{
	case 'a':
		flag |= MAFTER;
		break;
	case 'b':
		flag |= MBEFORE;
		break;
	case 'c':
		flag |= MCREATE;
		break;
	case 'q':
		qflag = 1;
		break;
	default:
		usage();
	}ARGEND

	if(argc != 2 || (flag&MAFTER)&&(flag&MBEFORE))
		usage();

	if(bind(argv[0], argv[1], flag) < 0){
		if(qflag)
			exits(0);
		/* try to give a less confusing error than the default */
		if(access(argv[0], 0) < 0)
			fprint(2, "bind: %s: %r\n", argv[0]);
		else if(access(argv[1], 0) < 0)
			fprint(2, "bind: %s: %r\n", argv[1]);
		else
			fprint(2, "bind %s %s: %r\n", argv[0], argv[1]);
		exits("bind");
	}
	exits(0);
}

void
usage(void)
{
	fprint(2, "usage: bind [-q] [-b|-a|-c|-bc|-ac] new old\n");
	exits("usage");
}
