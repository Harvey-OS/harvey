/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#include	<locale.h>
#include	<limits.h>
#include	<string.h>

static struct lconv Clocale = {
	".",		/* decimal_point */
	"",		/* thousands_sep */
	"",		/* grouping */
	"",		/* int_curr_symbol */
	"",		/* currency_symbol */
	"",		/* mon_decimal_point */
	"",		/* mon_thousands_sep */
	"",		/* mon_grouping */
	"",		/* positive_sign */
	"",		/* negative_sign */
	CHAR_MAX,	/* int_frac_digits */
	CHAR_MAX,	/* frac_digits */
	CHAR_MAX,	/* p_cs_precedes */
	CHAR_MAX,	/* p_sep_by_space */
	CHAR_MAX,	/* n_cs_precedes */
	CHAR_MAX,	/* n_sep_by_space */
	CHAR_MAX,	/* p_sign_posn */
	CHAR_MAX,	/* n_sign_posn */
};

static char *localename[2] = {"C", ""};
static short catlocale[6] = {0, 0, 0, 0, 0, 0};
	/* indices into localename  for categories LC_ALL, LC_COLLATE, etc. */

#define ASIZE(a) (sizeof(a)/sizeof(a[0]))

char *
setlocale(int category, const char *locale)
{
	int c, i;

	if(category < 0 || category >= ASIZE(catlocale))
		return 0;
	if(!locale)
		return localename[catlocale[category]];
	for(c=0; c<ASIZE(localename); c++)
		if(strcmp(locale, localename[c]) == 0)
			break;
	if(c >= ASIZE(localename))
		return 0;
	catlocale[category] = c;
	if(category == LC_ALL)
		for(i=0; i<ASIZE(catlocale); i++)
			catlocale[i] = c;
	return localename[c];
}

struct lconv *
localeconv(void)
{
	/* BUG: posix says look at environment variables
         * to set locale "", but we just make it the same
	 * as C, always.
	 */
	return &Clocale;
}

