/*
 * Graffiti.c is based on the file Scribble.c copyrighted
 * by Keith Packard:
 *
 * Copyright © 1999 Keith Packard
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of Keith Packard not be used in
 * advertising or publicity pertaining to distribution of the software without
 * specific, written prior permission.  Keith Packard makes no
 * representations about the suitability of this software for any purpose.  It
 * is provided "as is" without express or implied warranty.
 *
 * KEITH PACKARD DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
 * EVENT SHALL KEITH PACKARD BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */

#include <u.h>
#include <libc.h>
#include <draw.h>
#include <scribble.h>

#include "scribbleimpl.h"
#include "graffiti.h"

int ScribbleDebug;

char *cl_name[3] = {
	DEFAULT_LETTERS_FILE,
	DEFAULT_DIGITS_FILE,
	DEFAULT_PUNC_FILE
};

Rune
recognize (Scribble *s)
{
	struct graffiti *graf = s->graf;
	Stroke	    *ps = &s->ps;
	Rune		    rune;
	int		    	c;
	int				nr;
	rec_alternative	*ret;

	if (ps->npts == 0)
		return '\0';

	c = recognizer_translate(
		graf->rec[s->puncShift ? CS_PUNCTUATION : s->curCharSet],
		1, ps, false, &nr, &ret);
	if (c != -1)
		delete_rec_alternative_array(nr, ret, false);

	rune = '\0';

	switch (c) {
	case '\0':
		if(ScribbleDebug)fprint(2, "(case '\\0')\n");
		break;
	case 'A':	/* space */
		rune = ' ';
		if(ScribbleDebug)fprint(2, "(case A) character = ' %C' (0x%x)\n", rune, rune);
		break;
	case 'B':	/* backspace */
		rune = '\b';
		if(ScribbleDebug)fprint(2, "(case B) character = \\b (0x%x)\n", rune);
		break;
	case 'N': /* numlock */
		if(ScribbleDebug)fprint(2, "(case N)\n");
		if (s->curCharSet == CS_DIGITS) {
			s->curCharSet = CS_LETTERS;
		} else {
			s->curCharSet = CS_DIGITS;
		}
		s->tmpShift = 0;
		s->puncShift = 0;
		s->ctrlShift = 0;
		break;
	case 'P': /* usually puncshift, but we'll make it CTRL */
		if(ScribbleDebug)fprint(2, "(case P)\n");
		s->ctrlShift = !s->ctrlShift;
		s->tmpShift = 0;
		s->puncShift = 0;
		break;
	case 'R':	/* newline */
		rune = '\n';
		if(ScribbleDebug)fprint(2, "(case R) character = \\n (0x%x)\n", rune);
		break;
	case 'S': /* shift */
		if(ScribbleDebug)fprint(2, "(case S)\n");
		s->puncShift = 0;
		s->ctrlShift = 0;
		if (s->capsLock) {
			s->capsLock = 0;
			s->tmpShift = 0;
			break;
		}
		if (s->tmpShift == 0) {
			s->tmpShift++;
			break;
		}
		/* fall through */
	case 'L': /* caps lock */
		if(ScribbleDebug)fprint(2, "(case L)\n");
		s->capsLock = !s->capsLock;
		break;
	case '.':	/* toggle punctuation mode */
		if (s->puncShift) {
			s->puncShift = 0;
		} else {
			s->puncShift = 1;
			s->ctrlShift = 0;
			s->tmpShift = 0;
			return rune;
		}		  	
		rune = '.';
		if(0)fprint(2, "(case .) character = %c (0x%x)\n", rune, rune);
		break;
	default:
		if ('A' <= c && c <= 'Z') {
			if(ScribbleDebug)fprint(2, "(bad case?) character = %c (0x%x)\n", c, c);
			return rune;
		}
		rune = c;
		if (s->ctrlShift) 
		{
			if (c < 'a' || 'z' < c)
			{
				if(ScribbleDebug)fprint(2, "(default) character = %c (0x%x)\n", rune, rune);
				return rune;
			}
			rune = rune & 0x1f;
		} else if ((s->capsLock && !s->tmpShift) || 
				 (!s->capsLock && s->tmpShift)) 
		{
			if (rune < 0xff)
				rune = toupper(rune);
		} 
		s->tmpShift = 0;
		s->puncShift = 0;
		s->ctrlShift = 0;
		if(ScribbleDebug)fprint(2, "(default) character = %c (0x%x)\n", rune, rune);
	}
	return rune;
}

/* This procedure is called to initialize pg by loading the three
 * recognizers, loading the initial set of three classifiers, and
 * loading & verifying the recognizer extension functions.  If the
 * directory $HOME/.recognizers exists, the classifier files will be
 * loaded from that directory.  If not, or if there is an error, the
 * default files (directory specified in Makefile) will be loaded
 * instead.  Returns non-zero on success, 0 on failure.  (Adapted from
 * package tkgraf/src/GraffitiPkg.c.
 */

static int
graffiti_load_recognizers(struct graffiti *pg)
{
	bool usingDefault;
	char* homedir;
	int i;
	rec_fn *fns;

	/* First, load the recognizers... */
	/* call recognizer_unload if an error ? */
	for (i = 0; i < NUM_RECS; i++) {
		/* Load the recognizer itself... */
		pg->rec[i] = recognizer_load(DEFAULT_REC_DIR, "", nil);
		if (pg->rec[i] == nil) {
			fprint(2,"Error loading recognizer from %s.", DEFAULT_REC_DIR);
			return 0;
		}
		if ((* (int *)(pg->rec[i])) != 0xfeed) {
			fprint(2,"Error in recognizer_magic.");
			return 0;
		}
	}

	/* ...then figure out where the classifiers are... */
	if ( (homedir = (char*)getenv("home")) == nil ) {
		if(0)fprint(2, "no homedir, using = %s\n", REC_DEFAULT_USER_DIR);
		strecpy(pg->cldir, pg->cldir+sizeof pg->cldir, REC_DEFAULT_USER_DIR);
		usingDefault = true;
	} else {
		if(0)fprint(2, "homedir = %s\n", homedir);
		snprint(pg->cldir, sizeof pg->cldir, "%s/%s", homedir, CLASSIFIER_DIR);
		usingDefault = false;
	}

	/* ...then load the classifiers... */
	for (i = 0; i < NUM_RECS; i++) {
		int rec_return;
		char *s;

		rec_return = recognizer_load_state(pg->rec[i], pg->cldir, cl_name[i]);
		if ((rec_return == -1) && (usingDefault == false)) {
			if(0)fprint(2, "Unable to load custom classifier file %s/%s.\nTrying default classifier file instead.\nOriginal error: %s\n ", 
				pg->cldir, cl_name[i], 
				(s = recognizer_error(pg->rec[i])) ? s : "(none)");
			rec_return = recognizer_load_state(pg->rec[i],
						REC_DEFAULT_USER_DIR, cl_name[i]);
		}
		if (rec_return == -1) {
			fprint(2, "Unable to load default classifier file %s.\nOriginal error: %s\n",
				cl_name[i], 
				(s = recognizer_error(pg->rec[i])) ? s : "(none)");
			return 0;
		}
	}

	/* We have recognizers and classifiers now.   */
	/* Get the vector of LIextension functions..     */
	fns = recognizer_get_extension_functions(pg->rec[CS_LETTERS]);
	if (fns == nil) {
		fprint(2, "LI Recognizer Training:No extension functions!");
		return 0;
	}
	
	/* ... and make sure the training & get-classes functions are okay. */
	if( (pg->rec_train = (li_recognizer_train)fns[LI_TRAIN]) == nil ) {
		fprint(2,
			"LI Recognizer Training:li_recognizer_train() not found!");
		if (fns != nil) {
			free(fns);
		}
		return 0;
	}
  
	if( (pg->rec_getClasses = (li_recognizer_getClasses)fns[LI_GET_CLASSES]) == nil ) {
		fprint(2,
			"LI Recognizer Training:li_recognizer_getClasses() not found!");
		if (fns != nil) {
			free(fns);
		}
		return 0;
	}
	free(fns);
	return 1;
}

Scribble *
scribblealloc(void)
{
	Scribble *s;

	s = mallocz(sizeof(Scribble), 1);
	if (s == nil)
		sysfatal("Initialize: %r");
	s->curCharSet = CS_LETTERS;

	s->graf = mallocz(sizeof(struct graffiti), 1);
	if (s->graf == nil)
		sysfatal("Initialize: %r");

	graffiti_load_recognizers(s->graf);

	return s;
}
