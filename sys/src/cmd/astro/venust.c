/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#include "astro.h"

double	venfp[]	=
{
	4.889,		2.0788,
	11.261,		2.5870,
	7.128,		6.2384,
	3.446,		2.3721,
	1.034,		0.4632,
	1.575,		3.3847,
	1.439,		2.4099,
	1.208,		4.1464,
	2.966,		3.6318,
	1.563,		4.6829,
	0.,
	0.122,		4.2726,
	0.300,		0.0218,
	0.159,		1.3491,
	0.,
	2.246e-6,	0.5080,
	9.772e-6,	1.0159,
	8.271e-6,	4.6674,
	0.737e-6,	0.8267,
	1.426e-6,	5.1747,
	0.510e-6,	5.7009,
	1.572e-6,	1.8188,
	0.717e-6,	2.2969,
	2.991e-6,	2.0611,
	1.335e-6,	0.9628,
	0.,
};

char	vencp[]	=
{
	1,-1,0,0,
	2,-2,0,0,
	3,-3,0,0,
	2,-3,0,0,
	4,-4,0,0,
	4,-5,0,0,
	3,-5,0,0,
	1,0,-3,0,
	1,0,0,-1,
	0,0,0,-1,

	0,-1,0,
	4,-5,0,
	1,0,-2,

	1,-1,0,0,
	2,-2,0,0,
	3,-3,0,0,
	2,-3,0,0,
	4,-4,0,0,
	5,-5,0,0,
	4,-5,0,0,
	2,0,-3,0,
	1,0,0,-1,
	2,0,0,-2,
};
