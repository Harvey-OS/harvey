#include	<alef.h>
typedef Set;
aggr Panel
{
	Panel *next;
	byte *name;
};

aggr Tname
{
	byte* v;
};

Set
Name(byte* val)
{
	Tname ret;
	ret.v=val;
	return (alloc Set)ret;
}

void
set(Panel *p, Set a)
{
	typeof a {
	case Tname:
		p->name=a.v;
		print("name\n");
	}
}

void main(void)
{
	Panel p;
	set(&p, Name("howdy"));
}
