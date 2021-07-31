#include	<u.h>
#include	<libc.h>

int	table(int n);
int	in, lin;
int	ou, lou;
int	b;
int	lendian;

main(int argc, char *argv[])
{
	int i, osiz, osizb;
	ulong v;

	ARGBEGIN{
	case 'l':
		lendian = 1;
		break;
	default:
		fprint(2, "Usage: %s [-l] in-ldepth out-ldepth\n", argv0);
		exits("usage");
	}ARGEND
	if(argc != 2) {
		fprint(2, "Usage: %s [-l] in-ldepth out-ldepth\n", argv0);
		exits("usage");
	}
	lin = atol(argv[0]);
	lou = atol(argv[1]);
	if(in < 0 || in > 5 || ou < 0 || ou > 5) {
		fprint(2, "ldepths must be in range [0,5]\n");
		exits("ldepth range");
	}
	in = 1<<lin;
	ou = 1<<lou;
	if(ou > in){
		osizb = 0;
		if(ou/in <= 4) {
			b = 8;
			osiz = (2*ou)/in;
		} else {
			b = (32*in)/ou;
			osiz = 8;
		}
	} else {
		if(in/ou > 8) {
			fprint(2, "too big a contraction table\n");
			exits("toobig");
		}
		if(in == ou) {
			fprint(2, "identity\n");
			exits("identity");
		}
		b = 8;
		osiz = lendian? 0 : 1;
		osizb = 8 >> (lin-lou);
	}
	if(lendian)
		print("static ulong ");
	else if(osiz <= 2)
		print("static uchar ");
	else if(osiz <= 4)
		print("static ushort ");
	else
		print("static ulong ");
	print("tab%d%d[%d] =\n{\n", lin, lou, (1<<b));
	for(i=0; i<(1<<b); i++) {
		v = table(i);
		if(lendian){
			v <<= 32 - 4*osiz - osizb;
			print(" 0x%.8ux,", v);
		} else
			print(" 0x%.*ux,", osiz, v);
		if(lendian || osiz==8) {
			if(i%4 == 3)
				print("\n");
		} else {
			if(i%8 == 7)
				print("\n");
		}
	}
	print("};\n");
	exits(0);
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
 * This works even if lendian is true.
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
