/* bbfig.c --
 * Martin Gelbaum, Lawrence Berkeley Laboratory
 * martyg@lbl.gov
 * September 21, 1992
 * Simple VAX C equivalent of Bourne shell script "bbfig"
 */

#include <stdio.h>
#include <ssdef.h>	/* System status codes */
#include <unixlib>
  
#define BBFIG_HEADER "tex_disk:[tex.dvips.header_files]bb.ps"

main(argc,argv)
int argc; 	
char **argv;   
{
        FILE *infp, *libfp, *outfp;
	char outfile[1024], buf[BUFSIZ], line[512], *cptr;

	if (argc < 2) { error("Usage: bbfig psfile [> outfile]\n"); }
	if ( !strcmp(argv[1], "-help") || !strcmp(argv[1], "-HELP") )
	{
		fprintf(stderr, "bbfig: usage: bbfig psfile [> outfile]\n");
		fflush(stderr); fprintf(stderr, 
"If no output file specified, output file is bbfig_out.ps\n");
		fflush(stderr); fprintf(stderr, 
"Output file shows figure surrounded by box made of dashes\n");
		fflush(stderr); fprintf(stderr, 
"Output file also shows bounding box in default coordinates for use as\n");
		fflush(stderr); fprintf(stderr, 
"Bounding Box comment as second line of PostScript file.\n");
		exit(SS$_NORMAL);
	}
        if ( (libfp = fopen(BBFIG_HEADER,"r")) == NULL)
        {
		error("ABNORMAL EXIT:\nCan't open PostScript macro file %s\n", 
					BBFIG_HEADER);
        }
        if ( (infp = fopen(*++argv,"r")) == NULL)
        {
		error("ABNORMAL EXIT:\nCan't open user PostScript file %s\n", 
			*argv);
        }
	if (argc >=3)
	{
		argv++;
		if ( (*argv)[0] ==  '>')
		{
			if ((*argv)[1])
			/* We had bbfig file  >outfile */
			{
				cptr = &(*argv)[1];
				(void) sprintf(outfile,"%s", cptr);
			}
			else if (*(++argv) != NULL) 
			/* We had bbfig psfile > outfile */
         		{ 
				cptr = *argv;
				(void) sprintf(outfile,"%s", cptr);
			}
			else
			{
				error("%s: no filename with '>' option\n",
                                        	"ABNORMAL EXIT");
			}
		}
		else
		{
			error("ABNORMAL EXIT: unknown option %c\n",
					(*argv)[0]);
		}
	}
	else
	{
		/* Default filename is "bbfig_out.ps" */
		sprintf(outfile, "bbfig_out.ps");
	}
	/* Make a standard variable length record file
	* with carriage return carriage control. 
	*/
        if ( (outfp = fopen(outfile,"w", "rat=cr", "rfm=var")) == NULL)
        {
		error("ABNORMAL EXIT: can't create text file %s\n", 
			outfile);
        }
	while (fgets(line, 511, libfp))
	{
                        fputs(line, outfp); fflush(outfp);
        }
        (void) fclose (libfp);
	while (fgets(line, 511, infp))
	{
                        fputs(line, outfp); fflush(outfp);
        }
        (void) fclose (infp); (void) fclose (outfp);
	fprintf(stderr, "Output file showing bounding box is %s;\n", outfile);
	fflush(stderr);
	fprintf(stderr, 
"You may print it by lpr to a PostScript printer or\n"); fflush(stderr);
	fprintf(stderr, 
"preview it by gs (ghostscript) on a X11 display ...\n"); fflush(stderr);
	exit(SS$_NORMAL);
}

static error(s1,s2)   /* print error message and exit with error status. */
char *s1, *s2;
{
	/* Notice that the first argument (s1) contains the formatting
	 * information for fprintf. 
	 */
	fprintf(stderr, "bbfig: "); fprintf(stderr, s1, s2); exit(0x10000000);
}
