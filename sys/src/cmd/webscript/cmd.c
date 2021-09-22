#include <u.h>
#include <libc.h>
#include <bio.h>
#include <draw.h>
#include <html.h>
#include "dat.h"

int		errorstop;
int		xtrace;

static int
xnop(Cmd *c)
{
	USED(c);
	return 0;
}

static int
xnot(Cmd *c)
{
	return !runcmd(c->left);
}

static int
xlist(Cmd *c)
{
	runcmd(c->left);
	return runcmd(c->right);
}

static int
xif(Cmd *c)
{
	if(runcmd(c->cond))
		runcmd(c->left);
	else
		runcmd(c->right);
	return 0;
}

static int
xfind(Cmd *c)
{
	if(xtrace)
		print("# find %s%W%s%T%s%y\n",
			c->arg ? " " : "", c->arg,
			c->arg1 ? " " : "", c->arg1,
			c->s ? " " : "", c->s);
	return find(c->arg, c->arg1, c->s);
}

static int
xload(Cmd *c)
{
	if(xtrace)
		print("# load %s\n", c->s);
	loadurl(c->s, nil);
	return 1;
}

static int
xinput(Cmd *c)
{
	if(xtrace)
		print("# input %y\n", c->s);
	return inputvalue(&focus, c->s);
}

static int
xprint(Cmd *c)
{
	if(c->s){
		if(xtrace)
			print("# print %y\n", c->s);
		print("%s", c->s);
		return 1;
	}
	if(xtrace)
		print("# print\n");
	printfocus(&focus);
	return 1;
}

static int
xsubmit(Cmd *c)
{
	Formfield *ff;

	USED(c);
	if(xtrace)
		print("submit\n");
		
	switch(focus.type){
	case FocusForm:
		return submitform(focus.form, nil);
	case FocusFormfield:
		ff = focus.formfield;
		if(ff->ftype == Fsubmit || ff->ftype == Fimage)
			return submitform(ff->form, ff);
		return submitform(ff->form, nil);
	default:
		print("submit: bad focus\n");
		return 0;
	}
}

/*
 * Dispatch.
 */
int (*xfn[])(Cmd*) =
{
	xnop,
	xlist,
	xlist,
	xif,
	xinput,
	xfind,
	xload,
	xnot,
	xprint,
	xsubmit,
	xlist,
};

Cmd*
mkcmd(int op, int arg, char *s)
{
	Cmd *c;
	
	c = emalloc(sizeof *c);
	c->xop = op;
	c->arg = arg;
	if(s && s[0])
		c->s = estrdup(s);
	return c;
}

int
runcmd(Cmd *c)
{
	if(c == nil)
		return 0;
	if(c->xop < 0 || c->xop >= nelem(xfn) || xfn[c->xop] == nil)
		sysfatal("bad cmd %d", c->xop);
	return xfn[c->xop](c);
}

