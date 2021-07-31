/* COPYRIGHT (C) 1987 Kamal Al-Yahya */

/* tr2tex: troff to tex translator */
/* Author: Kamal Al-Yahya, Stanford University,		9/4/86 */
/* Last modified:					1/1/87 */
/* Keyword: convert translate tex troff */

char *documentation[] = {
" SYNTAX",
"        tr2tex [-m] file1 file2 ...",
"or",
"        tr2tex [-m] < file1 file2 ...",
"",
" Use the -m flag for manual",
"",
};

int	doclength = { sizeof documentation/sizeof documentation[0] };

#include        "setups.h"

FILE *out_file;

#if HAVE_SGTTY
struct sgttyb ttystat;
#endif

int man;
int xargc;
char **xargv;

int
main(int argc, char *argv[])
{

char *inbuf, *outbuf;

FILE *temp,*scr;
register char *cptr;
int piped_in;
int i;
long timeval;		/* clock value from time() for ctime()	*/
char *document = "article";			/* document type */
char *options = "[troffms,11pt]";		/* style options */

/* Allocate large arrays dynamically to conserve stack space */

if (((inbuf = (char *)malloc(MAXLEN*sizeof(char))) == (char *)NULL) ||
    ((outbuf = (char *)malloc(MAXLEN*sizeof(char))) == (char *)NULL))
	{
    	fprintf(stderr,"tr2tex: Cannot malloc() internal buffer space\nNeed two arrays of %d characters each\n",MAXLEN);
	exit(-1);
	}

/* If no arguments, and not in a pipeline, self document */
#if HAVE_SGTTY
piped_in = ioctl ((fileno (stdin)), TIOCGETP, &ttystat);
#else	/* if no sggty, it cannot distinguish piped input from no input */
piped_in = (argc == 1);
#endif

if (argc == 1 && !piped_in)
	{
	for( i=0; i<doclength; i++)
		printf("%s\n",documentation[i]);
	exit (0);
	}

/* initialize spacing and indentation parameters */
strcpy(linespacing.def_units,"\\normalbaselineskip");
strcpy(linespacing.old_units,"\\normalbaselineskip");
strcpy(indent.def_units,"em");		strcpy(indent.old_units,"em");
strcpy(tmpind.def_units,"em");		strcpy(tmpind.old_units,"em");
strcpy(space.def_units,"\\baselineskip");
strcpy(space.old_units,"\\baselineskip");
strcpy(vspace.def_units,"pt");		strcpy(vspace.old_units,"pt");
linespacing.value = 1.;			linespacing.old_value = 1.;
indent.value = 0.;			indent.old_value = 0.;
tmpind.value = 0.;			tmpind.old_value = 0.;
space.value = 1.;			space.old_value = 1.;
vspace.value = 1.;			vspace.old_value = 1.;
linespacing.def_value = 0;
indent.def_value = 0;
tmpind.def_value = 0;
space.def_value = 1;
vspace.def_value = 1;

out_file = stdout;		/* default output */
math_mode = 0;			/* start with non-math mode */
de_arg = 0;			/* not a .de argument */

/* process option flags */
xargc = argc;
xargv = argv;
for (xargc--,xargv++; xargc; xargc--,xargv++)
	{
	cptr = *xargv; 
	if( *cptr=='-' )
		{
		while( *(++cptr))
			{
			switch( *cptr )
				{
				case 'm':
					man = 1;
					strcpy(options,"[troffman]");
					break;
				default:
			     		fprintf(stderr,
						"tr2tex: unknown flag -%c\n",*cptr);
					break;
				}
			}
		}
	}
/* start of translated document */

timeval = time(0);
fprintf(out_file,"%% -*-LaTeX-*-\n%% Converted automatically from troff to LaTeX by tr2tex on %s",ctime(&timeval));
fprintf(out_file,"%% tr2tex was written by Kamal Al-Yahya at Stanford University\n%% (Kamal%%Hanauma@SU-SCORE.ARPA)\n\n\n");

/* document style and options */
fprintf(out_file,"\\documentstyle%s{%s}\n\\begin{document}\n",options,document);

/* first process pipe input */
if(piped_in)
	{
/* need to buffer; can't seek in pipes */
/* make a temporary and volatile file */
	scr = tmpfile();
	if (!scr)
		{
		fprintf(stderr,
	        "tr2tex: Cannot open scratch file\n");
		exit(-1);
		}
	scrbuf(stdin,scr);
	rewind(scr);
	tmpbuf(scr,inbuf);
	troff_tex(inbuf,outbuf,0,0);
	fprintf(out_file,"%%\n%% input file: stdin\n%%\n");
	fputs(outbuf,out_file);
	}

/* then process input line for arguments and assume they are input files */
xargc = argc;
xargv = argv;

for (xargc--,xargv++; xargc; xargc--,xargv++)
	{
	cptr = *xargv; 
	if( *cptr=='-' ) continue; /* this is a flag */
	if((temp=fopen(cptr,"r")) != (FILE *)NULL)
		{
		tmpbuf(temp,inbuf);
		fclose(temp);
		troff_tex(inbuf,outbuf,0,0);
		fprintf(out_file,"%%\n%% input file: %s\n%%\n",cptr);
		fputs(outbuf,out_file);
		}
	else
		fprintf(stderr,"tr2tex: Cannot open %s\n",cptr);
	}
/* close translated document */
fputs("\\end{document}\n",out_file);

exit(0);
}
