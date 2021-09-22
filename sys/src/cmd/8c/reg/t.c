struct a {
	long	v;
};

#pragma hjdicks on
struct b {
	long	v;
	char	x;
};
#pragma hjdicks off


struct aa {
	long	v[2];
};

struct aaa {
	long	v[4];
};

struct a av;
struct b bv;
struct aa aav;
struct aaa aaav;

void
f(void)
{
	av = av;
	bv = bv;
	aav = aav;
	aaav = aaav;
}
