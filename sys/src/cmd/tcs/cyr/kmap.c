#include	<u.h>
#include	<libc.h>
#include	<stdio.h>

int cvtab[] = {		/* jeg -> avg map */
	0341, 0342, 0367, 0347, 0344, 0345, 0366, 0372,
	/* A, B, V, G, D, E, ZH, Z */
	0351, 0352, 0353, 0354, 0355, 0356, 0357, 0360,
	/* I, IK, K, L, M, N, O, P */
	0362, 0363, 0364, 0365, 0346, 0350, 0343, 0376,
	/* R, S, T, U, F, H, C, Ch */
	0375, 0373, 0377, 0371, 0370, 0374, 0340, 0361,
	/* Shch, Sh, Tz, Y, Mz, Eh, Ju, Ja */
	0301, 0302, 0327, 0307, 0304, 0305, 0326, 0332,
	/* a, b, v, g, d, e, zh, z */
	0311, 0312, 0313, 0314, 0315, 0316, 0317, 0320,
	/* i, ik, k, l, m, n, o, p */
	0322, 0323, 0324, 0325, 0306, 0310, 0303, 0336,
	/* r, s, t, u, f, h, c, ch */
	0335, 0333, 0337, 0331, 0330, 0334, 0300, 0321
	/* shch, sh, tz, y, mz, eh, ju, ja */
};
int cs3[256];

main()
{
	register c;
	int tab1[256], tab2[256];
	int r, i;

	for(c = 0; c < 128; c++)
		tab1[c] = tab2[c] = c;
	for(c = 128; c < 256; c++)
		tab1[c] = tab2[c] = -1;
	tab1[0xa0] = tab2[0xa0] = 0x0400;

	while(scanf("%x %x", &c, &r) == 2){
		c &= 0xff;
		tab1[c] = r;
		if(c >= 0xb0)
			c = cvtab[c-0xb0];
		tab2[c] = r;
	}
	printf("long tabkio8dep[256] =\n{\n");
	for(c = 0; c < 128; c += 16){
		for(i = 0; i < 16; i++)
			printf("0x%2.2x,", tab1[c+i]);
		printf("\n");
	}
	for(c = 128; c < 256; c += 8){
		for(i = 0; i < 8; i++)
			if(tab1[c+i] < 0)
				printf("    -1,");
			else
				printf("0x%4.4x,", tab1[c+i]);
		printf("\n");
	}
	printf("};\n\n");
	printf("long tabkio8[256] =\n{\n");
	for(c = 0; c < 128; c += 16){
		for(i = 0; i < 16; i++)
			printf("0x%2.2x,", tab2[c+i]);
		printf("\n");
	}
	for(c = 128; c < 256; c += 8){
		for(i = 0; i < 8; i++)
			if(tab2[c+i] < 0)
				printf("    -1,");
			else
				printf("0x%4.4x,", tab2[c+i]);
		printf("\n");
	}
	printf("};\n");
	exits(0);
}
