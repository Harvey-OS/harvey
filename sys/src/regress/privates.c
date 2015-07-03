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

extern void **_privates;
extern int _nprivates;

int
main(void)
{
	unsigned char buf[512];
	int i, fail = 0;

	if (_privates == nil) {
		fprint(2, "_privates is nil\n");
		fail++;
	}

	if (_nprivates == 0) {
		fprint(2, "_nprivates is 0\n");
		fail++;
	}

	for (i = 0; i < _nprivates; i++) {
		_privates[i] = (void *)(0x77665544332210 + i);
	}

	memset(buf, 0, sizeof buf);

	for (i = 0; i < _nprivates; i++) {
		if (_privates[i] != (void *)(0x77665544332210 + i)){
			fprint(2, "_privates[%d] = %p\n", i, _privates[i]);
			fail++;
		}
	}

	void **p[_nprivates+1];
	for (i = 0; i < _nprivates; i++) {
		p[i] = privalloc();
		if(p[i] == nil){
			fail++;
			fprint(2, "privalloc[%d]: %p\n", i, p[i]);
		}
	}

	p[i] = privalloc();
	if(p[i] != nil){
		fail++;
		fprint(2, "privalloc[%d]: %p\n", i, p[i]);
	}

	for (i = 0; i < _nprivates; i++) {
		*(p[i]) = (void*)i;
	}
	for (i = 0; i < _nprivates; i++) {
		if(*(p[i]) != (void*)i){
			fprint(2, "p[%d] != %d\n", i, i);
			fail++;
		}
	}


	if (fail > 0) {
		print("FAIL\n");
		exits("FAIL");
	}

	print("PASS\n");
	exits("PASS");

	return 0;
}
