#include	<u.h>
#include	<libc.h>

int	table(int n);
int	in, lin;
int	ou, lou;
int	b;

main(int argc, char *argv[])
{
	int i, osiz;

	if(argc != 3) {
		print("Usage: in-ldepth out-ldepth\n");
		exit(1);
	}
	lin = atol(argv[1]);
	lou = atol(argv[2]);
	if(in < 0 || in > 5 || ou < 0 || ou > 5) {
		print("ldepths must be in range [0,5]\n");
		exit(1);
	}
	in = 1<<lin;
	ou = 1<<lou;
	if(ou > in){
		if(ou/in <= 4) {
			b = 8;
			osiz = (2*ou)/in;
		} else {
			b = (32*in)/ou;
			osiz = 8;
		}
	} else {
		if(in/ou > 8) {
			print("too big a contraction table\n");
			exit(1);
		}
		if(in == ou) {
			print("identity\n");
			exit(1);
		}
		b = 8;
		osiz = 1;
	}
	if(osiz <= 2)
		print("static uchar ");
	else if(osiz <= 4)
		print("static ushort ");
	else
		print("static ulong ");
	print("tab%d%d[%d] =\n{\n", lin, lou, (1<<b));
	for(i=0; i<(1<<b); i++) {
		print(" 0x%.*ux,", osiz, table(i));
		if(i%8 == 7)
			print("\n");
	}
	print("};\n");
	exit(0);
}

/*
 * Byte n of size b bits contains pixels with in bits/pixel.
 * Convert each pixel to a pixel with ou bits/pixel,
 * and assemble into an int.
 * When expanding, a 0 pixel goes to 0, an all 1's pixel
 * goes to all 1's, and the other input pixels equally divide
 * the output pixel maximum.  (E.g., in=2, ou=8: 0->0, 1->255/3,
 * 2-> 255/3 * 2, 3-> 255).  You get this effect by just
 * duplicating the input pixel as many times as necessary.
 */
int
table(int n)
{
	int i, j, x, t;

	x = 0;
	for(i=0; i<b/in; i++) {
		t = n >> (b-in);
		n <<= in;
		t &= (1<<in) - 1;
		x <<= ou;
		if(ou > in) {
			for(j=0; j<ou/in; j++) {
				x |= t;
				t <<= in;
			}
		} else {
			x |= t >> (in-ou);
		}
	}
	return x;
}
