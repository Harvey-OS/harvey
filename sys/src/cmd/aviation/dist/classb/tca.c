#include "tca.h"

void
main(int argc, char *argv[])
{
	char *file;

	header = "X17";
	file = "nyc";
	outline = 0;
	grain = 6076./1000;

	ARGBEGIN {
	default:
		fprint(2, "usage: tca [-a 123] ifile\n");
		exits("usage");
	case 'o':
		outline = 1;
		break;
	case 'g':
		grain = 6076./atol(ARGF());
		break;
	} ARGEND

	if(argc > 0)
		file = argv[0];
	bin = Bopen(file, OREAD);
	if(bin == 0) {
		yyerror("cannot open %s\n", file);
		exits(0);
	}
	lineno = 1;
	yyparse();
	exits(0);
}

int
yylex(void)
{

loop:
	lastchar = Bgetrune(bin);
	if(lastchar == '#')
		for(;;) {
			lastchar = Bgetrune(bin);
			if(lastchar < 0 || lastchar == '\n')
				break;
		}
	if(lastchar == '\n') {
		lineno++;
		lastchar = ';';
	}
	if(lastchar == ' ' || lastchar == '\t')
		goto loop;
	return lastchar;
}

void
yyerror(char *a, ...)
{
	char buf[100];
	va_list arg;

	va_start(arg, a);
	doprint(buf, buf+sizeof(buf), a, arg);
	va_end(arg);
	fprint(2, "line %d: (%C) %s\n", lineno, lastchar, buf);
	nerror++;
	if(nerror >= 10)
		exits("error");
}

Node*
new(int op)
{
	Node *n;

	n = malloc(sizeof(Node));
	memset(n, 0, sizeof(Node));
	n->op = op;
	return n;
}

Node*
lookupp(int name)
{
	Node *n;

	for(n=basep; n; n=n->nright)
		if(n->op == name)
			return n->nleft;
	yyerror("line %ld: name not found: %c\n", lineno, name);
	return 0;
}

double
lookupv(int name)
{
	Node *n;

	for(n=basev; n; n=n->nright)
		if(n->op == name)
			return n->vleft;
	yyerror("line %ld: name not found: %c\n", lineno, name);
	return 0;
}

void
definep(int name, Node *def)
{
	Node *n, *t;

	for(n=basep; n; n=n->nright)
		if(n->op == name) {
			yyerror("line %ld: name alreeady defined: %c\n", lineno, name);
			return;
		}
	n = eval(def);
	if(n == 0)
		return;
	t = basep;
	basep = new(name);
	basep->nleft = n;
	basep->nright = t;
}

void
definev(int name, double def)
{
	Node *n, *t;

	for(n=basev; n; n=n->nright)
		if(n->op == name) {
			yyerror("line %ld: name alreeady defined: %c\n", lineno, name);
			return;
		}
	t = basev;
	basev = new(name);
	basev->vleft = def;
	basev->nright = t;
}

Node*
eval(Node *n)
{
	Node *t;

	if(n == 0) {
		yyerror("eval null\n");
		return 0;
	}
	switch(n->op) {
	case Opoint:
		return n;
	case Oxcc:
		t = xcc(n);
		return t;
	case Oxcl:
		t = xcl(n);
		return t;
	case Oxll:
		t = xll(n);
		return t;
	case Odme:
		t = eval(n->nleft);
		t = dme(t, n->nright->vleft, n->nright->vright);
		return t;
	}
	yyerror("unknown op in eval %d\n", n->op);
	return 0;
}
