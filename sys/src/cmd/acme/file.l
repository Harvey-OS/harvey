#include <alef.h>
#include <libg.h>
#include <frame.h>
#include <fcall.h>
#include "dat.h"
#include "fns.h"

/*
 * Structure of Undo list:
 * 	The Undo structure follows any associated data, so the list
 *	can be read backwards: read the structure, then read whatever
 *	data is associated (insert string, file name) and precedes it.
 *	The structure includes the previous value of the modify bit
 *	and a sequence number; successive Undo structures with the
 *	same sequence number represent simultaneous changes.
 */

aggr Undo
{
	sint	type;		/* Delete, Insert, Filename */
	sint	mod;	/* modify bit */
	uint	seq;		/* sequence number */
	uint	p0;		/* location of change (unused in f) */
	uint	n;		/* # runes in string or file name */
};

enum
{
	Undosize = sizeof(Undo)/sizeof(Rune),
};

File*
File.addtext(File *f, Text *t)
{
	if(f == nil)
		f = malloc(sizeof(File));
	f->text = realloc(f->text, (f->ntext+1)*sizeof(Text*));
	f->text[f->ntext++] = t;
	f->curtext = t;
	return f;
}

void
File.deltext(File *f, Text *t)
{
	int i;

	for(i=0; i<f->ntext; i++)
		if(f->text[i] == t)
			goto Found;
	error("can't find text in File.deltext");

    Found:
	f->ntext--;
	if(f->ntext == 0){
		f->close();
		return;
	}
	memmove(f->text+i, f->text+i+1, (f->ntext-i)*sizeof(Text*));
	if(f->curtext == t)
		f->curtext = f->text[0];
}

void
File.insert(File *f, uint p0, Rune *s, uint ns)
{
	check p0<=f->nc;
	if(f->seq > 0)
		f->uninsert(&f->delta, p0, ns);
	f->Buffer.insert(p0, s, ns);
	if(ns)
		f->mod = TRUE;
}

void
File.uninsert(File *f, Buffer *delta, uint p0, uint ns)
{
	Undo u;

	/* undo an insertion by deleting */
	u.type = Delete;
	u.mod = f->mod;
	u.seq = f->seq;
	u.p0 = p0;
	u.n = ns;
	delta->insert(delta->nc, (Rune*)&u, Undosize);
}

void
File.delete(File *f, uint p0, uint p1)
{
	check p0<=p1 && p0<=f->nc && p1<=f->nc;
	if(f->seq > 0)
		f->undelete(&f->delta, p0, p1);
	f->Buffer.delete(p0, p1);
	if(p1 > p0)
		f->mod = TRUE;
}

void
File.undelete(File *f, Buffer *delta, uint p0, uint p1)
{
	Undo u;
	Rune *buf;
	uint i, n;

	/* undo a deletion by inserting */
	u.type = Insert;
	u.mod = f->mod;
	u.seq = f->seq;
	u.p0 = p0;
	u.n = p1-p0;
	buf = fbufalloc();
	for(i=p0; i<p1; i+=n){
		n = p1 - i;
		if(n > RBUFSIZE)
			n = RBUFSIZE;
		f->read(i, buf, n);
		delta->insert(delta->nc, buf, n);
	}
	fbuffree(buf);
	delta->insert(delta->nc, (Rune*)&u, Undosize);

}

void
File.setname(File *f, Rune *name, int n)
{
	if(f->seq > 0)
		f->unsetname(&f->delta);
	free(f->name);
	f->name = runemalloc(n);
	runemove(f->name, name, n);
	f->nname = n;
}

void
File.unsetname(File *f, Buffer *delta)
{
	Undo u;

	/* undo a file name change by restoring old name */
	u.type = Filename;
	u.mod = f->mod;
	u.seq = f->seq;
	u.p0 = 0;	/* unused */
	u.n = f->nname;
	if(f->nname)
		delta->insert(delta->nc, f->name, f->nname);
	delta->insert(delta->nc, (Rune*)&u, Undosize);
}

uint
File.load(File *f, uint p0, int fd, int *nulls)
{
	if(f->seq > 0)
		error("undo in file.load unimplemented");
	return f->Buffer.load(p0, fd, nulls);
}

(uint, uint)
File.undo(File *f, int isundo, uint q0, uint q1)
{
	Undo u;
	Rune *buf;
	uint i, j, n, up;
	uint stop;
	Buffer *delta, *epsilon;

	if(isundo){
		/* undo; reverse delta onto epsilon, seq decreases */
		delta = &f->delta;
		epsilon = &f->epsilon;
		stop = f->seq;
	}else{
		/* redo; reverse epsilon onto delta, seq increases */
		delta = &f->epsilon;
		epsilon = &f->delta;
		stop = 0;	/* don't know yet */
	}

	while(delta->nc > 0){
		up = delta->nc-Undosize;
		delta->read(up, (Rune*)&u, Undosize);
		if(isundo){
			if(u.seq < stop){
				f->seq = u.seq;
				return (q0, q1);
			}
		}else{
			if(stop == 0)
				stop = u.seq;
			if(u.seq > stop)
				return (q0, q1);
		}
		switch(u.type){
		default:
			fprint(2, "undo: 0x%ux\n", u.type);
			check 0;
			break;

		case Delete:
			f->seq = u.seq;
			f->undelete(epsilon, u.p0, u.p0+u.n);
			f->mod = u.mod;
			f->Buffer.delete(u.p0, u.p0+u.n);
			for(j=0; j<f->ntext; j++)
				f->text[j]->delete(u.p0, u.p0+u.n, FALSE);
			q0 = u.p0;
			q1 = u.p0;
			break;

		case Insert:
			f->seq = u.seq;
			f->uninsert(epsilon, u.p0, u.n);
			f->mod = u.mod;
			up -= u.n;
			buf = fbufalloc();
			for(i=0; i<u.n; i+=n){
				n = u.n - i;
				if(n > RBUFSIZE)
					n = RBUFSIZE;
				delta->read(up+i, buf, n);
				f->Buffer.insert(u.p0+i, buf, n);
				for(j=0; j<f->ntext; j++)
					f->text[j]->insert(u.p0+i, buf, n, FALSE);
			}
			fbuffree(buf);
			q0 = u.p0;
			q1 = u.p0+u.n;
			break;

		case Filename:
			f->seq = u.seq;
			f->unsetname(epsilon);
			f->mod = u.mod;
			up -= u.n;
			free(f->name);
			if(u.n == 0)
				f->name = nil;
			else
				f->name = runemalloc(u.n);
			delta->read(up, f->name, u.n);
			f->nname = u.n;
			break;
		}
		delta->delete(up, delta->nc);
	}
	if(isundo)
		f->seq = 0;
	return (q0, q1);
}

void
File.reset(File *f)
{
	f->delta.reset();
	f->epsilon.reset();
	f->seq = 0;
}

void
File.close(File *f)
{
	free(f->name);
	f->nname = 0;
	f->name = nil;
	free(f->text);
	f->ntext = 0;
	f->text = nil;
	f->Buffer.close();
	f->delta.close();
	f->epsilon.close();
}

void
File.mark(File *f)
{
	if(f->epsilon.nc)
		f->epsilon.delete(0, f->epsilon.nc);
	f->seq = seq;
}
