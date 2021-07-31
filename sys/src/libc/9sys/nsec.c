#include <u.h>
#include <libc.h>


static uvlong order = 0x0001020304050607ULL;

static void
be2vlong(vlong *to, uchar *f)
{
	uchar *t, *o;
	int i;

	t = (uchar*)to;
	o = (uchar*)&order;
	for(i = 0; i < 8; i++)
		t[o[i]] = f[i];
}

/*
 *  After a fork with fd's copied, both fd's are pointing to
 *  the same Chan structure.  Since the offset is kept in the Chan
 *  structure, the seek's and read's in the two processes can
 *  compete at moving the offset around.  Hence the retry loop.
 *
 *  Since the bintime version doesn't need a seek, it doesn't
 *  have the loop.
 */
vlong
nsec(void)
{
	char b[12+1];
	static int f = -1;
	static int usebintime;
	int retries;
	vlong t;

	if(f < 0){
		usebintime = 1;
		f = open("/dev/bintime", OREAD|OCEXEC);
		if(f < 0){
			usebintime = 0;
			f = open("/dev/nsec", OREAD|OCEXEC);
			if(f < 0)
				return 0;
		}
	}

	if(usebintime){
		if(read(f, b, sizeof(uvlong)) < 0)
			goto error;
		be2vlong(&t, (uchar*)b);
		return t;
	} else {
		for(retries = 0; retries < 100; retries++){
			if(seek(f, 0, 0) >= 0 && read(f, b, sizeof(b)-1) >= 0){
				b[sizeof(b)-1] = 0;
				return strtoll(b, 0, 0);
			}
		}
	}

error:
	close(f);
	f = -1;
	return 0;
}
