#include <u.h>
#include <libc.h>

typedef struct QR QR;
struct QR {
	vlong	quo;
	vlong	rem;
};

QR	divrem64(vlong, vlong);

vlong
f(vlong a, vlong b)
{
	vlong q;

	q = divrem64(a, b).quo;
	return q;
}
