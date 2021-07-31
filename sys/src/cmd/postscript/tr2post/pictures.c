/*
 *
 * PostScript picture inclusion routines. Support for managing in-line pictures
 * has been added, and works in combination with the simple picpack pre-processor
 * that's supplied with this package. An in-line picture begins with a special
 * device control command that looks like,
 *
 *		x X InlinPicture name size
 *
 * where name is the pathname of the original picture file and size is the number
 * of bytes in the picture, which begins immediately on the next line. When dpost
 * encounters the InlinePicture device control command inlinepic() is called and
 * that routine appends the string name and the integer size to a temporary file
 * (fp_pic) and then adds the next size bytes read from the current input file to
 * file fp_pic. All in-line pictures are saved in fp_pic and located later using
 * the name string and picture file size that separate pictures saved in fp_pic.
 *
 * When a picture request (ie. an "x X PI" command) is encountered picopen() is
 * called and it first looks for the picture file in fp_pic. If it's found there
 * the entire picture (ie. size bytes) is copied from fp_pic to a new temp file
 * and that temp file is used as the picture file. If there's nothing in fp_pic
 * or if the lookup failed the original route is taken.
 *
 * Support for in-line pictures is an attempt to address requirements, expressed
 * by several organizations, of being able to store a document as a single file
 * (usually troff input) that can then be sent through dpost and ultimately to
 * a PostScript printer. The mechanism may help some users, but the are obvious
 * disadvantages to this approach, and the original mechanism is the recommended
 * approach! Perhaps the most important problem is that troff output, with in-line
 * pictures included, doesn't fit the device independent language accepted by
 * important post-processors (like proff) and that means you won't be able to
 * reliably preview a packed file on your 5620 (or whatever).
 *
 */

#include <u.h>
#include <libc.h>
#include <bio.h>
#include <stdio.h>
#include "ext.h"
#include "common.h"
#include "tr2post.h"
/* PostScript file structuring comments */
#include "comments.h"
/* general purpose definitions */
/* #include "gen.h" */
/* just for TEMPDIR definition */
#include "path.h"
/* external variable declarations */
/* #include "ext.h" */

Biobuf	*bfp_pic = NULL;
Biobufhdr	*Bfp_pic;
Biobufhdr	*picopen(char *);

#define MAXGETFIELDS	16
char *fields[MAXGETFIELDS];
int nfields;

extern int	devres, hpos, vpos;
extern int	picflag;

/*****************************************************************************/

void
picture(Biobufhdr *inp, char *buf) {
	int	poffset;		/* page offset */
	int	indent;		/* indent */
	int	length;		/* line length  */
	int	totrap;		/* distance to next trap */
	char	name[100];	/* picture file and page string */
	char	hwo[40], *p;	/* height, width and offset strings */
	char	flags[20];		/* miscellaneous stuff */
	int	page = 1;		/* page number pulled from name[] */
	double	frame[4];	/* height, width, y, and x offsets from hwo[] */
	char	units;		/* scale indicator for frame dimensions */
	int	whiteout = 0;	/* white out the box? */
	int	outline = 0;	/* draw a box around the picture? */
	int	scaleboth = 0;	/* scale both dimensions? */
	double	adjx = 0.5;	/* left-right adjustment */
	double	adjy = 0.5;	/* top-bottom adjustment */
	double	rot = 0;	/* rotation in clockwise degrees */
	Biobufhdr	*fp_in;	/* for *name */
	int	i;			/* loop index */

/*
 *
 * Called from devcntrl() after an 'x X PI' command is found. The syntax of that
 * command is:
 *
 *	x X PI:args
 *
 * with args separated by colons and given by:
 *
 *	poffset
 *	indent
 *	length
 *	totrap
 *	file[(page)]
 *	height[,width[,yoffset[,xoffset]]]
 *	[flags]
 *
 * poffset, indent, length, and totrap are given in machine units. height, width,
 * and offset refer to the picture frame in inches, unless they're followed by
 * the u scale indicator. flags is a string that provides a little bit of control
 * over the placement of the picture in the frame. Rotation of the picture, in
 * clockwise degrees, is set by the a flag. If it's not followed by an angle
 * the current rotation angle is incremented by 90 degrees, otherwise the angle
 * is set by the number that immediately follows the a.
 *
 */

	if (!picflag)		/* skip it */
		return;
	endstring();

	flags[0] = '\0';			/* just to be safe */

	nfields = getfields(buf, fields, MAXGETFIELDS, 0, ":\n");
	if (nfields < 6) {
		error(WARNING, "too few arguments to specify picture");
		return;
	}
	poffset = atoi(fields[1]);
	indent = atoi(fields[2]);
	length = atoi(fields[3]);
	totrap = atoi(fields[4]);
	strncpy(name, fields[5], sizeof(name));
	strncpy(hwo, fields[6], sizeof(hwo));
	if (nfields >= 6)
		strncpy(flags, fields[7], sizeof(flags));

	nfields = getfields(buf, fields, MAXGETFIELDS, 0, "()");
	if (nfields == 2) {
		strncpy(name, fields[0], sizeof(name));
		page = atoi(fields[1]);
	}

	if ((fp_in = picopen(name)) == NULL) {
		error(WARNING, "can't open picture file %s\n", name);
		return;
	}

	frame[0] = frame[1] = -1;		/* default frame height, width */
	frame[2] = frame[3] = 0;		/* and y and x offsets */

	for (i = 0, p = hwo-1; i < 4 && p != NULL; i++, p = strchr(p, ','))
		if (sscanf(++p, "%lf%c", &frame[i], &units) == 2)
	    		if (units == 'i' || units == ',' || units == '\0')
				frame[i] *= devres;

	if (frame[0] <= 0)		/* check what we got for height */
		frame[0] = totrap;

    	if (frame[1] <= 0)		/* and width - check too big?? */
		frame[1] = length - indent;

	frame[3] += poffset + indent;	/* real x offset */

	for (i = 0; flags[i]; i++)
		switch (flags[i]) {
		case 'c': adjx = adjy = 0.5; break;	/* move to the center */
		case 'l': adjx = 0; break;		/* left */
		case 'r': adjx = 1; break;		/* right */
		case 't': adjy = 1; break;		/* top */
		case 'b': adjy = 0; break;		/* or bottom justify */
		case 'o': outline = 1; break;	/* outline the picture */
		case 'w': whiteout = 1; break;	/* white out the box */
		case 's': scaleboth = 1; break;	/* scale both dimensions */
		case 'a': if ( sscanf(&flags[i+1], "%lf", &rot) != 1 )
			  rot += 90;
	}

	/* restore(); */
	endstring();
	Bprint(Bstdout, "cleartomark\n");
	Bprint(Bstdout, "saveobj restore\n");

	ps_include(fp_in, Bstdout, page, whiteout, outline, scaleboth,
		frame[3]+frame[1]/2, -vpos-frame[2]-frame[0]/2, frame[1], frame[0], adjx, adjy, -rot);
	/* save(); */
	Bprint(Bstdout, "/saveobj save def\n");
	Bprint(Bstdout, "mark\n");
	Bterm(fp_in);

}

/*
 *
 * Responsible for finding and opening the next picture file. If we've accumulated
 * any in-line pictures fp_pic won't be NULL and we'll look there first. If *path
 * is found in *fp_pic we create another temp file, open it for update, unlink it,
 * copy in the picture, seek back to the start of the new temp file, and return
 * the file pointer to the caller. If fp_pic is NULL or the lookup fails we just
 * open file *path and return the resulting file pointer to the caller.
 *
 */
Biobufhdr *
picopen(char *path) {
/*	char	name[100];	/* pathnames */
/*	long	pos;			/* current position */
/*	long	total;			/* and sizes - from *fp_pic */
	Biobuf *bfp;
	Biobufhdr	*Bfp;		/* and pointer for the new temp file */


	if ((bfp = Bopen(path, OREAD)) == 0)
		error(FATAL, "can't open %s\n", path);
	Bfp = &(bfp->Biobufhdr);
	return(Bfp);
#ifdef UNDEF
	if (Bfp_pic != NULL) {
		Bseek(Bfp_pic, 0L, 0);
		while (Bgetfield(Bfp_pic, 's', name, 99)>0
			&& Bgetfield(Bfp_pic, 'd', &total, 0)>0) {
			pos = Bseek(Bfp_pic, 0L, 1);
			if (strcmp(path, name) == 0) {
				if (tmpnam(pictmpname) == NULL)
					error(FATAL, "can't generate temp file name");
				if ( (bfp = Bopen(pictmpname, ORDWR)) == NULL )
					error(FATAL, "can't open %s", pictmpname);
				Bfp = &(bfp->Biobufhdr);
				piccopy(Bfp_pic, Bfp, total);
				Bseek(Bfp, 0L, 0);
				return(Bfp);
	    		}
			Bseek(Bfp_pic, total+pos, 0);
		}
	}
	if ((bfp = Bopen(path, OREAD)) == 0)
		Bfp = 0;
	else
		Bfp = &(bfp->Biobufhdr);
	return(Bfp);
#endif
}

/*
 *
 * Adds an in-line picture file to the end of temporary file *Bfp_pic. All pictures
 * grabbed from the input file are saved in the same temp file. Each is preceeded
 * by a one line header that includes the original picture file pathname and the
 * size of the picture in bytes. The in-line picture file is opened for update,
 * left open, and unlinked so it disappears when we do.
 *
 */
/*	*fp;			/* current input file */
/*	*buf;			/* whatever followed "x X InlinePicture" */

#ifdef UNDEF
void
inlinepic(Biobufhdr *Bfp, char *buf) {
	char	name[100];		/* picture file pathname */
	long	total;			/* and size - both from *buf */


	if (Bfp_pic == NULL ) {
		tmpnam(pictmpname);
		if ((bfp_pic = Bopen(pictmpname, ORDWR)) == 0)
	    		error(FATAL, "can't open in-line picture file %s", ipictmpname);
		unlink(pictmpname);
	}

	if ( sscanf(buf, "%s %ld", name, &total) != 2 )
		error(FATAL, "in-line picture error");

	fseek(Bfp_pic, 0L, 2);
	fprintf(Bfp_pic, "%s %ld\n", name, total);
	getc(fp);
	fflush(fp_pic);
	piccopy(fp, fp_pic, total);
	ungetc('\n', fp);

}
#endif

/*
 *
 * Copies total bytes from file fp_in to fp_out. Used to append picture files to
 * *fp_pic and then copy them to yet another temporary file immediately before
 * they're used (in picture()).
 *
 */
/*	*fp_in;	input */
/*	*fp_out;	and output file pointers */
/*	total;		number of bytes to be copied */
void
piccopy(Biobufhdr *Bfp_in, Biobufhdr *Bfp_out, long total) {
	long i;

	for (i = 0; i < total; i++)
		if (Bputc(Bfp_out, Bgetc(Bfp_in)) < 0)
			error(FATAL, "error copying in-line picture file");
	Bflush(Bfp_out);
}
