typedef unsigned long ulong;
enum
{
	Tnleaf,
};
short *talloc, *tend;

void print(char *s, ...)
{
}

void myexit(char*s)
{
}

typedef	struct	Code	Code;
struct	Code
{
	int	val;
	ulong	count;
	Code*	left;
	Code*	right;
};

static short*
copytree(Code *p)
{
	ulong off;

	if(p->left) {
		if(off >= Tnleaf){
			print("tree offset too large %lux\n", off);
		}
	}
}
