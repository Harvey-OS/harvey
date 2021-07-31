#include "sam.h"
#include "parse.h"

Address	addr;
String	lastpat;
int	patset;
File	*menu;

File	*matchfile(String*);
Address	charaddr(Posn, Address, int);

Address
address(Addr *ap, Address a, int sign)
{
	File *f = a.f;
	Address a1, a2;

	do{
		switch(ap->type){
		case 'l':
		case '#':
			a = (*(ap->type=='#'?charaddr:lineaddr))(ap->num, a, sign);
			break;

		case '.':
			a = f->dot;
			break;

		case '$':
			a.r.p1 = a.r.p2 = f->nrunes;
			break;

		case '\'':
			a.r = f->mark;
			break;

		case '?':
			sign = -sign;
			if(sign == 0)
				sign = -1;
			/* fall through */
		case '/':
			nextmatch(f, ap->are, sign>=0? a.r.p2 : a.r.p1, sign);
			a.r = sel.p[0];
			break;

		case '"':
			a = matchfile(ap->are)->dot;
			f = a.f;
			if(f->state == Unread)
				load(f);
			break;

		case '*':
			a.r.p1 = 0, a.r.p2 = f->nrunes;
			return a;

		case ',':
		case ';':
			if(ap->left)
				a1 = address(ap->left, a, 0);
			else
				a1.f = a.f, a1.r.p1 = a1.r.p2 = 0;
			if(ap->type == ';'){
				f = a1.f;
				f->dot = a = a1;
			}
			if(ap->next)
				a2 = address(ap->next, a, 0);
			else
				a2.f = a.f, a2.r.p1 = a2.r.p2 = f->nrunes;
			if(a1.f != a2.f)
				error(Eorder);
			a.f = a1.f, a.r.p1 = a1.r.p1, a.r.p2 = a2.r.p2;
			if(a.r.p2 < a.r.p1)
				error(Eorder);
			return a;

		case '+':
		case '-':
			sign = 1;
			if(ap->type == '-')
				sign = -1;
			if(ap->next==0 || ap->next->type=='+' || ap->next->type=='-')
				a = lineaddr(1L, a, sign);
			break;
		default:
			panic("address");
			return a;
		}
	}while(ap = ap->next);	/* assign = */
	return a;
}

void
nextmatch(File *f, String *r, Posn p, int sign)
{
	compile(r);
	if(sign >= 0){
		if(!execute(f, p, INFINITY))
			error(Esearch);
		if(sel.p[0].p1==sel.p[0].p2 && sel.p[0].p1==p){
			if(++p>f->nrunes)
				p = 0;
			if(!execute(f, p, INFINITY))
				panic("address");
		}
	}else{
		if(!bexecute(f, p))
			error(Esearch);
		if(sel.p[0].p1==sel.p[0].p2 && sel.p[0].p2==p){
			if(--p<0)
				p = f->nrunes;
			if(!bexecute(f, p))
				panic("address");
		}
	}
}

File *
matchfile(String *r)
{
	File *f;
	File *match = 0;
	int i;

	for(i = 0; i<file.nused; i++){
		f = file.filepptr[i];
		if(f == cmd)
			continue;
		if(filematch(f, r)){
			if(match)
				error(Emanyfiles);
			match = f;
		}
	}
	if(!match)
		error(Efsearch);
	return match;
}

int
filematch(File *f, String *r)
{
	char *c, buf[STRSIZE+100];
	String *t;

	c = Strtoc(&f->name);
	sprint(buf, "%c%c%c %s\n", " '"[f->state==Dirty],
		"-+"[f->rasp!=0], " ."[f==curfile], c);
	free(c);
	t = tmpcstr(buf);
	Strduplstr(&genstr, t);
	freetmpstr(t);
	/* A little dirty... */
	if(menu == 0)
		(menu=Fopen())->state=Clean;
	Bdelete(menu->buf, 0, menu->buf->nrunes);
	Binsert(menu->buf, &genstr, 0);
	menu->nrunes = menu->buf->nrunes;
	compile(r);
	return execute(menu, 0, menu->nrunes);
}

Address
charaddr(Posn l, Address addr, int sign)
{
	if(sign == 0)
		addr.r.p1 = addr.r.p2 = l;
	else if(sign < 0)
		addr.r.p2 = addr.r.p1-=l;
	else if(sign > 0)
		addr.r.p1 = addr.r.p2+=l;
	if(addr.r.p1<0 || addr.r.p2>addr.f->nrunes)
		error(Erange);
	return addr;
}

Address
lineaddr(Posn l, Address addr, int sign)
{
	int n;
	int c;
	File *f = addr.f;
	Address a;

	SET(c);
	a.f = f;
	if(sign >= 0){
		if(l == 0){
			if(sign==0 || addr.r.p2==0){
				a.r.p1 = a.r.p2 = 0;
				return a;
			}
			a.r.p1 = addr.r.p2;
			Fgetcset(f, addr.r.p2-1);
		}else{
			if(sign==0 || addr.r.p2==0){
				Fgetcset(f, (Posn)0);
				n = 1;
			}else{
				Fgetcset(f, addr.r.p2-1);
				n = Fgetc(f)=='\n';
			}
			for(; n<l; ){
				c = Fgetc(f);
				if(c == -1)
					error(Erange);
				else if(c == '\n')
					n++;
			}
			a.r.p1 = f->getcp;
		}
		do; while((c=Fgetc(f))!='\n' && c!=-1);
		a.r.p2 = f->getcp;
	}else{
		Fbgetcset(f, addr.r.p1);
		if(l == 0)
			a.r.p2 = addr.r.p1;
		else{
			for(n = 0; n<l; ){	/* always runs once */
				c = Fbgetc(f);
				if(c == '\n')
					n++;
				else if(c == -1){
					if(++n != l)
						error(Erange);
				}
			}
			a.r.p2 = f->getcp;
			if(c == '\n')
				a.r.p2++;	/* lines start after a newline */
		}
		do; while((c=Fbgetc(f))!='\n' && c!=-1);
		a.r.p1 = f->getcp;
		if(c == '\n')
			a.r.p1++;	/* lines start after a newline */
	}
	return a;
}
