#include <alef.h>

intern int
nextarg(Arg *p)
{
	byte *bp;

	if(p->ac > 0){
		bp = *p->av;
		if(bp != nil && *bp == '-'){
			p->p = bp+1;
			p->av++;
			p->ac--;
			return 1;
		}
	}
	p->p = nil;
	return 0;
}

Arg*
arginit(int ac, byte **av)
{
	Arg *p;

	alloc p;
	check p != nil;
	p->arg0 = *av;
	p->ac = ac - 1;
	p->av = av + 1;
	nextarg(p);
	return p;
}

Rune
argopt(Arg *p)
{
	int n;
	Rune r;

	if(p->p == nil)
		return 0;
	if(*p->p == 0 && nextarg(p) == 0)
		return 0;
	n = chartorune(&r, p->p);
	p->p += n;
	return r;
}

byte*
argf(Arg *p)
{
	byte *bp;

	if(p->p != nil && *p->p){
		bp = p->p;
		p->p = "";
	} else if(p->ac > 0){
		p->ac--;
		bp = *p->av++;
	} else
		bp = nil;
	return bp;
}
