/*
 *
 * postbgi - BGI (Basic Graphical Instructions) to PostScript translator.
 *
 * A simple program that translates BGI files into PostScript. Probably only
 * useful in Computer Centers that support STARE or PRISM plotters. Most of the
 * code was borrowed from the corresponding program that was written for printers
 * that understand Impress.
 *
 * Extending the original program to handle PRISM jobs was not trivial. Graphics
 * packages that support PRISM occasionally use BGI commands that I ignored in the
 * STARE implementation. Subroutines, color requests, patterns (for filling), and
 * filled trapeziods were the most important omissions. All are now implemented,
 * and at present only repeats, filled slices, and raster rectangles are missing.
 *
 * Pattern filling results were not always predictable or even good, unless the
 * halftone screen definitions were changed and scaling was adjusted so one pixel
 * in user space mapped into an integral number of device space pixels. Doing that
 * makes the resulting PostScript output device dependent, but was often necessary.
 * I've added two booleans to the PostScript prologue (fixscreen and scaletodevice)
 * that control what's done. By default both are false (check postbgi.ps) but can
 * be set to true on the command line using the -P option or by hand by changing
 * the definitions in the prologue. A command line that would set fixscreen and
 * scaletodevice true would look like,
 *
 *	postbgi -P"/fixscreen true" -P"/scaletodevice true" file >file.ps
 *
 * Several other approaches are available if you want to have your spooler handle
 * STARE and PRISM jobs differently. A boolean called prism is defined in the
 * prologue (postbgi.ps) and if it's set to true PostScript procedure setup will
 * set fixscreen and scaletodevice to true before anything important is done. That
 * means the following command line,
 *
 *	postbgi -P"/prism true" file >file.ps
 *
 * accomplishes the same things as the last example. Two different prologue files,
 * one for STARE jobs and the other for PRISM, could be used and the spooler could
 * point postbgi to the appropriate one using the -L option. In that case the only
 * important difference in the two prologues would be the definition of prism. The
 * prologue used for PRISM jobs would have prism set to true, while the STARE
 * prologue would have it set to false.
 *
 * Also included is code that ties lines to device space coordinates. What you get
 * is a consistent line thickness, but placement of lines won't be exact. It's a
 * trade-off that should be right for most jobs. Everything is implemented in the
 * prologue (postbgi.ps) and nothing will be done if the linewidth is zero or if
 * the boolean fixlinewidth (again in postbgi.ps) is false. Once again the -P
 * option can be used to set fixlinewidth to whatever you choose.
 *
 * BGI supports color mixing but PostScript doesn't. BGI files that expect to mix
 * colors won't print properly. PostScript's fill operator overlays whatever has
 * already been put down. Implementing color mixing would have been a terribly
 * difficult job - not worth the effort!
 *
 * The PostScript prologue is copied from *prologue before any of the input files
 * are translated. The program expects that the following PostScript procedures
 * are defined in that file:
 *
 *	setup
 *
 *	  mark ... setup -
 *
 *	    Handles special initialization stuff that depends on how the program
 *	    was called. Expects to find a mark followed by key/value pairs on the
 *	    stack. The def operator is applied to each pair up to the mark, then
 *	    the default state is set up.
 *
 *	pagesetup
 *
 *	  page pagesetup -
 *
 *	    Does whatever is needed to set things up for the next page. Expects
 *	    to find the current page number on the stack.
 *
 *	v
 *
 *	  dx1 dy1 ... dxn dyn x y v -
 *
 *	    Draws the vector described by the numbers on the stack. The top two
 *	    numbers are the coordinates of the starting point. The rest of the
 *	    numbers are relative displacements from the preceeding point.
 *
 *	pp
 *
 *	  x1 y1 ... xn yn string pp -
 *
 *	    Prints string, which is always a single character, at the points
 *	    represented by the rest of the numbers on the stack.
 *
 *	R
 *
 *	  n deltax deltay x y R -
 *
 *	    Creates a rectangular path with its lower left corner at (x, y) and
 *	    sides of length deltax and deltay. The resulting path is stroked if
 *	    n is 0 and filled otherwise.
 *
 *	T
 *
 *	  dx3 dy3 dx2 dy2 dx1 dy1 x y T -
 *
 *	    Fills a trapezoid starting at (x, y) and having relative displacements
 *	    given by the (dx, dy) pairs.
 *
 *	t
 *
 *	  angle x y string t -
 *
 *	    Prints string starting at (x, y) using an orientation of angle degrees.
 *	    The PostScript procedure can handle any angle, but BGI files will only
 *	    request 0 or 90 degrees. Text printed at any other orientation will be
 *	    vector generated.
 *
 *	p
 *
 *	  x y p -
 *
 *	    Called to mark the point (x, y). It fills a small circle, that right
 *	    now has a constant radius. This stuff could probably be much more
 *	    efficient?
 *
 *	l
 *
 *	  array l -
 *
 *	    Sets the line drawing mode according to the description given in
 *	    array. The arrays that describe the different line styles are declared
 *	    in STYLES (file posttek.h), although it would be better to have them
 *	    defined in the prologue.
 *
 *	c
 *
 *	  red green blue c -
 *
 *	    Sets the current PostScript RGB color using setrgbcolor. Also used for
 *	    selecting appropriate patterns as colors.
 *
 *	f
 *
 *	  bgisize f -
 *
 *	    Changes the size of the font that's used to print text. bgisize is a
 *	    grid separation in a 5 by 7 array in which characters are assumed to
 *	    be built.
 *
 *	done
 *
 *	  done
 *
 *	    Makes sure the last page is printed. Only needed when we're printing
 *	    more than one page on each sheet of paper.
 *
 * The default line width is zero, which forces lines to be one pixel wide. That
 * works well for 'write to black' engines but won't be right for 'write to white'
 * engines. The line width can be changed using the -w option, or you can change
 * the initialization of linewidth in the prologue. Code in the prologue supports
 * the generation of uniform width lines when linewidth is non-zero and boolean
 * fixlinewidth is true.
 *
 * Many default values, like the magnification and orientation, are defined in 
 * the prologue, which is where they belong. If they're changed (by options), an
 * appropriate definition is made after the prologue is added to the output file.
 * The -P option passes arbitrary PostScript through to the output file. Among
 * other things it can be used to set (or change) values that can't be accessed by
 * other options.
 *
 */

#include <stdio.h>
#include <sys/types.h>
#include <fcntl.h>
#include <signal.h>
#include <math.h>
#include <ctype.h>
#ifdef plan9
#define	isascii(c)	((unsigned char)(c)<=0177)
#endif

#include "comments.h"			/* PostScript file structuring comments */
#include "gen.h"			/* general purpose definitions */
#include "path.h"			/* for the prologue */
#include "ext.h"			/* external variable declarations */
#include "postbgi.h"			/* a few definitions just used here */

char	*optnames = "a:c:f:m:n:o:p:w:x:y:A:C:E:J:L:P:R:DI";

char	*prologue = POSTBGI;		/* default PostScript prologue */
char	*formfile = FORMFILE;		/* stuff for multiple pages per sheet */

int	formsperpage = 1;		/* page images on each piece of paper */
int	copies = 1;			/* and this many copies of each sheet */

char	*styles[] = STYLES;		/* descriptions of line styles */

int	hpos = 0;			/* current horizontal */
int	vpos = 0;			/* and vertical position */

int	bgisize = BGISIZE;		/* just the character grid spacing */
int	linespace;			/* distance between lines of text */

int	bgimode;			/* character or graph mode */

int	in_subr = FALSE;		/* currently defining a subroutine */
int	in_global = FALSE;		/* to save space with subroutine defs */
int	subr_id = 0;			/* defining this subroutine */
int	shpos = 0;			/* starting horizontal */
int	svpos = 0;			/* and vertical positions - subroutines */
Disp	displacement[64];		/* dx and dy after a subroutine call */

Fontmap	fontmap[] = FONTMAP;		/* for translating font names */
char	*fontname = "Courier";		/* use this PostScript font */

int	page = 0;			/* page we're working on */
int	printed = 0;			/* printed this many pages */

FILE	*fp_in = stdin;			/* read from this file */
FILE	*fp_out = NULL;			/* and write stuff here */
FILE	*fp_acct = NULL;		/* for accounting data */

/*****************************************************************************/

main(agc, agv)

    int		agc;
    char	*agv[];

{

/*
 *
 * A program that converts BGI (Basic Graphical Instructions) files generated by
 * packages like GRAFPAC and DISSPLA into PostScript. It does an adequate job but
 * is far from perfect. A few things still haven't been implemented (eg. repeats
 * and raster rectangles), but what's here should be good enough for most of our
 * STARE and PRISM jobs. Color mixing (in PRISM jobs) won't work on PostScript
 * printers, and there's no chance I'll implement it!
 *
 */

    argc = agc;				/* global so everyone can use them */
    argv = agv;

    prog_name = argv[0];		/* just for error messages */

    init_signals();			/* set up interrupt handling */
    header();				/* PostScript header comments */
    options();				/* command line options */
    setup();				/* for PostScript */
    arguments();			/* followed by each input file */
    done();				/* print the last page etc. */
    account();				/* job accounting data */

    exit(x_stat);			/* everything probably went OK */

}   /* End of main */

/*****************************************************************************/

init_signals()

{

/*
 *
 * Make sure we handle interrupts.
 *
 */

    if ( signal(SIGINT, interrupt) == SIG_IGN )  {
	signal(SIGINT, SIG_IGN);
	signal(SIGQUIT, SIG_IGN);
	signal(SIGHUP, SIG_IGN);
    } else {
	signal(SIGHUP, interrupt);
	signal(SIGQUIT, interrupt);
    }   /* End else */

    signal(SIGTERM, interrupt);

}   /* End of init_signals */

/*****************************************************************************/

header()

{

    int		ch;			/* return value from getopt() */
    int		old_optind = optind;	/* for restoring optind - should be 1 */

/*
 *
 * Scans the option list looking for things, like the prologue file, that we need
 * right away but could be changed from the default. Doing things this way is an
 * attempt to conform to Adobe's latest file structuring conventions. In particular
 * they now say there should be nothing executed in the prologue, and they have
 * added two new comments that delimit global initialization calls. Once we know
 * where things really are we write out the job header, follow it by the prologue,
 * and then add the ENDPROLOG and BEGINSETUP comments.
 *
 */

    while ( (ch = getopt(argc, argv, optnames)) != EOF )
	if ( ch == 'L' )
	    prologue = optarg;
	else if ( ch == '?' )
	    error(FATAL, "");

    optind = old_optind;		/* get ready for option scanning */

    fprintf(stdout, "%s", CONFORMING);
    fprintf(stdout, "%s %s\n", VERSION, PROGRAMVERSION);
    fprintf(stdout, "%s %s\n", DOCUMENTFONTS, ATEND);
    fprintf(stdout, "%s %s\n", PAGES, ATEND);
    fprintf(stdout, "%s", ENDCOMMENTS);

    if ( cat(prologue) == FALSE )
	error(FATAL, "can't read %s", prologue);

    fprintf(stdout, "%s", ENDPROLOG);
    fprintf(stdout, "%s", BEGINSETUP);
    fprintf(stdout, "mark\n");

}   /* End of header */

/*****************************************************************************/

options()

{

    int		ch;			/* option name - from getopt() */

/*
 *
 * Reads and processes the command line options.
 *
 */

    while ( (ch = getopt(argc, argv, optnames)) != EOF )  {
	switch ( ch )  {
	    case 'a':			/* aspect ratio */
		    fprintf(stdout, "/aspectratio %s def\n", optarg);
		    break;

	    case 'c':			/* copies */
		    copies = atoi(optarg);
		    fprintf(stdout, "/#copies %s def\n", optarg);
		    break;

	    case 'f':			/* new font */
		    fontname = get_font(optarg);
		    fprintf(stdout, "/font /%s def\n", fontname);
		    break;

	    case 'm':			/* magnification */
		    fprintf(stdout, "/magnification %s def\n", optarg);
		    break;

	    case 'n':			/* forms per page */
		    formsperpage = atoi(optarg);
		    fprintf(stdout, "%s %s\n", FORMSPERPAGE, optarg);
		    fprintf(stdout, "/formsperpage %s def\n", optarg);
		    break;

	    case 'o':			/* output page list */
		    out_list(optarg);
		    break;

	    case 'p':			/* landscape or portrait mode */
		    if ( *optarg == 'l' )
			fprintf(stdout, "/landscape true def\n");
		    else fprintf(stdout, "/landscape false def\n");
		    break;

	    case 'w':			/* line width */
		    fprintf(stdout, "/linewidth %s def\n", optarg);
		    break;

	    case 'x':			/* shift horizontally */
		    fprintf(stdout, "/xoffset %s def\n", optarg);
		    break;

	    case 'y':			/* and vertically on the page */
		    fprintf(stdout, "/yoffset %s def\n", optarg);
		    break;

	    case 'A':			/* force job accounting */
	    case 'J':
		    if ( (fp_acct = fopen(optarg, "a")) == NULL )
		    	error(FATAL, "can't open accounting file %s", optarg);
		    break;

	    case 'C':			/* copy file straight to output */
		    if ( cat(optarg) == FALSE )
			error(FATAL, "can't read %s", optarg);
		    break;

	    case 'E':			/* text font encoding */
		    fontencoding = optarg;
		    break;

	    case 'L':			/* Postscript prologue file */
		    prologue = optarg;
		    break;

	    case 'P':			/* PostScript pass through */
		    fprintf(stdout, "%s\n", optarg);
		    break;

	    case 'R':			/* special global or page level request */
		    saverequest(optarg);
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

	    default:			/* don't know what to do for ch */
		    error(FATAL, "missing case for option %c", ch);
		    break;
	}   /* End switch */
    }	/* End while */

    argc -= optind;			/* get ready for non-option args */
    argv += optind;

}   /* End of options */

/*****************************************************************************/

char *get_font(name)

    char	*name;			/* name the user asked for */

{

    int		i;			/* for looking through fontmap[] */

/*
 *
 * Called from options() to map a user's font name into a legal PostScript name.
 * If the lookup fails *name is returned to the caller. That should let you choose
 * any PostScript font.
 *
 */

    for ( i = 0; fontmap[i].name != NULL; i++ )
	if ( strcmp(name, fontmap[i].name) == 0 )
	    return(fontmap[i].val);

    return(name);

}   /* End of get_font */

/*****************************************************************************/

setup()

{

/*
 *
 * Handles things that must be done after the options are read but before the
 * input files are processed.
 *
 */

    writerequest(0, stdout);		/* global requests eg. manual feed */
    setencoding(fontencoding);
    fprintf(stdout, "setup\n");

    if ( formsperpage > 1 )  {
	if ( cat(formfile) == FALSE )
	    error(FATAL, "can't read %s", formfile);
	fprintf(stdout, "%d setupforms\n", formsperpage);
    }	/* End if */

    fprintf(stdout, "%s", ENDSETUP);

}   /* End of setup */

/*****************************************************************************/

arguments()

{

/*
 *
 * Makes sure all the non-option command line options are processed. If we get
 * here and there aren't any arguments left, or if '-' is one of the input files
 * we'll process stdin.
 *
 */

    if ( argc < 1 )
	conv();
    else
	while ( argc > 0 )  {
	    if ( strcmp(*argv, "-") == 0 )
		fp_in = stdin;
	    else if ( (fp_in = fopen(*argv, "r")) == NULL )
		error(FATAL, "can't open %s", *argv);
	    conv();
	    if ( fp_in != stdin )
		fclose(fp_in);
	    argc--;
	    argv++;
	}   /* End while */

}   /* End of arguments */

/*****************************************************************************/

done()

{

/*
 *
 * Finished with the last input file, so mark the end of the pages, make sure the
 * last page is printed, and restore the initial environment.
 *
 */

    fprintf(stdout, "%s", TRAILER);
    fprintf(stdout, "done\n");
    fprintf(stdout, "%s %s\n", DOCUMENTFONTS, fontname);
    fprintf(stdout, "%s %d\n", PAGES, printed);

}   /* End of done */

/*****************************************************************************/

account()

{

/*
 *
 * Writes an accounting record to *fp_acct, provided it's not NULL.
 *
 */

    if ( fp_acct != NULL )
	fprintf(fp_acct, " print %d\n copies %d\n", printed, copies);

}   /* End of account */

/*****************************************************************************/

conv()

{

    int		ch;			/* next input character */

/*
 *
 * Controls the conversion of BGI files into PostScript. Not everything has been
 * implemented, but what's been done should be good enough for our purposes.
 *
 */

    redirect(-1);			/* get ready for the first page */
    bgimode = 0;
    formfeed();

    while ( (ch = get_char()) != EOF )  {
	switch ( ch )  {
		case BRCHAR:			/* rotated character mode */
			    bgimode = ch;
			    text(90);
			    break;

		case BCHAR:			/* graphical character mode */
			    bgimode = ch;
			    text(0);
			    break;

		case BGRAPH:			/* graphical master mode */
			    bgimode = ch;
			    break;

		case BSUB:			/* subroutine definition */
			    subr_def();
			    break;

		case BRET:			/* end of subroutine */
			    subr_end();
			    break;

		case BCALL:			/* subroutine call */
			    subr_call();
			    break;

		case BEND:			/* end display - page */
			    formfeed();
			    break;

		case BERASE:			/* erase - shouldn't be used */
			    error(FATAL, "BGI erase opcode obsolete");
			    break;

		case BREP:			/* repeat */
			    error(FATAL, "Repeat not implemented");
			    repeat();
			    break;

		case BSETX:			/* new x coordinate */
			    hgoto(get_int(0));
			    break;

		case BSETY:			/* new y coordinate */
			    vgoto(get_int(0));
			    break;

		case BSETXY:			/* new x and y coordinates */
			    hgoto(get_int(0));
			    vgoto(get_int(0));
			    break;

		case BINTEN:			/* mark the current point */
			    fprintf(fp_out, "%d %d p\n", hpos, vpos);
			    break;

		case BVISX:			/* visible x */
			    vector(X_COORD, VISIBLE);
			    break;

		case BINVISX:			/* invisible x */
			    vector(X_COORD, INVISIBLE);
			    break;

		case BVISY:			/* visible y */
			    vector(Y_COORD, VISIBLE);
			    break;

		case BINVISY:			/* invisible y */
			    vector(Y_COORD, INVISIBLE);
			    break;

		case BVEC:			/* arbitrary vector */
			    vector(LONGVECTOR, VISIBLE);
			    break;

		case BSVEC:			/* short vector */
			    vector(SHORTVECTOR, VISIBLE);
			    break;

		case BRECT:			/* draw rectangle */
			    rectangle(OUTLINE);
			    break;

		case BPOINT1:			/* point plot 1 */
		case BPOINT:			/* point plot 2 */
			    point_plot(ch, get_char());
			    break;

		case BLINE:			/* line plot */
			    line_plot();
			    break;

		case BLTY:			/* line type */
			    fprintf(fp_out, "%s l\n", styles[get_data()]);
			    break;

		case BARC:			/* circular arc */
			    arc(OUTLINE);
			    break;

		case BFARC:			/* filled circle */
			    arc(FILL);
			    break;

		case BFRECT:			/* filled rectangle */
			    rectangle(FILL);
			    break;

		case BRASRECT:			/* raster rectangle */
			    error(FATAL, "Raster Rectangle not implemented");
			    break;

		case BCOL:			/* select color */
			    set_color(get_data());
			    break;

		case BFTRAPH:			/* filled trapezoid */
			    trapezoid();
			    break;

		case BPAT:			/* pattern for area filling */
			    pattern();
			    break;

		case BCSZ:			/* change BGI character 'size' */
			    setsize(get_data());
			    break;

		case BNOISE:			/* from bad file format */
			    break;

		default:			/* don't recognize the code */
			    error(FATAL, "bad BGI command %d (0%o)", ch, ch);
			    break;
	}   /* End switch */

	if ( debug == ON )
	    fprintf(stderr, "\n");
    }	/* End while */

    formfeed();					/* in case BEND was missing */

}   /* End of conv */

/*****************************************************************************/

hgoto(n)

    int		n;			/* new horizontal position */

{

/*
 *
 * Sets the current BGI horizontal position to n.
 *
 */

    hpos = n;

}   /* End of hgoto */

/*****************************************************************************/

vgoto(n)

    int		n;			/* move to this vertical position */

{

/*
 *
 * Sets the absolute vertical position to n.
 * 
 */

    vpos = n;

}   /* End of vgoto */

/*****************************************************************************/

setsize(n)

    int		n;			/* BGI size - just a grid separation */

{

/*
 *
 * Called when we're supposed to change the BGI character size to n. The BGI
 * size is the grid separation in a 5 by 7 array in which characters are assumed
 * to be built.
 *
 */

    bgisize = n;
    linespace = LINESPACE(bgisize);

    fprintf(fp_out, "%d f\n", bgisize);

    if ( debug == ON )
	fprintf(stderr, "BGI size = %d\n", n);

}   /* End of setsize */

/*****************************************************************************/

repeat()

{

    int		count;			/* repeat this many times */
    int		ch;			/* next input character */

/*
 *
 * Haven't implemented repeats, although it wouldn't be difficult. Apparently it's
 * not used by any graphics packages that generate BGI.
 *
 */

    count = get_int();			/* get the repeat count */

    while ( (ch = get_char()) != EOF  &&  ch != BENDR ) ;

}   /* End of repeat */

/*****************************************************************************/

text(angle)

    int		angle;			/* either 0 or 90 degrees */

{

    int		ch;			/* next character from file *fp_in */

/*
 *
 * Called from conv() after we've entered one of the graphical character modes.
 * Characters are read from the input file and printed until the next mode change
 * opcode is found (or until EOF). angle will be 90 for rotated character mode
 * and 0 otherwise.
 *
 *
 */

    fprintf(fp_out, "%d %d %d(", angle, hpos, vpos);

    while ( (ch = get_char()) != EOF )  {
	if ( ch == BGRAPH || ch == BCHAR || ch == BRCHAR )  {
	    ungetc(ch, fp_in);
	    position--;
	    break;
	}   /* End if */

	switch ( ch )  {
	    case '\012':
		vgoto(vpos - linespace);

	    case '\015':
		hgoto(0);
		fprintf(fp_out, ")t\n%d %d %d(", angle, hpos, vpos);
		break;

	    case '(':
	    case ')':
	    case '\\':
		putc('\\', fp_out);

	    default:
		if ( isascii(ch) && isprint(ch) )
		    putc(ch, fp_out);
		else fprintf(fp_out, "\\%.3o", ch & 0377);
		break;
	}   /* End switch */
    }	/* End while */

    fprintf(fp_out, ") t\n");

}   /* End of text */

/*****************************************************************************/

formfeed()

{

    int		ch;			/* repeat count for this page */

/*
 *
 * Does whatever is needed to print the last page and get ready for the next one.
 * It's called, from conv(), after a BEND code is processed. I'm ignoring the
 * copy count that's expected to follow each page.
 *
 */

    if ( bgimode == BGRAPH && (ch = get_char()) != EOF  &&  ! (ch & MSB) )  {
	ungetc(ch, fp_in);
	position--;
    }	/* End if */

    if ( fp_out == stdout )		/* count the last page */
	printed++;

    fprintf(fp_out, "cleartomark\n");
    fprintf(fp_out, "showpage\n");
    fprintf(fp_out, "saveobj restore\n");
    fprintf(fp_out, "%s %d %d\n", ENDPAGE, page, printed);

    while ( (ch = get_char()) == 0 ) ;	/* skip any NULL characters */
    ungetc(ch, fp_in);
    position--;

    if ( ungetc(getc(fp_in), fp_in) == EOF )
	redirect(-1);
    else redirect(++page);

    fprintf(fp_out, "%s %d %d\n", PAGE, page, printed+1);
    fprintf(fp_out, "/saveobj save def\n");
    fprintf(fp_out, "mark\n");
    writerequest(printed+1, fp_out);
    fprintf(fp_out, "%d pagesetup\n", printed+1);

    setsize(bgisize);
    hpos = vpos = 0;

}    /* End of formfeed */

/*****************************************************************************/

subr_def()

{

/*
 *
 * Starts a subroutine definition. All subroutines are defined as PostScript
 * procedures that begin with the character S and end with the subroutine's id
 * (a number between 0 and 63 - I guess). The primary, and perhaps only use of
 * subroutines is in special color plots produced by several graphics libraries,
 * and even there it's not all that common. I've also chosen not to worry about
 * nested subroutine definitions - that would certainly be overkill!
 *
 * All subroutines set up their own (translated) coordinate system, do their work
 * in that system, and restore things when they exit. To make everything work
 * properly we save the current point (in shpos and svpos), set our position to
 * (0, 0), and restore things at the end of the subroutine definition. That means
 * hpos and vpos measure the relative displacement after a subroutine returns, and
 * we save those values in the displacement[] array. The displacements are used
 * (in subr_call()) to properly adjust our position after each subroutine call,
 * and all subroutines are called with the current x and y coordinates on top of
 * the stack.
 *
 */

    if ( in_subr == TRUE )		/* a nested subroutine definition?? */
	error(FATAL, "can't handle nested subroutine definitions");

    if ( (subr_id = get_data()) == EOF )
	error(FATAL, "missing subroutine identifier");

    if ( in_global == FALSE )  {	/* just used to reduce file size some */
	fprintf(fp_out, "cleartomark\n");
	fprintf(fp_out, "saveobj restore\n");
	fprintf(fp_out, "%s", BEGINGLOBAL);
	in_global = TRUE;
    }	/* End if */

    fprintf(fp_out, "/S%d {\n", subr_id);
    fprintf(fp_out, "gsave translate\n");

    shpos = hpos;			/* save our current position */
    svpos = vpos;

    hgoto(0);				/* start at the origin */
    vgoto(0);

    in_subr = TRUE;			/* in a subroutine definition */

}   /* End of subr_def */

/*****************************************************************************/

subr_end()

{

    int		ch;			/* for looking at next opcode */

/*
 *
 * Handles stuff needed at the end of each subroutine. Want to remember the change
 * in horizontal and vertical positions for each subroutine so we can adjust our
 * position after each call - just in case. The current position was set to (0, 0)
 * before we started the subroutine definition, so when we get here hpos and vpos
 * are the relative displacements after the subroutine is called. They're saved in
 * the displacement[] array and used to adjust the current position when we return
 * from a subroutine.
 *
 */

    if ( in_subr == FALSE )		/* not in a subroutine definition?? */
	error(FATAL, "subroutine end without corresponding start");

    fprintf(fp_out, "grestore\n");
    fprintf(fp_out, "} def\n");

    if ( in_global == TRUE && (ch = get_char()) != BSUB )  {
	fprintf(fp_out, "%s", ENDGLOBAL);
	fprintf(fp_out, "/saveobj save def\n");
	fprintf(fp_out, "mark\n");
	in_global = FALSE;
    }	/* End if */

    ungetc(ch, fp_in);			/* put back the next opcode */

    displacement[subr_id].dx = hpos;
    displacement[subr_id].dy = vpos;

    hgoto(shpos);			/* back to where we started */
    vgoto(svpos);

    in_subr = FALSE;			/* done with the definition */

}   /* End of subr_end */

/*****************************************************************************/

subr_call()

{

    int		ch;			/* next byte from *fp_in */
    int		id;			/* subroutine id if ch wasn't an opcode */

/*
 *
 * Handles subroutine calls. Everything that follows the BCALL opcode (up to the
 * next opcode) is taken as a subroutine identifier - thus the loop that generates
 * the subroutine calls.
 *
 */

    while ( (ch = get_char()) != EOF && (ch & MSB) )  {
	id = ch & DMASK;
	fprintf(fp_out, "%d %d S%d\n", hpos, vpos, id);

	hgoto(hpos + displacement[id].dx);	/* adjust our position */
	vgoto(vpos + displacement[id].dy);
    }	/* End while */

    ungetc(ch, fp_in);

}   /* End of subr_call */

/*****************************************************************************/

vector(var, mode)

    int		var;			/* coordinate that varies next? */
    int		mode;			/* VISIBLE or INVISIBLE vectors */

{

    int		ch;			/* next character from *fp_in */
    int		x, y;			/* line drawn to this point */
    int		count = 0;		/* number of points so far */

/*
 *
 * Handles plotting of all types of BGI vectors. If it's a manhattan vector var
 * specifies which coordinate will be changed by the next number in the input
 * file.
 *
 */

    x = hpos;				/* set up the first point */
    y = vpos;

    while ( (ch = get_char()) != EOF  &&  ch & MSB )  {
	if ( var == X_COORD )		/* next length is change in x */
	    x += get_int(ch);
	else if ( var == Y_COORD )	/* it's the change in y */
	    y += get_int(ch);
	else if ( var == LONGVECTOR )  {	/* long vector */
	    x += get_int(ch);
	    y += get_int(0);
	} else {			/* must be a short vector */
	    x += ((ch & MSBMAG) * ((ch & SGNB) ? -1 : 1));
	    y += (((ch = get_data()) & MSBMAG) * ((ch & SGNB) ? -1 : 1));
	}   /* End else */

	if ( mode == VISIBLE )  {	/* draw the line segment */
	    fprintf(fp_out, "%d %d\n", hpos - x, vpos - y);
	    count++;
	}   /* End if */

	hgoto(x);			/* adjust the current BGI position */
	vgoto(y);

	if ( var == X_COORD )		/* vertical length comes next */
	    var = Y_COORD;
	else if ( var == Y_COORD )	/* change horizontal next */
	    var = X_COORD;
    }	/* End while */

    if ( count > 0 )
	fprintf(fp_out, "%d %d v\n", hpos, vpos);

    ungetc(ch, fp_in);			/* it wasn't part of the vector */
    position--;

}   /* End of vector */

/*****************************************************************************/

rectangle(mode)

    int		mode;			/* FILL or OUTLINE the rectangle */

{

    int		deltax;			/* displacement for horizontal side */
    int		deltay;			/* same but for vertical sides */

/*
 *
 * Draws a rectangle and either outlines or fills it, depending on the value of
 * mode. Would be clearer, and perhaps better, if {stroke} or {fill} were put on
 * the stack instead of 0 or 1. R could then define the path and just do an exec
 * to fill or stroke it.
 *
 */

    deltax = get_int(0);		/* get the height and width */
    deltay = get_int(0);

    if ( mode == OUTLINE )
	fprintf(fp_out, "0 %d %d %d %d R\n", deltax, deltay, hpos, vpos);
    else fprintf(fp_out, "1 %d %d %d %d R\n", deltax, deltay, hpos, vpos);

}   /* End of rectangle */

/*****************************************************************************/

trapezoid()

{

    int		kind;			/* which sides are parallel */
    int		d[6];			/* true displacements - depends on kind */

/*
 *
 * Handles filled trapeziods. A data byte of 0101 following the opcode means the
 * horizontal sides are parallel, 0102 means the vertical sides are parallel.
 * Filling is handled by eofill so we don't need to get things in the right order.
 *
 */

    kind = get_data();

    d[0] = get_int(0);
    d[1] = 0;
    d[2] = get_int(0);
    d[3] = get_int(0);
    d[4] = get_int(0);
    d[5] = 0;

    if ( kind == 2 )  {			/* parallel sides are vertical */
	d[1] = d[0];
	d[0] = 0;
	d[5] = d[4];
	d[4] = 0;
    }	/* End if */

    fprintf(fp_out, "%d %d %d %d %d %d %d %d T\n", d[4], d[5], d[2], d[3], d[0], d[1], hpos, vpos);

}   /* End of trapezoid */

/*****************************************************************************/

point_plot(mode, ch)

    int		mode;			/* plotting mode BPOINT or BPOINT1 */
    int		ch;			/* will be placed at the points */

{

    int		c;			/* next character from input file */
    int		x, y;			/* ch gets put here next */
    int		deltax;			/* x increment for BPOINT1 mode */

/*
 *
 * The two point plot modes are used to place a character at selected points. The
 * difference in the two modes, namely BPOINT and BPOINT1, is the way we get the
 * coordinates of the next point. In BPOINT1 the two bytes immediately following
 * ch select a constant horizontal change, while both coordinates are given for
 * all points in BPOINT mode.
 *
 */

    if ( mode == BPOINT1 )  {		/* first integer is change in x */
	deltax = get_int(0);
	x = hpos - deltax;
    }	/* End if */

    while ( (c = get_char()) != EOF  &&  (c & MSB) )  {
	if ( mode == BPOINT1 )  {	/* only read y coordinate */
	    y = get_int(c);
	    x += deltax;
	} else {			/* get new x and y from input file */
	    x = get_int(c);
	    y = get_int(0);
	}   /* End else */

	hgoto(x);			/* adjust BGI position */
	vgoto(y);

	fprintf(fp_out, "%d %d\n", hpos, vpos);
    }	/* End while */

    putc('(', fp_out);

    switch ( ch )  {
	case '(':
	case ')':
	case '\\':
		putc('\\', fp_out);

	default:
		putc(ch, fp_out);
    }	/* End switch */

    fprintf(fp_out, ")pp\n");

    ungetc(c, fp_in);			/* it wasn't part of the point plot */
    position--;

}   /* End of point_plot */

/*****************************************************************************/

line_plot()

{

    int		c;			/* next input character from fp_in */
    int		deltax;			/* change in x coordinate */
    int		x0, y0;			/* starting point for next segment */
    int		x1, y1;			/* endpoint of the line */
    int		count = 0;		/* number of points so far */

/*
 *
 * Essentially the same format as BPOINT1, except that in this case we connect
 * pairs of points by line segments.
 *
 */

    deltax = get_int(0);		/* again the change in x is first */

    x1 = hpos;				/* so it works first time through */
    y1 = get_int(0);

    while ( (c = get_char()) != EOF  &&  (c & MSB) )  {
	x0 = x1;			/* line starts here */
	y0 = y1;

	x1 += deltax;			/* and ends at this point */
    	y1 = get_int(c);

	fprintf(fp_out, "%d %d\n", -deltax, y0 - y1);
	count++;
    }	/* End while */

    hgoto(x1);				/* adjust current BGI position */
    vgoto(y1);

    if ( count > 0 )
	fprintf(fp_out, "%d %d v\n", hpos, vpos);

    ungetc(c, fp_in);			/* wasn't part of the line */
    position--;

}   /* End of line_plot */

/*****************************************************************************/

arc(mode)

    int		mode;			/* FILL or OUTLINE the path */

{

    int		dx1, dy1;		/* displacements for first point */
    int		dx2, dy2;		/* same for the second point */
    int		radius;			/* of the arc */
    int		angle1, angle2;		/* starting and ending angles */

/*
 *
 * Called whenever we need to draw an arc. I'm ignoring filled slices for now.
 *
 */

    dx1 = get_int(0);			/* displacements relative to center */
    dy1 = get_int(0);
    dx2 = get_int(0);
    dy2 = get_int(0);

    radius = get_int(0);		/* and the radius */

    if ( radius == 0 )			/* nothing to do */
	return;

    angle1 = (atan2((double) dy1, (double) dx1) * 360) / (2 * PI) + .5;
    angle2 = (atan2((double) dy2, (double) dx2) * 360) / (2 * PI) + .5;

    fprintf(fp_out, "%d %d %d %d %d arcn stroke\n", hpos, vpos, radius, angle1, angle2);

}   /* End of arc */

/*****************************************************************************/

pattern()

{

    double	red = 0;		/* color components */
    double	green = 0;
    double	blue = 0;
    int		kind;			/* corse or fine pattern */
    int		val;			/* next color data byte */
    int		i;			/* loop index */

/*
 *
 * Handles patterns by setting the current color based of the values assigned to
 * the next four data bytes. BGI supports two kinds of patterns (fine or coarse)
 * but I'm handling each in the same way - for now. In a fine pattern the four
 * data bytes assign a color to four individual pixels (upperleft first) while
 * in a coarse pattern the four colors are assigned to groups of four pixels,
 * for a total of 16. Again the first color goes to the group in the upper left
 * corner. The byte immediately following the BPAT opcode selects fine (040) or
 * coarse (041) patterns. The PostScript RGB color is assigned by averaging the
 * RED, GREEN, and BLUE components assigned to the four pixels (or groups of
 * pixels). Acceptable results, but there's no distinction between fine and
 * coarse patterns.
 *
 */

    if ( (kind = get_char()) == EOF )
	error(FATAL, "bad pattern command");

    for ( i = 0; i < 4; i++ )  {
	val = get_data();
	red += get_color(val, RED);
	green += get_color(val, GREEN);
	blue += get_color(val, BLUE);
    }	/* End for */

    fprintf(fp_out, "%g %g %g c\n", red/4, green/4, blue/4);

}   /* End of pattern */

/*****************************************************************************/

get_color(val, component)

    int		val;			/* color data byte */
    int		component;		/* RED, GREEN, or BLUE component */

{


    int		primary;		/* color mixing mode - bits 2 to 4 */
    int		plane;			/* primary color plane - bits 5 to 7 */
    unsigned	rgbcolor;		/* PostScript expects an RGB triple */

/*
 *
 * Picks the requested color component (RED, GREEN, or BLUE) from val and returns
 * the result to the caller. BGI works with Cyan, Yellow, and Magenta so the one's
 * complement stuff (following the exclusive or'ing) recovers the RED, BLUE, and
 * GREEN components that PostScript's setrgbcolor operator needs. The PostScript
 * interpreter in the ColorScript 100 has a setcmycolor operator, but it's not
 * generally available so I've decided to stick with setrgbcolor.
 *
 */

    primary = (val >> 3) & 07;
    plane = val & 07;
    rgbcolor = (~(primary ^ plane)) & 07;

    if ( debug == ON )
	fprintf(stderr, "val = %o, primary = %o, plane = %o, rgbcolor = %o\n",
		val, primary, plane, rgbcolor);

    switch ( component )  {
	case RED:
		return(rgbcolor>>2);

	case GREEN:
		return(rgbcolor&01);

	case BLUE:
		return((rgbcolor>>1)&01);

	default:
		error(FATAL, "unknown color component");
		return(0);
    }	/* End switch */

}   /* End of get_color */

/*****************************************************************************/

set_color(val)

    int		val;			/* color data byte */

{

/*
 *
 * Arranges to have the color set to the value requested in the BGI data byte val.
 *
 */

    fprintf(fp_out, "%d %d %d c\n", get_color(val, RED), get_color(val, GREEN), get_color(val, BLUE));

}   /* End of set_color */

/*****************************************************************************/

get_int(highbyte)

    int		highbyte;		/* already read this byte */

{

    int		lowbyte;		/* this and highbyte make the int */

/*
 *
 * Figures out the value on the integer (sign magnitude form) that's next in the
 * input file. If highbyte is nonzero we'll use it and the next byte to build the
 * integer, otherwise two bytes are read from fp_in.
 *
 */


    if ( highbyte == 0 )		/* need to read the first byte */
	highbyte = get_data();

    lowbyte = get_data();		/* always need the second byte */

    return(highbyte & SGNB ? -MAG(highbyte, lowbyte) : MAG(highbyte, lowbyte));

}   /* End of get_int */

/*****************************************************************************/

get_data()

{

    int		val;			/* data value returned to caller */

/*
 *
 * Called when we expect to find a single data character in the input file. The
 * data bit is turned off and the resulting value is returned to the caller.
 *
 */

    if ( (val = get_char()) == EOF  ||  ! (val & MSB) )
	error(FATAL, "missing data value");

    return(val & DMASK);

}   /* End of get_data */

/*****************************************************************************/

get_char()

{

    int		ch;			/* character we just read */

/*
 *
 * Reads the next character from file *fp_in and returns the value to the caller.
 * This routine isn't really needed, but we may want to deal directly with some
 * screwball file formats so I thought it would probably be a good idea to isolate
 * all the input in one routine that could be easily changed.
 *
 */

    if ( (ch = getc(fp_in)) != EOF )  {
	position++;
	ch &= CHMASK;
    }	/* End if */

    if ( debug == ON )
	fprintf(stderr, "%o ", ch);

    return(ch);

}   /* End of get_char */

/*****************************************************************************/

redirect(pg)

    int		pg;			/* next page we're printing */

{

    static FILE	*fp_null = NULL;	/* if output is turned off */

/*
 *
 * If we're not supposed to print page pg, fp_out will be directed to /dev/null,
 * otherwise output goes to stdout.
 *
 */

    if ( pg >= 0 && in_olist(pg) == ON )
	fp_out = stdout;
    else if ( (fp_out = fp_null) == NULL )
	fp_out = fp_null = fopen("/dev/null", "w");

}   /* End of redirect */

/*****************************************************************************/

