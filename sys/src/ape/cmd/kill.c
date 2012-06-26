#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <signal.h>
#include <ctype.h>
#include <stdio.h>
#include <string.h>

#define NSIG SIGUSR2

char *signm[NSIG+1] = { 0,
"SIGHUP", "SIGINT", "SIGQUIT", "SIGILL", "SIGABRT", "SIGFPE", "SIGKILL", /* 1-7 */
"SIGSEGV", "SIGPIPE", "SIGALRM", "SIGTERM", "SIGUR1", "SIGUSR2", /* 8-13 */
};

main(int argc, char **argv)
{
	int signo, pid, res;
	int errlev;

	errlev = 0;
	if (argc <= 1) {
	usage:
		fprintf(stderr, "usage: kill [ -sig ] pid ...\n");
		fprintf(stderr, "for a list of signals: kill -l\n");
		exit(2);
	}
	if (*argv[1] == '-') {
		if (argv[1][1] == 'l') {
			int i = 0;
			for (signo = 1; signo <= NSIG; signo++)
				if (signm[signo]) {
					printf("%s ", signm[signo]);
					if (++i%8 == 0)
						printf("\n");
				}
			if(i%8 !=0)
				printf("\n");
			exit(0);
		} else if (isdigit(argv[1][1])) {
			signo = atoi(argv[1]+1);
			if (signo < 0 || signo > NSIG) {
				fprintf(stderr, "kill: %s: number out of range\n",
				    argv[1]);
				exit(1);
			}
		} else {
			char *name = argv[1]+1;
			for (signo = 1; signo <= NSIG; signo++)
				if (signm[signo] && (
				    !strcmp(signm[signo], name)||
				    !strcmp(signm[signo]+3, name)))
					goto foundsig;
			fprintf(stderr, "kill: %s: unknown signal; kill -l lists signals\n", name);
			exit(1);
foundsig:
			;
		}
		argc--;
		argv++;
	} else
		signo = SIGTERM;
	argv++;
	while (argc > 1) {
		if ((**argv<'0' || **argv>'9') && **argv!='-')
			goto usage;
		res = kill(pid = atoi(*argv), signo);
		if (res<0) {
			perror("kill");
		}
		argc--;
		argv++;
	}
	return(errlev);
}
