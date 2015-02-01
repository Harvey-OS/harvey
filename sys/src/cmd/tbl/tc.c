/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

/* tc.c: find character not in table to delimit fields */
# include "t.h"

# define COMMON "\002\003\005\006\007!%&#/?,:;<=>@`^~_{}+-*" \
	"ABCDEFGHIJKMNOPQRSTUVWXZabcdefgjkoqrstwxyz"

void
choochar(void)
{
				/* choose funny characters to delimit fields */
	int	had[128], ilin, icol, k;
	char	*s;

	for (icol = 0; icol < 128; icol++)
		had[icol] = 0;
	F1 = F2 = 0;
	for (ilin = 0; ilin < nlin; ilin++) {
		if (instead[ilin] || fullbot[ilin])
			continue;
		for (icol = 0; icol < ncol; icol++) {
			k = ctype(ilin, icol);
			if (k == 0 || k == '-' || k == '=')
				continue;
			s = table[ilin][icol].col;
			if (point(s))
				for (; *s; s++)
					if((unsigned char)*s < 128)
						had[(unsigned char)*s] = 1;
			s = table[ilin][icol].rcol;
			if (point(s))
				for (; *s; s++)
					if((unsigned char)*s < 128)
						had[(unsigned char)*s] = 1;
		}
	}
				/* choose first funny character */
	for (s = COMMON "Y"; *s; s++) {
		if (had[*s] == 0) {
			F1 = *s;
			had[F1] = 1;
			break;
		}
	}
				/* choose second funny character */
	for (s = COMMON "u"; *s; s++) {
		if (had[*s] == 0) {
			F2 = *s;
			break;
		}
	}
	if (F1 == 0 || F2 == 0)
		error("couldn't find characters to use for delimiters");
}

int
point(char *ss)
{
	vlong s = (uintptr)ss;

	return(s >= 128 || s < 0);
}
