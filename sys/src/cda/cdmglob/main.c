#include <stdio.h>
#include "cdmglob.h"

extern char	*ego;
extern int	nerrors;
int	 v_flag;

int	kdm_flag,	/* shape shenanigans for kdm */
	flat_flag,	/* flatten hierarchy */
	lsl_flag,	/* generate LSL */
	mws_flag;	/* case crap for mws */


void
main (int argc, char **argv)
{
	int	i;

	ego = argv[0];

	for (i = 1; i < argc; i++)
		if (*argv[i] != '-')
			scan (argv[i]);
		else	switch (argv[i][1]) {
			case 0:
				scan ((char *) 0);
			case 'f':
				flat_flag = 1;
				break;
			case 'k':
				kdm_flag = 1;
				break;
			case 'm':
				mws_flag = 1;
				break;
			case 'L':
				lsl_flag = 1;
				break;
			case 'v':
				v_flag = 1;
				break;
			default:
				goto usage;
			}

	macheck();
	if (nerrors)
#ifdef PLAN9
		exits ("cdmglob errors");
#else
		exit (1);
#endif
	macprint();
#ifdef PLAN9
	exits ((char *) 0);
#else
	exit (0);
#endif

usage:
	fprintf (stderr, "usage: %s [-L] [-f] [-k] [-m] [-v] [files]\n", ego);
#ifdef PLAN9
	exits ("bad usage");
#else
	exit (1);
#endif
}
