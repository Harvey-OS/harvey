#include <u.h>
#include <libc.h>
#include <draw.h>
#include <thread.h>
#include <cursor.h>
#include <mouse.h>
#include <keyboard.h>
#include <frame.h>
#include <fcall.h>
#include <plumb.h>
#include "dat.h"
#include "fns.h"
#include "edit.h"

static char Wsequence[] = "warning: changes out of sequence\n";
static int	warned = FALSE;

/*
 * Log of changes made by editing commands.  Three reasons for this:
 * 1) We want addresses in commands to apply to old file, not file-in-change.
 * 2) It's difficult to track changes correctly as things move, e.g. ,x m$
 * 3) This gives an opportunity to optimize by merging adjacent changes.
 * It's a little bit like the Undo/Redo log in Files, but Point 3) argues for a
 * separate implementation.  To do this well, we use Replace as well as
 * Insert and Delete
 */

typedef struct Buflog Buflog;
struct Buflog
{
	short	type;		/* Replace, Filename */
	uint		q0;		/* location of change (unused in f) */
	uint		nd;		/* # runes to delete */
	uint		nr;		/* # runes in string or file name */
};

enum
{
	Buflogsize = sizeof(Buflog)/sizeof(Rune),
};

/*
 * Minstring shouldn't be very big or we will do lots of I/O for small changes.
 * Maxstring is RBUFSIZE so we can fbufalloc() once and not realloc elog.r.
 */
enum
{
	Minstring = 16,		/* distance beneath which we merge changes */
	Maxstring = RBUFSIZE,	/* maximum length of change we will merge into one */
};

void
eloginit(File *f)
{
	if(f->elog.type != Empty)
		return;
	f->elog.type = Null;
	if(f->elogbuf == nil)
		f->elogbuf = emalloc(sizeof(Buffer));
	if(f->elog.r == nil)
		f->elog.r = fbufalloc();
	bufreset(f->elogbuf);
}

void
elogclose(File *f)
{
	if(f->elogbuf){
		bufclose(f->elogbuf);
		free(f->elogbuf);
		f->elogbuf = nil;
	}
}

void
elogreset(File *f)
{
	f->elog.type = Null;
	f->elog.nd = 0;
	f->elog.nr = 0;
}

void
elogterm(File *f)
{
	elogreset(f);
	if(f->elogbuf)
		bufreset(f->elogbuf);
	f->elog.type = Empty;
	fbuffree(f->elog.r);
	f->elog.r = nil;
	warned = FALSE;
}

void
elogflush(File *f)
{
	Buflog b;

	b.type = f->elog.type;
	b.q0 = f->elog.q0;
	b.nd = f->elog.nd;
	b.nr = f->elog.nr;
	switch(f->elog.type){
	default:
		warning(nil, "unknown elog type 0x%ux\n", f->elog.type);
		break;
	case Null:
		break;
	case Insert:
	case Replace:
		if(f->elog.nr > 0)
			bufinsert(f->elogbuf, f->elogbuf->nc, f->elog.r, f->elog.nr);
		/* fall through */
	case Delete:
		bufinsert(f->elogbuf, f->elogbuf->nc, (Rune*)&b, Buflogsize);
		break;
	}
	elogreset(f);
}

void
elogreplace(File *f, int q0, int q1, Rune *r, int nr)
{
	uint gap;

	if(q0==q1 && nr==0)
		return;
	eloginit(f);
	if(f->elog.type!=Null && q0<f->elog.q0){
		if(warned++ == 0)
			warning(nil, Wsequence);
		elogflush(f);
	}
	/* try to merge with previous */
	gap = q0 - (f->elog.q0+f->elog.nd);	/* gap between previous and this */
	if(f->elog.type==Replace && f->elog.nr+gap+nr<Maxstring){
		if(gap < Minstring){
			if(gap > 0){
				bufread(f, f->elog.q0+f->elog.nd, f->elog.r+f->elog.nr, gap);
				f->elog.nr += gap;
			}
			f->elog.nd += gap + q1-q0;
			runemove(f->elog.r+f->elog.nr, r, nr);
			f->elog.nr += nr;
			return;
		}
	}
	elogflush(f);
	f->elog.type = Replace;
	f->elog.q0 = q0;
	f->elog.nd = q1-q0;
	f->elog.nr = nr;
	if(nr > RBUFSIZE)
		editerror("internal error: replacement string too large(%d)", nr);
	runemove(f->elog.r, r, nr);
}

void
eloginsert(File *f, int q0, Rune *r, int nr)
{
	int n;

	if(nr == 0)
		return;
	eloginit(f);
	if(f->elog.type!=Null && q0<f->elog.q0){
		if(warned++ == 0)
			warning(nil, Wsequence);
		elogflush(f);
	}
	/* try to merge with previous */
	if(f->elog.type==Insert && q0==f->elog.q0 && (q0+nr)-f->elog.q0<Maxstring){
		f->elog.r = runerealloc(f->elog.r, f->elog.nr+nr);
		runemove(f->elog.r+f->elog.nr, r, nr);
		f->elog.nr += nr;
		return;
	}
	while(nr > 0){
		elogflush(f);
		f->elog.type = Insert;
		f->elog.q0 = q0;
		n = nr;
		if(n > RBUFSIZE)
			n = RBUFSIZE;
		f->elog.nr = n;
		runemove(f->elog.r, r, n);
		r += n;
		nr -= n;
	}
}

void
elogdelete(File *f, int q0, int q1)
{
	if(q0 == q1)
		return;
	eloginit(f);
	if(f->elog.type!=Null && q0<f->elog.q0+f->elog.nd){
		if(warned++ == 0)
			warning(nil, Wsequence);
		elogflush(f);
	}
	/* try to merge with previous */
	if(f->elog.type==Delete && f->elog.q0+f->elog.nd==q0){
		f->elog.nd += q1-q0;
		return;
	}
	elogflush(f);
	f->elog.type = Delete;
	f->elog.q0 = q0;
	f->elog.nd = q1-q0;
}

void
elogapply(File *f)
{
	Buflog b;
	Rune *buf;
	uint i, n, up, mod;
	Buffer *log;

	elogflush(f);
	log = f->elogbuf;

	buf = fbufalloc();
	mod = FALSE;
	while(log->nc > 0){
		up = log->nc-Buflogsize;
		bufread(log, up, (Rune*)&b, Buflogsize);
		switch(b.type){
		default:
			fprint(2, "elogapply: 0x%ux\n", b.type);
			abort();
			break;

		case Replace:
			if(!mod){
				mod = TRUE;
				filemark(f);
			}
			textdelete(f->curtext, b.q0, b.q0+b.nd, TRUE);
			up -= b.nr;
			for(i=0; i<b.nr; i+=n){
				n = b.nr - i;
				if(n > RBUFSIZE)
					n = RBUFSIZE;
				bufread(log, up+i, buf, n);
				textinsert(f->curtext, b.q0+i, buf, n, TRUE);
			}
			f->curtext->q0 = b.q0;
			f->curtext->q1 = b.q0+b.nr;
			break;

		case Delete:
			if(!mod){
				mod = TRUE;
				filemark(f);
			}
			textdelete(f->curtext, b.q0, b.q0+b.nd, TRUE);
			f->curtext->q0 = b.q0;
			f->curtext->q1 = b.q0;
			break;

		case Insert:
			if(!mod){
				mod = TRUE;
				filemark(f);
			}
			up -= b.nr;
			for(i=0; i<b.nr; i+=n){
				n = b.nr - i;
				if(n > RBUFSIZE)
					n = RBUFSIZE;
				bufread(log, up+i, buf, n);
				textinsert(f->curtext, b.q0+i, buf, n, TRUE);
			}
			f->curtext->q0 = b.q0;
			f->curtext->q1 = b.q0+b.nr;
			break;

/*		case Filename:
			f->seq = u.seq;
			fileunsetname(f, epsilon);
			f->mod = u.mod;
			up -= u.n;
			free(f->name);
			if(u.n == 0)
				f->name = nil;
			else
				f->name = runemalloc(u.n);
			bufread(delta, up, f->name, u.n);
			f->nname = u.n;
			break;
*/
		}
		bufdelete(log, up, log->nc);
	}
	fbuffree(buf);
	elogterm(f);
}
