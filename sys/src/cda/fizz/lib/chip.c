#include <u.h>
#include <libc.h>
#include	<cda/fizz.h>

void f_minor(char *, ...);
void f_major(char *, ...);

static Keymap keys[] = {
	"name", NULLFN, 1,
	"type", NULLFN, 2,
	"comment", NULLFN, 3,
	0
};
void symdel(char *, int);
int f_nunplaced;
char *ttnames = "gvwxyz";

void
f_chip(char *s)
{
	int loop;
	Chip *cc = 0;
	char *typename = 0;
	char *comment = 0;

	BLANK(s);
	if(loop = *s == '{')
		s = f_inline();
	do {
		if(*s == '}') break;
		switch((int)f_keymap(keys, &s))
		{
		case 1:
			if((cc = (Chip *)symlook(s, S_CHIP, (void *)0)) == 0){
				cc = NEW(Chip);
				cc->name = f_strdup(s);
				cc->flags |= UNPLACED;
				(void)symlook(cc->name, S_CHIP, (void *)cc);
				if(typename)
					cc->typename = typename;
				if(comment)
					cc->comment = comment;
			}
			break;
		case 2:
			if(cc)
				cc->typename = f_strdup(s);
			else
				typename = f_strdup(s);
			break;
		case 3:
			if(cc)
				cc->comment = f_strdup(s);
			else
				comment = f_strdup(s);
			break;
		default:
			f_minor("Chip: unknown field: '%s'\n", s);
			break;
		}
	} while(loop && (s = f_inline()));
}

void
chkchip(Chip *c)
{
	register char *s, *ss;
	int p;

	if(c->typename == 0){
		fprint(2, "Chip %s: no type specified, deleted\n", c->name);
		symdel(c->name, S_CHIP);
		return;
	}
	c->type = (Type *)symlook(c->typename, S_TYPE, (void *)0);
	if(c->type == 0){
		f_major("Chip %s: type %s undefined", c->name, c->typename);
		return;
	}
	c->npins = c->type->pkg->npins;
	c->ndrills = c->type->pkg->ndrills;
	c->pmin = c->type->pkg->pmin;
	c->netted = (char *)lmalloc((long)c->npins);
	memset(c->netted, NOTNETTED, c->npins);
	c->pins = (Pin *)lmalloc(c->npins*(long)sizeof(Pin));
	c->drills = (Pin *)lmalloc(c->ndrills*(long)sizeof(Pin));
	for(s = c->type->tt; *s; s++){
		char ttc;

		if(isupper(ttc = *s))
			ttc = tolower(ttc);
		if(ss = strchr(ttnames, ttc)){
			int specsig = ss-ttnames;

			ttadd(specsig, c->name, p = c->pmin+(int)(s-c->type->tt));
			c->netted[p-c->pmin] = VBNET|specsig;
		}
	}
	if(c->flags&UNPLACED)
		unplace(c);
	else
		place(c);
	if(c->flags&UNPLACED)
		f_nunplaced++; 
}
