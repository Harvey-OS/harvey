From ulysses!grass Mon Feb 24 12:59:25 EST 1992
To: research!andrew
Subject: jeg-avg.c

#include <stdio.h>

static char cvtab[] = {
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

main()
{
	register c;

	while( (c = getchar()) != EOF ) {
		/*c &= 0xff; */
		if( c >= 0260 )
			c = cvtab[c-0260];
		putchar(c);
	}
	exit(0);
}

