/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

/*
 * #!/bin/sh
 * case $1 in
 * -n)
 * 	exit 0 ;;
 * -m*|-f*|-r*|-p*|-e*|"")
 * 	exec /usr/lib/upas/edmail $*
 * 	exit $? ;;
 * *)
 * 	exec /usr/lib/upas/send $*
 * 	exit $? ;;
 * esac
 */


extern *UPASROOT;

#define	EDMAIL	"edmail"
#define	SEND	"send"

main (argc, argv)
	int argc;
	char **argv;
{
	char *progname = SEND;
	char realprog[500];

	if (argc > 1) {
		if (argv[1][0] == '-') {
			switch (argv[1][1]) {
			case 'n':
				exit (0);

			case 'm':
			case 'f':
			case 'r':
			case 'p':
			case 'e':
			case '\0':
				progname = EDMAIL;
			}
		}
	} else
		progname = EDMAIL;

	sprint(realprog, "%s/%s", UPASROOT, progname);
	execv (realprog, argv);
	perror (realprog);
	exit (1);
}

