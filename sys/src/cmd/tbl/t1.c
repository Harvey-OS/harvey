/* t1.c: main control and input switching */
#
# include "t.h"

# define MACROS "/usr/lib/tmac.s"
# define PYMACS "/usr/lib/tmac.m"


# define ever (;;)

void
main(int argc, char *argv[])
{
	exits(tbl(argc, argv)? "error" : 0);
}


int
tbl(int argc, char *argv[])
{
	char	line[5120];
	/*int x;*/
	/*x=malloc((char *)0);	uncomment when allocation breaks*/
	tabin = stdin; 
	tabout = stdout;
	setinp(argc, argv);
	while (gets1(line, sizeof(line))) {
		fprintf(tabout, "%s\n", line);
		if (prefix(".TS", line))
			tableput();
	}
	fclose(tabin);
	return(0);
}


int	sargc;
char	**sargv;

void
setinp(int argc, char **argv)
{
	sargc = argc;
	sargv = argv;
	sargc--; 
	sargv++;
	if (sargc > 0)
		swapin();
}


int
swapin(void)
{
	char	*name;
	while (sargc > 0 && **sargv == '-') {
		if (match("-ms", *sargv)) {
			*sargv = MACROS;
			break;
		}
		if (match("-mm", *sargv)) {
			*sargv = PYMACS;
			break;
		}
		if (match("-TX", *sargv))
			pr1403 = 1;
		if (match("-", *sargv))
			break;
		sargc--; 
		sargv++;
	}
	if (sargc <= 0) 
		return(0);
	/* file closing is done by GCOS troff preprocessor */
	if (tabin != stdin) 
		fclose(tabin);
	if (match(*sargv, "-")) {
		tabin = stdin;
		name = "-";
	} else {
		tabin = fopen(ifile = *sargv, "r");
		name = ifile;
	}
	iline = 1;
	fprintf(tabout, ".ds f. %s\n", ifile);
	fprintf(tabout, ".lf %d %s\n", iline, name);
	if (tabin == NULL)
		error("Can't open file");
	sargc--;
	sargv++;
	return(1);
}


