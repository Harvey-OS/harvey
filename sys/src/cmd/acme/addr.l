#include <alef.h>
#include <libg.h>
#include <frame.h>
#include <fcall.h>
#include "dat.h"
#include "fns.h"

enum
{
	None = 0,
	Fore = '+',
	Back = '-',
};

enum
{
	Char,
	Line,
};

int
isaddrc(int r)
{
	if(r && utfrune("0123456789+-/$.#", r)!=nil)
		return TRUE;
	return FALSE;
}

(int, Range)
number(Text *t, Range r, int line, int dir, int size)
{
	uint q0, q1;

	rescue{
		warning(nil, "address out of range\n");
		return (FALSE, r);
	}
	if(size == Char){
		if(dir == Fore)
			line = t->file->nc+line;
		else if(dir == Back)
			line = t->file->nc - line;
		if(line<0 || line>t->file->nc)
			raise;
		return (TRUE, (Range)(line, line));
	}
	(q0, q1) = r;
	switch(dir){
	case None:
		q0 = 0;
		q1 = 0;
	Forward:
		while(line>0 && q1<t->file->nc)
			if(t->readc(q1++) == '\n')
				if(--line > 0)
					q0 = q1;
		if(line > 0)
			raise;
		break;
	case Fore:
		if(q1 > 0)
			while(t->readc(q1-1) != '\n')
				q1++;
		q0 = q1;
		goto Forward;
	case Back:
		if(q0 < t->file->nc)
			while(q0>0 && t->readc(q0-1)!='\n')
				q0--;
		q1 = q0;
		while(line>0 && q0>0){
			if(t->readc(q0-1) == '\n'){
				if(--line >= 0)
					q1 = q0;
			}
			--q0;
		}
		if(line > 0)
			raise;
		while(q0>0 && t->readc(q0-1)!='\n')
			--q0;
	}
	return (TRUE, (Range)(q0, q1));
}


(int, Range)
regexp(Text *t, Range lim, Range r, Rune *pat, int dir)
{
	int found;
	Rangeset sel;
	int q;

	if(pat[0] && rxcompile(pat) == FALSE)
		return (FALSE, r);
	if(dir == Back)
		(found, sel) = rxbexecute(t, r.q0);
	else{
		if(lim.q0 < 0)
			q = Infinity;
		else
			q = lim.q1;
		(found, sel) = rxexecute(t, r.q1, q);
	}
	if(!found)
		warning(nil, "no match for regexp\n");
	return (found, sel.r[0]);
}

(int, int, Range)
address(Text *t, Range lim, Range ar, void *a, uint q0, uint q1, int (*getc)(void*, uint),  int eval)
{
	int dir, size, npat;
	int prevc, c, n;
	uint q;
	Rune *pat;
	Range r, nr;

	r = ar;
	q = q0;
	dir = None;
	size = Line;
	c = 0;
	while(q < q1){
		prevc = c;
		c = (*getc)(a, q++);
		switch(c){
		default:
			return (eval, q-1, r);
		case ';':
			ar = r;
			/* fall through */
		case ',':
			(eval, q, nr) = address(t, lim, ar, a, q, q1, getc, eval);
			r.q1 = nr.q1;
			return (eval, q, r);
		case '+':
		case '-':
			if(eval && (prevc=='+' || prevc=='-'))
				(eval, r) = number(t, r, 1, prevc, Line);	/* do previous one */
			dir = c;
			break;
		case '.':
		case '$':
			if(q != q0+1)
				return (eval, q-1, r);
			if(eval)
				if(c == '.')
					r = ar;
				else
					r = (Range)(t->file->nc, t->file->nc);
			if(q < q1)
				dir = Fore;
			else
				dir = None;
			break;
		case '#':
			if(q==q1 || (c=(*getc)(a, q++))<'0' || '9'<c)
				return (eval, q-1, r);
			size = Char;
			/* fall through */
		case '0': case '1': case '2': case '3': case '4':
		case '5': case '6': case '7': case '8': case '9':
			n = c -'0';
			while(q<q1){
				c = (*getc)(a, q++);
				if(c<'0' || '9'<c){
					q--;
					break;
				}
				n = n*10+(c-'0');
			}
			if(eval)
				(eval, r) = number(t, r, n, dir, size);
			dir = None;
			size = Line;
			break;
		case '/':
			npat = 0;
			pat = nil;
			while(q<q1){
				c = (*getc)(a, q++);
				switch(c){
				case '\n':
					--q;
					break 2;
				case '\\':
					pat = runerealloc(pat, npat+1);
					pat[npat++] = c;
					if(q == q1)
						break 2;
					c = (*getc)(a, q++);
					break;
				case '/':
					break 2;
				}
				pat = runerealloc(pat, npat+1);
				pat[npat++] = c;
			}
			pat = runerealloc(pat, npat+1);
			pat[npat] = 0;
			if(eval)
				(eval, r) = regexp(t, lim, r, pat, dir);
			free(pat);
			dir = None;
			size = Line;
			break;
		}
	}
	if(eval && dir != None)
		(eval, r) = number(t, r, 1, dir, Line);	/* do previous one */
	return (eval, q, r);
}
