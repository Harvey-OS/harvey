#include	<alef.h>

enum {
	N = 4,
};

typedef float Matrix[N][N];

void
mul(Matrix r, Matrix a, Matrix b)
{
	int i, j, k;

	r[0::N][0::N] = 0;
	r[i=0::N][j=0::N] += a[i][k=0::N]*b[k][j];
}

void
main(void)
{
	int i;
	Matrix a, b, c;

	a[i=0::N][0::N] = i;
	b[0::N][i=0::N] = i;

	mul(c, a, b);

	for(i = 0; i < N; i++) {
		print(" %f ", c[i][0::N]);
		print("\n");
	}
}
