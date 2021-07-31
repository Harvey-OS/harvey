#include <u.h>
#include <libc.h>
#include <stdio.h>
#include	<cda/fizz.h>

#define	islower(c)	(((c) >= 'a') && ((c) <= 'z'))
#define isalpha(c)	((islower(c)) || (isupper(c)))

extern char * comment;
extern int f_iline;
extern FILE *ifp;
char *f_ifname;
extern int f_nerrs;
int c_crack(char *);
Type * do_ttline(char *, Type *);
Type * do_fline(char *, Type *);
Type * do_tline(char *);
void do_signal(char *, Chip * );
Chip * do_chip(char *);
void f_major(char *, ...);
void f_minor(char *, ...);
void out(void);

int nonames = 0, nocomments = 0, nofamilies = 0;

typedef struct Netpoint {
	char * chip;
	short pinno;
	char * pinname;
	struct Netpoint * next;
	} Netpoint;

main(int argc, char **argv)
{
	argc--;
	argv++;
	while(argc) {
		if(**argv == '-') switch(argv[0][1]) {
			case 'n':
				nonames++;
				break;
			case 'c':
				nocomments++;
				break;
			case 'f':
				nofamilies++;
				break;
			default:
				break;
		}
		else if(c_crack(argv[0])) {
				exit(1);
		}
		argc--;
		argv++;
	}
	out();
	exit(0);
}

int
c_crack(char *file)
{
	char *s;
	Chip *chip = (Chip *) 0;
	Type *type = (Type *) 0;
	if((ifp = fopen(file, "r")) == 0){
		perror(file);
		return(1);
	}
	f_iline = 0;
	f_ifname = file;
	while(s = f_inline()){
		if(*s != '.') {
			if(chip) do_signal(s, chip);
			continue;
		}
		++s;
		switch(*s++) {
		case 't':
			chip = (Chip *) 0;
			if(*s == 't') type = do_ttline(++s, type);
			else if(*s == 'T') type = do_ttline(++s, type);
			else if(*s == 'f') type = do_fline(++s, type);
			else type = do_tline(s);
			break;
		case 'c':
			type = (Type *) 0;
			chip = do_chip(s);
			break;
		case 'f':
			type = (Type *) 0;
			chip = (Chip *) 0;
			break;
		case 'm':
			break;
		default:
			type = (Type *) 0;
			chip = (Chip *) 0;
			f_major("bad format %s\n", s);
			exit(1);
		}
	}
	fclose(ifp);
	return(f_nerrs);
}

Type *
do_ttline(char *s, Type *tp)
{
	char *q, *ss;
	if(tp == (Type*) 0) return((Type *) 0);
	BLANK(s);
	ss = q = s;
	while(*s) {
		if(ISBL(*s)) s++;
		else *ss++ = *s++;
	}
	*ss = 0;
	tp->tt = f_strdup(q);
	return(tp);
}
	
Type *
do_fline(char *s, Type *tp)
{
	char *q, *ss;
	if(tp == (Type*) 0) return((Type *) 0);
	BLANK(s);
	ss = q = s;
	while(*s) {
		if(ISBL(*s)) s++;
		else *ss++ = *s++;
	}
	*ss = 0;
	tp->family = f_strdup(q);
	return(tp);
}
	
Type *
do_tline(char *s)
{
	char *q, *np;
	int i, l;
	Type * tp;
	BLANK(s);
	q = s;
	NAME(s);
	if((tp = (Type *)symlook(q, S_TYPE, (void *)0)) == 0){
		tp = NEW(Type);
		tp->name = f_strdup(q);
		tp->tt = "";
		(void)symlook(tp->name, S_TYPE, (void *)tp);
	}
	q = s;
	NAME(s);
	tp->pkgname = f_strdup(q);
	tp->comment = f_strdup(comment);
	if (tp->name[0] == '7' && tp->name[1] == '4') {
		np = &tp->name[2];
		while (isalpha(*np)) np++;
		l = np-&tp->name[2];
		tp->family = f_malloc(l+1);
		for(i=0; i<l; i++) tp->family[i] = islower(tp->name[2+i]) ?
			toupper(tp->name[2+i]): tp->name[2+i];
		tp->family[l] = '\0';
	}
	return(tp);
}
	

void
do_signal(char *s, Chip * cc)
{
	char *q;
	int pinno, nm;
	Signal *ss;
	Netpoint *np;
	BLANK(s);
	q = s;
	NAME(s);
	if((ss = (Signal *) symlook(q, S_SIGNAL, (void *)0)) == 0){
		ss = NEW(Signal);
		ss->name = f_strdup(q);
		ss->coords = (Coord * ) 0;
		ss->n = 0;
		(void)symlook(ss->name, S_SIGNAL, (void *)ss);
	}
	q = s;
	NUM(s, pinno);
	np = NEW(Netpoint);
	np->chip = cc->name;
	np->pinno = pinno;
	q = s;
	if(*s) {
		NAME(s);
		np->pinname = f_strdup(q);
	}
	else np->pinname = "";
	np->next = (Netpoint *) ss->coords;
	ss->coords = (Coord *) np;
	ss->n += 1;
}

Chip *
do_chip(char *s)
{
	char *q;
	Chip *cc;
	Type *tp;
	BLANK(s);
	q = s;
	NAME(s);
	if((cc = (Chip *)symlook(q, S_CHIP, (void *)0)) == 0){
		cc = NEW(Chip);
		cc->name = f_strdup(q);
		cc->typename = "";
		(void)symlook(cc->name, S_CHIP, (void *)cc);
	}
	q = s;
	NAME(s);
	if((tp = (Type *)symlook(q, S_TYPE, (void *)0)) == 0){
		tp = NEW(Type);
		tp->name = f_strdup(q);
		tp->tt = "";
		tp->comment = "";
		tp->pkgname = "";
		tp->family = "";
		(void)symlook(tp->name, S_TYPE, (void *)tp);
	}
	cc->type = tp;
	cc->typename = tp->name;
	return(cc);
}

void
outtypes(Type *tp)
{
	fprint(1, "Type{\n\tname %s\n\tpkg %s\n\ttt %s\n",
		tp->name, tp->pkgname, tp->tt);
	if(!nofamilies && tp->family) fprint(1, "\tfamily %s\n", tp->family);
	if(*(tp->comment) && !nocomments) fprint(1, "\tcomment %s\n", tp->comment);
	fprint(1, "}\n");
}

void
outchips(Chip *cc)
{
	fprint(1, "Chip{\n\tname %s\n\ttype %s\n}\n",
		cc->name, cc->typename);
}

void
outsigs(Signal *ss)
{
	Netpoint *np;
	fprint(1, "Net %s %d{\n",
		ss->name, ss->n);
	for(np = (Netpoint *) ss->coords; np; np = np->next) {
		fprint(1, "\t%s %d", np->chip, np->pinno);
		if(*(np->pinname) && !nonames) fprint(1, " %s\n", np->pinname);
		else fprint(1, "\n");
	}
	fprint(1, "}\n"); 
}



void
out(void) 
{
		symtraverse(S_TYPE, outtypes);
		symtraverse(S_CHIP, outchips);
		symtraverse(S_SIGNAL, outsigs);
}
