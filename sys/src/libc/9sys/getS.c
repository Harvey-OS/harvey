#include	<u.h>
#include	<libc.h>
#include 	<auth.h>
#include	<fcall.h>

/*
 *  read a message from fd and convert it.
 *  ignore 0-length messages.
 */
char *
getS(int fd, char *buf, Fcall *f, long *lp)
{
	long m, n;
	int i, j;
	char *errstr;

	errstr = "EOF";
	n = 0;
	for(i = 0; i < 3; i++){
		n = read(fd, buf, *lp);
		if(n == 0){
			fprint(2, "getS: read retry\n");
			continue;
		}
		if(n < 0)
			return "read error";
		m = convM2S(buf, f, n);
		if(m == 0){
			fprint(2, "getS: retry bad type, n=%d\n", n);
			for(j=0; j<10; j++)
				fprint(2, "%.2ux ", buf[j]&0xFF);
			fprint(2, "\n");
			errstr = "bad type";
			continue;
		}
		*lp = m;
		return 0;
	}
	*lp = n;
	return errstr;
}
