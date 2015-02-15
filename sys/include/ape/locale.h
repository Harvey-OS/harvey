/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#ifndef __LOCALE
#define __LOCALE
#pragma lib "/$M/lib/ape/libap.a"

#include <stddef.h>

#define LC_ALL		0
#define LC_COLLATE	1
#define LC_CTYPE	2
#define LC_MONETARY	3
#define LC_NUMERIC	4
#define LC_TIME		5

struct lconv {
	int8_t *decimal_point;
	int8_t *thousands_sep;
	int8_t *grouping;
	int8_t *int_curr_symbol;
	int8_t *currency_symbol;
	int8_t *mon_decimal_point;
	int8_t *mon_thousands_sep;
	int8_t *mon_grouping;
	int8_t *positive_sign;
	int8_t *negative_sign;
	int8_t int_frac_digits;
	int8_t frac_digits;
	int8_t p_cs_precedes;
	int8_t p_sep_by_space;
	int8_t n_cs_precedes;
	int8_t n_sep_by_space;
	int8_t p_sign_posn;
	int8_t n_sign_posn;
};

#ifdef __cplusplus
extern "C" {
#endif

extern int8_t *setlocale(int, const int8_t *);
extern struct lconv *localeconv(void);

#ifdef __cplusplus
}
#endif

#endif /* __LOCALE */
