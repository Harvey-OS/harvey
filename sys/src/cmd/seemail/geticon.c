#include <u.h>
#include <libc.h>
#include <bio.h>
#include <regexp.h>
#include <ctype.h>

char	machineused[64];
char	userused[64];
int	depthused;

/*
 *  translate mail address into an open file
 */
char*
tryfile(char *machine, char *user, int ld)
{
	static char file[128];
	char *cp, *p;
	Biobuf *mf;

	sprint(file, "/lib/face/48x48x%d/.dict", 1<<ld);
	mf = Bopen(file, OREAD);
	if(mf == 0)
		return 0;

	sprint(file, "%s/%s", machine, user);
	while(cp = Brdline(mf, '\n')) {
		cp[Blinelen(mf)-1] = 0;
		p = strchr(cp, ' ');
		if(p == 0)
			continue;
		*p = 0;
		if(strcmp(cp, file) == 0) {
			sprint(file, "/lib/face/48x48x%d/%s", 1<<ld, p+1);
			strncpy(machineused, machine, sizeof(machineused));
			strncpy(userused, user, sizeof(userused));
			depthused = ld;
			Bterm(mf);
			return file;
		}
	}
	Bterm(mf);
	return 0;
}

char*
tryfiles(char *machine, char *user, int ld)
{
	char *cp;

	cp = tryfile(machine, user, ld);
	if (cp != 0)
		return cp;
	if (ld == 1)
		cp = tryfile(machine, user, 0);
	return cp;
}

char*
getfile(char *machine, char *user, int ld)
{
	char *fp;
	Biobuf *mf;
	char *cp, *dp, *ep;
	Reprog *exp;
	char buf[200];

	if(*user == 0)
		user = "unknown";
	if(*machine == 0)
		machine = "astro";	/* this is hopelessly provincial */

	fp = tryfiles(machine, user, ld);
	if (fp != 0)
		return fp;

	cp = strchr(machine, '.');
	ep = strrchr(machine, '.');
	while (cp != 0 && cp < ep) {	/* try higher domain */
		if ((fp = tryfiles(++cp, user, ld)) != 0)
			return fp;
		cp = strchr(cp, '.');
	}

	mf = Bopen("/lib/face/.machinelist", OREAD);
	if (mf == 0)
		return 0;

	while (cp = Brdline(mf, '\n')) {
		int clean = 1;
		if (*cp == '#' || (ep=strpbrk(cp, " 	")) == 0)
			continue;
		cp[Blinelen(mf)-1] = 0;
		for (dp=&buf[1]; cp < ep; ) {
			if (!isalnum(*cp))
				clean = 0;
			*dp++ = *cp++;
		}
		*dp = '\0';
		ep += strspn(ep, "	 ");
		/* skip expression processing for a simple compare */
		if (clean && (strcmp(&buf[1], machine) == 0))
			return tryfiles(ep, user, ld);

		if (buf[1] != '^') {
			buf[0] = '^';
			cp = buf;
		} else
			cp = buf+1;
		if (*(dp-1) != '$')
			*dp++ = '$';
		*dp = '\0';

		exp = regcomp(cp);
		if (!exp) {
			fprint(2, "Error in regular expression: %s\n",
				cp);
			exits("regexp");
		}
		if (regexec(exp, machine, 0, 0) != 0) {
			free(exp);
			return tryfiles(ep, user, ld);
		}
		free(exp);
	}
	Bterm(mf);
	return 0;
}

void
makelower(char *cp)
{
	for (; *cp; cp++)
		if (isupper(*cp))
			*cp = tolower(*cp);
}

void
main(int argc, char *argv[])
{
	char *machine, *user, *fd;
	int depth;

	if (argc != 4) {
		fprint(2, "usage: geticon machine user depth\n");
		exits("usage");
	}

	machine = argv[1];
	user = argv[2];
	depth = atoi(argv[3]);

	makelower(machine);
	makelower(user);
	if (depth > 1)
		depth = 1;

	fd = getfile(machine, user, depth);
	if (fd == 0)
		fd = getfile(machine, "unknown", depth);
	if (fd == 0)
		fd = getfile("", "unknown", depth);

	if (fd == 0)
		exits("not found");

	print("%s %s %d %s\n", machineused, userused, depthused, fd);
	exits("");
}
