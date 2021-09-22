#include "a.h"

int
yyrulesym(Ygram *g, Yact *fn, Ysym *left, Ysym *prec, Ysym *right0, ...)
{
	int i, n;
	va_list arg;
	Ysym **right;
	
	if(right0 == nil)
		n = 0;
	else{
		va_start(arg, right0);
		for(n=1;; n++)
			if(va_arg(arg, Ysym*) == nil)
				break;
		va_end(arg);
	}
	right = malloc((n+1)*sizeof right[0]);
	if(right == nil)
		return -1;
	right[0] = right0;
	va_start(arg, right0);
	for(i=1; i<n; i++)
		right[i] = va_arg(arg, Ysym*);
	va_end(arg);
	right[i] = nil;
	va_end(arg);
	return yyrule(g, fn, left, prec, right, n);
}

int
yyrulestr(Ygram *g, Yact *fn, char *leftstr, char *precstr, char *right0, ...)
{
	int i, n;
	va_list arg;
	Ysym *left;
	Ysym **right;
	Ysym *prec;
	char *p;
	
	left = yylooksym(g, leftstr);
	if(left == nil)
		return -1;
	if(right0 == nil)
		n = 0;
	else{
		va_start(arg, right0);
		for(n=1;; n++)
			if(va_arg(arg, char*) == nil)
				break;
		va_end(arg);
	}
	if(precstr){
		prec = yylooksym(g, precstr);
fprint(2, "bad prec %s\n", precstr);
		if(prec == nil)
			return -1;
	}else
		prec = nil;
	right = malloc((n+1)*sizeof right[0]);
	if(right == nil)
		return -1;
	if(right0 == nil)
		right[0] = nil;
	else{
		right[0] = yylooksym(g, right0);
		if(right[0] == nil){
fprint(2, "bad right0 %s\n", right0);
		Free:
			free(right);
			return -1;
		}
		va_start(arg, right0);
		for(i=1; i<n; i++){
			right[i] = yylooksym(g, p=va_arg(arg, char*));
			if(right[i] == nil){
				fprint(2, "bad right%d: %s\n", i, p);
				goto Free;
			}
		}
		va_end(arg);
		right[i] = nil;
	}
	return yyrule(g, fn, left, prec, right, n);
}

int
yyrule(Ygram *g, Yact *fn, Ysym *left, Ysym *prec, Ysym **right, int nright)
{
	int i;
	Yrule *r;
	Yrule **a;
	
	r = mallocz(sizeof *r, 1);
	if(r == nil){
		free(right);
		return -1;
	}
	r->n = g->nrule;
	r->fn = fn;
	r->left = left;
	r->right = right;
	r->nright = nright;
	
	if(prec == nil)
	for(i=0; i<nright; i++){
		if(right[i]->prec){
			prec = right[i];
			break;
		}
	}

	if(prec)
		r->prec = prec->prec+prec->assoc;
	
	if(g->nrule%32 == 0){
		a = realloc(g->rule, (g->nrule+32)*sizeof g->rule[0]);
		if(a == nil){
			free(right);
			free(r);
			return -1;
		}
		g->rule = a;
	}
	if(left->nrule%32 == 0){
		a = realloc(left->rule, (left->nrule+32)*sizeof left->rule[0]);
		if(a == nil){
			free(right);
			free(r);
			return -1;
		}
		left->rule = a;
	}
	g->rule[g->nrule++] = r;
	left->rule[left->nrule++] = r;
	g->dirty = 1;
	return 0;
}

int
yyrulefmt(Fmt *fmt)
{
	int i, dot;
	Yrule *r;

	dot = -1;
	if(fmt->flags&FmtPrec){
		dot = fmt->prec;
		fmt->flags &= ~FmtPrec;
		fmt->prec = 0;
	}
	r = va_arg(fmt->args, Yrule*);
	if(r == nil)
		return fmtprint(fmt, "<nil>");
	fmtprint(fmt, "%s =>", r->left->name);
	for(i=0; i<r->nright; i++){
		if(i == dot)
			fmtprint(fmt, " .");
		fmtprint(fmt, " %s", r->right[i]->name);
	}
	if(i == dot)
		fmtprint(fmt, " .");
	return 0;
}
