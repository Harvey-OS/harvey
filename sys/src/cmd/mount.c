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
#include <auth.h>

void	usage(void);
void	catch(void *c, char*);

char *keyspec = "";

static int
amount0(int fd, char *mntpt, int flags, char *attachname, int devspec, char *keyspec)
{
	int rv, afd;
	AuthInfo *ai;

	afd = fauth(fd, attachname);
	if(afd >= 0){
		ai = auth_proxy(afd, amount_getkey, "proto=p9any role=client %s", keyspec);
		if(ai != nil)
			auth_freeAI(ai);
		else
			fprint(2, "%s: auth_proxy: %r\n", argv0);
	}
	rv = mount(fd, afd, mntpt, flags, attachname, devspec);
	if(afd >= 0)
		close(afd);
	return rv;
}

void
main(int argc, char *argv[])
{
	char *devspec = "M";
	char *attachname = "";
	uint32_t flag = 0;
	int qflag = 0;
	int noauth = 0;
	int fd, rv;

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
	case 'C':
		flag |= MCACHE;
		break;
	case 'k':
		keyspec = EARGF(usage());
		break;
	case 'M':
		devspec = EARGF(usage());
		break;
	case 'n':
		noauth = 1;
		break;
	case 'q':
		qflag = 1;
		break;
	default:
		usage();
	}ARGEND

	// Why do this test here and not in the switch?
	// In case someone mistakenly messes up the default
	// value of devspec in its declaration. This is insurance.
	if (strlen(devspec) > 1) {
		exits("devspec can only be one UTF-8 character");
	}
	if(argc == 2)
		attachname = "";
	else if(argc == 3)
		attachname = argv[2];
	else
		usage();

	if((flag&MAFTER)&&(flag&MBEFORE))
		usage();

	fd = open(argv[0], ORDWR);
	if(fd < 0){
		if(qflag)
			exits(0);
		fprint(2, "%s: can't open %s: %r\n", argv0, argv[0]);
		exits("open");
	}

	notify(catch);
	if(noauth)
		rv = mount(fd, -1, argv[1], flag, attachname, devspec[0]);
	else
		rv = amount0(fd, argv[1], flag, attachname, devspec[0], keyspec);
	if(rv < 0){
		if(qflag)
			exits(0);
		fprint(2, "%s: mount %s: %r\n", argv0, argv[1]);
		exits("mount");
	}
	exits(0);
}

void
catch(void *x, char *m)
{
	USED(x);
	fprint(2, "mount: %s\n", m);
	exits(m);
}

void
usage(void)
{
	fprint(2, "usage: mount [-a|-b] [-cnq] [-k keypattern] /srv/service dir [spec]\n");
	exits("usage");
}
