#include	<u.h>
#include	<libc.h>
#include	<bio.h>

enum
{
	Nname	= 20,
};

typedef	struct	Name	Name;
struct	Name
{
	char	name[Nname];
	int	count;
	int	value;
};

Name	list[1000];
char	name[Nname];

Name*
lookup(char *s)
{
	Name *n;

	for(n=list; n->name[0]; n++)
		if(strcmp(s, n->name) == 0)
			return n;
	strcpy(n->name, s);
	return n;
}

void
main(int argc, char *argv[])
{
	Biobuf *bin, *bout;
	char *s, *p, *q;
	long lineno;
	Name *n;
	int i, value, swout;;

	ARGBEGIN {
	default:
		fprint(2, "usage: mkstate statefile cfile\n");
		exits(0);
	} ARGEND
	if(argc != 2) {
		fprint(2, "usage: mkstate statefile cfile\n");
		exits("usage");
	}
	bin = Bopen(argv[0], OREAD);
	bout = Bopen(argv[1], OWRITE);
	lineno = 0;
	Bprint(bout, "#line %d \"%s\"\n", 1, argv[0]);
	n = 0;
	swout = 0;
	value = 1;
	for(;;) {
		s = Brdline(bin, '\n');
		if(s == 0)
			break;
		lineno++;
		s[Blinelen(bin)-1] = 0;
		p = strchr(s, '@');
		if(p == 0)
			goto prline;

		if(p[1] == '@') {
			for(n=list; n->name[0]; n++) {
				Bprint(bout, "case %6d: goto A%s;\n", n->value, n->name);
				for(i=1; i<n->count; i++)
					Bprint(bout, "case %6d: goto A%s_%d;\n", n->value+i, n->name, i);
			}
			Bprint(bout, "#line %ld \"%s\"\n%s\n", lineno, argv[0], p+2);
			swout = 1;
			continue;
		}

		if(p[1] == '"') {
			for(n=list; n->name[0]; n++)
				Bprint(bout, "\t\"%s\", %d,\n", n->name, n->value);
			Bprint(bout, "\t0\n");
			Bprint(bout, "#line %ld \"%s\"\n%s\n", lineno, argv[0], p+2);
			swout = 1;
			continue;
		}
		if(p[1] == '\'') {
			for(n=list; n->name[0]; n++)
				Bprint(bout, "\t[%d]	= \"%s\",\n", n->value, n->name);
			Bprint(bout, "#line %ld \"%s\"\n%s\n", lineno, argv[0], p+2);
			swout = 1;
			continue;
		}
		if(p[1] >= '1' && p[1] <= '9' && (p[2] == 'b' || p[2] == 'f')) {
			p[0] = 0;
			i = p[1] - '1';
			if(p[2] == 'b')
				i = -i - 1;
			i += n->count;
			if(i)
				Bprint(bout, "%s%s_%d%s\n", s, name, i, p+3);
			else
				Bprint(bout, "%s%s%s\n", s, name, p+3);
			continue;
		}

		q = strchr(p, ':');
		if(q) {
			if(swout)
				fprint(2, "%ld: cannot create label after switch has been generated\n",
					lineno);
			if(p[1] == ':') {
				n = lookup(name);
				if(n->count == 0)
					fprint(2, "%ld:  no base name\n", lineno);
				goto prbreak;
			}

			i = 0;
			if(n != nil)
				i = n->count;

			memset(name, 0, sizeof(name));
			memcpy(name, p+1, q-p-1);
			n = lookup(name);
			if(n->count)
				fprint(2, "%ld: %s: redefined\n", lineno, name);
			value += i;
			n->value = value;

		prbreak:
			if(n->count) {
				Bprint(bout, "goto B%s_%d; B%s_%d: x->state = %d; goto out; A%s_%d: x->state = %d;\n",
					name, n->count, name, n->count,
					n->value+n->count, name, n->count, n->value);
			} else {
				Bprint(bout, "goto B%s; B%s: x->state = %d; goto out; A%s: x->state = %d;\n",
					name, name, n->value, name, n->value);
			}
			n->count++;
			Bprint(bout, "#line %ld \"%s\"\n%s\n", lineno, argv[0], q+1);
			continue;
		}

	prline:
		Bprint(bout, "%s\n", s);
	}
	exits(0);
}
