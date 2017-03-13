/*
 * Copyright 2015 - 2017 cinap_lenrek <cinap_lenrek@felloff.net>
 * Copyright 2017 HarveyOS
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include "os.h"
#include <mp.h>
#include "dat.h"

void
mptober(mpint *b, uint8_t *p, int n)
{
	int i, j, m;
	mpdigit x;

	memset(p, 0, n);

	p += n;
	m = b->top*Dbytes;
	if(m < n)
		n = m;

	i = 0;
	while(n >= Dbytes){
		n -= Dbytes;
		x = b->p[i++];
		for(j = 0; j < Dbytes; j++){
			*--p = x;
			x >>= 8;
		}
	}
	if(n > 0){
		x = b->p[i];
		for(j = 0; j < n; j++){
			*--p = x;
			x >>= 8;
		}
	}
}
