#include <u.h>
#include <libc.h>
#include	<cda/fizz.h>
void f_major(char *, ...);
void f_minor(char *, ...);
/*
	if you add a field, add a check to the dup test `DUP'
*/
static Keymap keys[] = {
	"name", NULLFN, 1,
	"pkg", NULLFN, 2,
	"tt", NULLFN, 3,
	"code", NULLFN, 4,
	"value", NULLFN, 5,
	"part", NULLFN, 6,
	"comment", NULLFN, 7,
	"family", NULLFN, 8,
	0
};

void
f_type(char *s)
{
	int loop;
	Type *t, *tt;

	BLANK(s);
	if(loop = *s == '{')
		s = f_inline();
	t = NEW(Type);
	t->tt = f_strdup("");
	t->comment = "";
	do {
		if(*s == '}') break;
		switch((int)f_keymap(keys, &s))
		{
		case 1:
			t->name = f_strdup(s);
			break;
		case 2:
			t->pkgname = f_strdup(s);
			break;
		case 3:
			t->tt = f_strdup(s);
			break;
		case 4:
			t->code = f_strdup(s);
			break;
		case 5:
			t->value = f_strdup(s);
			break;
		case 6:
			t->part = f_strdup(s);
			break;
		case 7:
			t->comment = f_strdup(s);
			break;
		case 8:
			t->family = f_strdup(s);
			break;
		default:
			f_major("Type: unknown field: '%s'", s);
			break;
		}
	} while(loop && (s = f_inline()));
	if(t->name == 0){
		f_major("type with no name");
		return;
	}
	if(tt = (Type *)symlook(t->name, S_TYPE, (void *)0)){ /* test for DUP */
#define	CHK(x)	if(t->x)if(tt->x)f_minor("type %s: dup field x", t->name);else tt->x = t->x
#define	TCHK(x)	if(*t->x)if(*tt->x)f_minor("type %s: dup field x", t->name);else tt->x = t->x
		/* not name*/ CHK(pkgname); TCHK(tt);
		CHK(code); CHK(value); CHK(part);
	} else
		(void)symlook(t->name, S_TYPE, (void *)t);
}

void
chktype(Type *t)
{
	if(t->pkgname == 0){
		f_major("type %s has no package", t->name);
		return;
	}
	t->pkg = (Package *)symlook(t->pkgname, S_PACKAGE, (void *)0);
	if(t->pkg == 0){
		f_major("type %s refers to nonexistent package %s", t->name, t->pkgname);
		return;
	}
	if(t->tt)
		if(strlen(t->tt) != t->pkg->npins)
			f_major("type %s has %d `tt' pins; package %s has %d pins",
				t->name, strlen(t->tt), t->pkg->name, t->pkg->npins);
}
