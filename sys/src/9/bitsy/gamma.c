#include <u.h>
#include <libc.h>
#include <stdio.h>

double	gamma = 1.6;

int
remap5(int i)
{
	double v;

	v = (double)i/31.0;
	return 31.0*pow(v, gamma);
}

int
remap6(int i)
{
	double v;

	v = (double)i/63.0;
	return 63.0*pow(v, gamma);
}

int
remap(int i)
{
	int r, g, b;

	b = i & 0x1F;
	g = (i>>5) & 0x3F;
	r = (i>>11) & 0x1F;
	return (remap5(r)<<11) | (remap6(g)<<5) | (remap5(b)<<0);
}

void
main(void)
{
	int i;

	printf("/* gamma = %.2f */\n", gamma);
	printf("ushort gamma[65536] = {\n");
	for(i=0; i<65536; i++){
		if((i%8) == 0)
			printf("\t");
		printf("0x%.4x, ", remap(i));
		if((i%8) == 7)
			printf("\n");
	}
	printf("};\n");
}
