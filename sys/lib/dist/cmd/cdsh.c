/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

/*
 * The `cd' shell.  
 * Just has cd and lc.
 */

#include <u.h>
#include <libc.h>
#include <bio.h>

char *pwd;
char *root = "/";

void
usage(void)
{
	fprint(2, "usage: cdsh [-r root]\n");
	exits("usage");
}

int
system(char *cmd)
{
	int pid;
	if((pid = fork()) < 0)
		return -1;

	if(pid == 0) {
		dup(2, 1);
		execl("/bin/rc", "rc", "-c", cmd, nil);
		exits("exec");
	}
	waitpid();
	return 0;
}

int
cd(char *s)
{
	char *newpwd;
	int l;

	if(s[0] == '/') {
		cleanname(s);
		newpwd = strdup(s);
	} else {
		l = strlen(pwd)+1+strlen(s)+1+50;	/* 50 = crud for unicode mistakes */
		newpwd = malloc(l);
		snprint(newpwd, l, "%s/%s", pwd, s);
		cleanname(newpwd);
		assert(newpwd[0] == '/');
	}

	if(chdir(root) < 0 || (newpwd[1] != '\0' && chdir(newpwd+1) < 0)) {
		chdir(root);
		chdir(pwd+1);
		free(newpwd);
		return -1;
	} else {
		free(pwd);
		pwd = newpwd;
		return 0;
	}
}

void
main(int argc, char **argv)
{
	char *p;
	Biobuf bin;
	char *f[2];
	int nf;

	ARGBEGIN{
	case 'r':
		root = ARGF();
		if(root == nil)
			usage();
		if(root[0] != '/') {
			fprint(2, "root must be rooted\n");
			exits("root");
		}
		break;
	default:
		usage();
	}ARGEND;

	if(argc != 0)
		usage();

	cleanname(root);
	if(cd("/") < 0) {
		fprint(2, "cannot cd %s: %r\n", root);
		exits("root");
	}

	Binit(&bin, 0, OREAD);
	while(fprint(2, "%s%% ", pwd), (p = Brdline(&bin, '\n'))) {
		p[Blinelen(&bin)-1] = '\0';
		nf = tokenize(p, f, nelem(f));
		if(nf < 1)
			continue;
		if(strcmp(f[0], "exit") == 0)
			break;
		if(strcmp(f[0], "lc") == 0) {
			if(nf == 1) {
				if(system("/bin/lc") < 0)
					fprint(2, "lc: %r\n");
			} else if(nf == 2) {
				if(strpbrk(p, "'`{}^@$#&()|\\;><"))
					fprint(2, "no shell characters allowed\n");
				else {
					p = f[1];
					*--p = ' ';
					*--p = 'c';
					*--p = 'l';
					if(system(p) < 0)
						fprint(2, "lc: %r\n");
				}
			}
			continue;
		}
		if(strcmp(f[0], "cd") == 0) {
			if(nf < 2)
				fprint(2, "usage: cd dir\n");
			else if(cd(f[1]) < 0)
				fprint(2, "cd: %r\n");
			continue;
		}
		fprint(2, "commands are cd, lc, and exit\n");
	}

	print("%s\n", pwd);
}
