/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#include	"mk.h"

static int8_t *vexpand(int8_t*, Envy*, Bufblock*);
static int8_t *shquote(int8_t*, Rune, Bufblock*);
static int8_t *shbquote(int8_t*, Bufblock*);

void
shprint(int8_t *s, Envy *env, Bufblock *buf)
{
	int n;
	Rune r;

	while(*s) {
		n = chartorune(&r, s);
		if (r == '$')
			s = vexpand(s, env, buf);
		else {
			rinsert(buf, r);
			s += n;
			s = copyq(s, r, buf);	/*handle quoted strings*/
		}
	}
	insert(buf, 0);
}

static int8_t *
mygetenv(int8_t *name, Envy *env)
{
	if (!env)
		return 0;
	if (symlook(name, S_WESET, 0) == 0 && symlook(name, S_INTERNAL, 0) == 0)
		return 0;
		/* only resolve internal variables and variables we've set */
	for(; env->name; env++){
		if (strcmp(env->name, name) == 0)
			return wtos(env->values, ' ');
	}
	return 0;
}

static int8_t *
vexpand(int8_t *w, Envy *env, Bufblock *buf)
{
	int8_t *s, carry, *p, *q;

	assert(/*vexpand no $*/ *w == '$');
	p = w+1;	/* skip dollar sign */
	if(*p == '{') {
		p++;
		q = utfrune(p, '}');
		if (!q)
			q = strchr(p, 0);
	} else
		q = shname(p);
	carry = *q;
	*q = 0;
	s = mygetenv(p, env);
	*q = carry;
	if (carry == '}')
		q++;
	if (s) {
		bufcpy(buf, s, strlen(s));
		free(s);
	} else 		/* copy name intact*/
		bufcpy(buf, w, q-w);
	return(q);
}

void
front(int8_t *s)
{
	int8_t *t, *q;
	int i, j;
	int8_t *flds[512];

	q = strdup(s);
	i = getfields(q, flds, nelem(flds), 0, " \t\n");
	if(i > 5){
		flds[4] = flds[i-1];
		flds[3] = "...";
		i = 5;
	}
	t = s;
	for(j = 0; j < i; j++){
		for(s = flds[j]; *s; *t++ = *s++);
		*t++ = ' ';
	}
	*t = 0;
	free(q);
}
