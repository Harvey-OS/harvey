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

int
cexecpipe(int *p0, int *p1)
{
	/* pipe the hard way to get close on exec */
	if(bind("#|", "/mnt/temp", MREPL) < 0)
		return -1;
	//execl("/bin/ls", "ls", "-l", "/mnt/temp", 0);
	//wait();
	*p0 = open("/mnt/temp/data", ORDWR);
	*p1 = open("/mnt/temp/data1", ORDWR|OCEXEC);
	unmount(nil, "/mnt/temp");
	if(*p0<0 || *p1<0)
		return -1;
	return 0;
}

int main(int argc, char *argv[])
{
	int f1, f2, ret;
	ret = cexecpipe(&f1, &f2);
	print("result %d f1 %d f2 %d\n", ret, f1, f2);
	if (ret < 0)
		printf("FAIL\n");
	else
		printf("OK\n");

}
