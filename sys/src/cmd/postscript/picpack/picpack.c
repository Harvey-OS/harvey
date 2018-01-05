/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

/*
 *
 * picpack - picture packing pre-processor
 *
 * A trivial troff pre-processor that copies files to stdout, expanding picture
 * requests into an in-line format that's passed transparently through troff and
 * handled by dpost. The program is an attempt to address requirements, expressed
 * by several organizations, of being able to store a document as a single file
 * (usually troff input) that can then be sent through dpost and ultimately to
 * a PostScript printer.
 *
 * The program looks for strings listed in the keys[] array at the start of each
 * line. When a picture request (as listed in keys[]) is found the second string
 * on the line is taken to be a picture file pathname that's added (in transparent
 * mode) to the output file. In addition each in-line picture file is preceeded by
 * device control command (again passed through in transparent mode) that looks
 * like,
 *
 *	x X InlinePicture filename bytes
 *
 * where bytes is the size of the picture file (which begins on the next line)
 * and filename is the pathname of the picture file. dpost uses both arguments to
 * manage in-line pictures (in a big temp file). To handle pictures in diversions
 * picpack reads each input file twice. The first pass looks for picture inclusion
 * requests and copies each picture file transparently to the output file, while
 * second pass just copies the input file to the output file. Things could still
 * break, but the two pass method should handle most jobs.
 *
 * The recognized in-line picture requests are saved in keys[] and by default only
 * expand .BP and .PI macro calls. The -k option changes the recognized strings,
 * and may be needed if you've built your own picture inclusion macros on top of
 * .BP or .PI or decided to list each picture file at the start of your input file
 * using a dummy macro. For example you could require every in-line picture be
 * named by a dummy macro (say .iP), then the command line,
 *
 *	picpack -k.iP file > file.pack
 *
 * hits on lines that begin with .iP (rather than .BP or .PI), and the only files
 * pulled in would be ones named as the second argument to the new .iP macro. The
 * -k option accepts a space or comma separated list of up to 10 different key
 * strings. picpack imposes no contraints on key strings, other than not allowing
 * spaces or commas. A key string can begin with \" and in that case it would be
 * troff comment.
 *
 * Although the program will help some users, there are obvious disadvantages.
 * Perhaps the most important is that troff output files (with in-line pictures
 * included) don't fit the device independent language accepted by important post
 * processors like proof, and that means you won't be able to reliably preview a
 * packed file on your 5620 or whatever. Another potential problem is that picture
 * files can be large. Packing everything together in a single file at an early
 * stage has a better chance of exceeding your system's ulimit.
 *
 */

#include	<stdio.h>
#include	<sys/types.h>
#include	<sys/stat.h>
#include	<string.h>

#include	"gen.h"			/* general purpose definitions */
#include	"ext.h"			/* external variable definitions */
#include	"path.h"		/* just for TEMPDIR definition */

char		*keys[11] = {".BP", ".PI", NULL};
int		quiet = FALSE;

FILE		*fp_in = stdin;		/* input */
FILE		*fp_out = stdout;	/* and output files */

/*****************************************************************************/

main(agc, agv)

    int		agc;
    char	*agv[];

{

/*
 *
 * A picture packing pre-processor that copies input files to stdout, expanding
 * picture requests (as listed in keys[]) to an in-line format that can be passed
 * through troff (using transparent mode) and handled later by dpost.
 *
 */

    argc = agc;				/* global so everyone can use them */
    argv = agv;

    prog_name = argv[0];		/* just for error messages */

    options();				/* command line options */
    arguments();			/* translate all the input files */
    done();				/* clean things up */

    exit(x_stat);			/* everything probably went OK */

}   /* End of main */

/*****************************************************************************/

options()

{

    int		ch;			/* name returned by getopt() */

    extern char	*optarg;		/* option argument set by getopt() */
    extern int	optind;

/*
 *
 * Handles the command line options.
 *
 */

    while ( (ch = getopt(argc, argv, "k:qDI")) != EOF )  {
	switch ( ch )  {
	    case 'k':			/* new expansion key strings */
		    newkeys(optarg);
		    break;

	    case 'q':			/* disables "missing picture" messages */
		    quiet = TRUE;
		    break;

	    case 'D':			/* debug flag */
		    debug = ON;
		    break;

	    case 'I':			/* ignore FATAL errors */
		    ignore = ON;
		    break;

	    case '?':			/* don't know the option */
		    error(FATAL, "");
		    break;

	    default:
		    error(FATAL, "missing case for option %c", ch);
		    break;
	}   /* End switch */
    }	/* End while */

    argc -= optind;			/* get ready for non-options args */
    argv += optind;

}   /* End of options */

/*****************************************************************************/

newkeys(list)

    char	*list;			/* comma or space separated key strings */

{

    char	*p;			/* next key string from *list */
    int		i;			/* goes in keys[i] */
    int		n;			/* last key string slot in keys[] */

/*
 *
 * Separates *list into space or comma separated strings and adds each to the
 * keys[] array. The strings in keys[] are used to locate the picture inclusion
 * requests that are translated to the in-line format. The keys array must end
 * with a NULL pointer and by default only expands .BP and .PI macro calls.
 *
 */

    n = (sizeof(keys) / sizeof(char *)) - 1;

    for ( i = 0, p = strtok(list, " ,"); p != NULL; i++, p = strtok(NULL, " ,") )
	if ( i >= n )
	    error(FATAL, "too many key strings");
	else keys[i] = p;

    keys[i] = NULL;

}   /* End of newkeys */

/*****************************************************************************/

arguments()

{

    FILE	*copystdin();

/*
 *
 * Makes sure all the non-option command line arguments are processed. If we get
 * here and there aren't any arguments left, or if '-' is one of the input files
 * we process stdin, after copying it to a temporary file.
 *
 */

    if ( argc < 1 )  {
	fp_in = copystdin();
	picpack();
    } else
	while ( argc > 0 ) {
	    if ( strcmp(*argv, "-") == 0 )
		fp_in = copystdin();
	    else if ( (fp_in = fopen(*argv, "r")) == NULL )
		error(FATAL, "can't open %s", *argv);
	    picpack();
	    fclose(fp_in);
	    argc--;
	    argv++;
	}   /* End while */

}   /* End of arguments */

/*****************************************************************************/

FILE *copystdin()

{

    char	*tfile;			/* temporary file name */
    int		fd_out;			/* and its file descriptor */
    FILE	*fp;			/* return value - the new input file */

/*
 *
 * Copies stdin to a temp file, unlinks the file, and returns the file pointer
 * for the new temporary file to the caller. Needed because we read each input
 * file twice in an attempt to handle pictures in diversions.
 *
 */

    if ( (tfile = tempnam(TEMPDIR, "post")) == NULL )
	error(FATAL, "can't generate temp file name");

    if ( (fd_out = creat(tfile, 0660)) == -1 )
	error(FATAL, "can't create %s", tfile);

    copyfile(fileno(stdin), fd_out);
    close(fd_out);

    if ( (fp = fopen(tfile, "r")) == NULL )
	error(FATAL, "can't open %s", tfile);

    unlink(tfile);
    return(fp);

}   /* End of copystdin */

/*****************************************************************************/

copyfile(fd_in, fd_out)

    int		fd_in;			/* input */
    int		fd_out;			/* and output files */

{

    char	buf[512];		/* internal buffer for reads and writes */
    int		count;			/* number of bytes put in buf[] */

/*
 *
 * Copies file fd_in to fd_out. Handles the second pass for each input file and
 * also used to copy stdin to a temporary file.
 *
 */

    while ( (count = read(fd_in, buf, sizeof(buf))) > 0 )
	if ( write(fd_out, buf, count) != count )
	    error(FATAL, "write error");

}   /* End of copyfile */

/*****************************************************************************/

done()

{

/*
 *
 * Finished with all the input files - unlink the temporary file that was used
 * to record the in-line picture file pathnames.
 *
 */

    if ( temp_file != NULL )
	unlink(temp_file);

}   /* End of done */

/*****************************************************************************/

picpack()

{

    char	line[512];		/* next input line */
    char	name[100];		/* picture file names - from BP or PI */
    int		i;			/* for looking through keys[] */

/*
 *
 * Handles the two passes over the next input file. First pass compares the start
 * of each line in *fp_in with the key strings saved in the keys[] array. If a
 * match is found inline() is called to copy the picture file (ie. the file named
 * as the second string in line[]) to stdout, provided the file hasn't previously
 * been copied. The second pass goes back to the start of fp_in and copies it all
 * to the output file.
 *
 */

    while ( fgets(line, sizeof(line), fp_in) != NULL )  {
	for ( i = 0; keys[i] != NULL; i++ )
	    if ( strncmp(line, keys[i], strlen(keys[i])) == 0 )  {
		if ( sscanf(line, "%*s %s", name) == 1 )  {
		    strtok(name, "(");
		    if ( gotpicfile(name) == FALSE )
			inline(name);
		}   /* End if */
	    }   /* End if */
    }	/* End while */

    fflush(fp_out);			/* second pass - copy fp_in to fp_out */
    fseek(fp_in, 0L, 0);
    copyfile(fileno(fp_in), fileno(fp_out));

}   /* End of picpack */

/*****************************************************************************/

inline(name)

    char	*name;			/* name of the in-line picture file */

{

    long	size;			/* size in bytes - from fstat */
    FILE	*fp;			/* for reading *name */
    int		ch;			/* next character from picture file */
    int		lastch = '\n';		/* so we know when to put out \! */

    struct stat	sbuf;			/* for the picture file size */

/*
 *
 * Copies the picture file *name to the output file in an in-line format that can
 * be passed through troff and recovered later by dpost. Transparent mode is used
 * so each line starts with \! and all \ characters must be escaped. The in-line
 * picture sequence begins with an "x X InlinePicture" device control command that
 * names the picture file and gives its size (in bytes).
 *
 */

    if ( (fp = fopen(name, "r")) != NULL )  {
	fstat(fileno(fp), &sbuf);
	if ( (size = sbuf.st_size) > 0 )  {
	    fprintf(fp_out, "\\!x X InlinePicture %s %ld\n", name, size);
	    while ( (ch = getc(fp)) != EOF )  {
		if ( lastch == '\n' )
		    fprintf(fp_out, "\\!");
		if ( ch == '\\' )
		    putc('\\', fp_out);
		putc(lastch = ch, fp_out);
	    }   /* End while */
	    if ( lastch != '\n' )
		putc('\n', fp_out);
	}    /* End if */
	fclose(fp);
	addpicfile(name);
    } else if ( quiet == FALSE )
	error(NON_FATAL, "can't read picture file %s", name);

}   /* End of inline */

/*****************************************************************************/

gotpicfile(name)

    char	*name;

{

    char	buf[100];
    FILE	*fp_pic;

/*
 *
 * Checks the list of previously added picture files in *temp_file and returns
 * FALSE if it's a new file and TRUE otherwise. Probably should open the temp
 * file once for update and leave it open, rather than opening and closing it
 * every time.
 *
 */

    if ( temp_file != NULL )
	if ( (fp_pic = fopen(temp_file, "r")) != NULL )  {
	    while ( fscanf(fp_pic, "%s", buf) != EOF )
		if ( strcmp(buf, name) == 0 )  {
		    fclose(fp_pic);
		    return(TRUE);
		}   /* End if */
	    fclose(fp_pic);
	}   /* End if */

    return(FALSE);

}   /* End of gotpicfile */

/*****************************************************************************/

addpicfile(name)

    char	*name;

{

    FILE	*fp_pic;

/*
 *
 * Adds string *name to the list of in-line picture files that's maintained in
 * *temp_file. Should undoubtedly open the file once for update and use fseek()
 * to move around in the file!
 *
 */

    if ( temp_file == NULL )
	if ( (temp_file = tempnam(TEMPDIR, "picpac")) == NULL )
	    return;

    if ( (fp_pic = fopen(temp_file, "a")) != NULL )  {
	fprintf(fp_pic, "%s\n", name);
	fclose(fp_pic);
    }	/* End if */

}   /* End of addpicfile */

/*****************************************************************************/
