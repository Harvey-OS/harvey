/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#include <u.h>
#include <libc.h>

typedef uint64_t uint64_t;

#define TWID64	((uint64_t)~(uint64_t)0)


uint64_t
unittoull(char *s)
{
	char *es;
	uint64_t n;

	if(s == nil)
		return TWID64;
	n = strtoul(s, &es, 0);
	if(*es == 'k' || *es == 'K'){
		n *= 1024;
		es++;
	}else if(*es == 'm' || *es == 'M'){
		n *= 1024*1024;
		es++;
	}else if(*es == 'g' || *es == 'G'){
		n *= 1024*1024*1024;
		es++;
	}
	if(*es != '\0')
		return TWID64;
	return n;
}

void
main(int argc, char *argv[])
{
	int fd, i;
	int n = 1000, m;
	int s = 1;
	double *t, t0, t1;
	unsigned char *buf;
	double a, d, max, min;

	m = OREAD;
	ARGBEGIN{
	case 'n':
		n = atoi(ARGF());
		break;
	case 's':
		s = unittoull(ARGF());
		if(s < 1 || s > 1024*1024)
			sysfatal("bad size");
		break;
	case 'r':
		m = OREAD;
		break;
	case 'w':
		m = OWRITE;
		break;
	}ARGEND

	fd = 0;
	if(argc == 1){
		fd = open(argv[0], m);
		if(fd < 0)
			sysfatal("could not open file: %s: %r", argv[0]);
	}

	buf = malloc(s);
	t = malloc(n*sizeof(double));

	t0 = nsec();
	for(i=0; i<n; i++){
		if(m == OREAD){
			if(pread(fd, buf, s, 0) < s)
				sysfatal("bad read: %r");
		}else{
			if(pwrite(fd, buf, s, 0) < s)
				sysfatal("bad write: %r");
		}
		t1 = nsec();
		t[i] = (t1 - t0)*1e-3;
		t0 = t1;
	}

	a = 0.;
	d = 0.;
	max = 0.;
	min = 1e12;

	for(i=0; i<n; i++){
		a += t[i];
		if(max < t[i])
			max = t[i];
		if(min > t[i])
			min = t[i];
	}

	a /= n;

	for(i=0; i<n; i++)
		d += (a - t[i]) * (a - t[i]);
	d /= n;
	d = sqrt(d);

	print("avg = %.0fµs min = %.0fµs max = %.0fµs dev = %.0fµs\n", a, min, max, d);

	exits(0);
}
