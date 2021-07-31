/*
prototype scope for structure tags, enum values, typedefs:
*/
enum
{
	AA,
	BB
};
static
int
f(enum {BB, AA} n)
{
	return BB;
}

typedef void D;
void
f(double D)
{
}
