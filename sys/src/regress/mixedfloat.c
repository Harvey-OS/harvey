#include <u.h>
#include <libc.h>

#define INT 2
#define FLOAT 2.5
#define A 4 // addition result
#define M 5 // multiplication result

void
main()
{
	int a, b, x, y;
	float f;
	double d;

	f = FLOAT;
	d = FLOAT;
	a = b = x = y = INT;

	a += (double)d;
	b *= (double)d;
	x += (float)f;
	y *= (float)f;

	print("[double] addition: %d; multiplication: %d\n", a, b);

	print("[float] addition: %d; multiplication: %d\n", x, y);

	if(a != A || x != A || b != M || y != M)
		exits("FAIL");
}
