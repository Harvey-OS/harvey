#include <alef.h>

/* find returns a pointer to the location of string str2 in string str1,
 * if it exists.  Otherwise, it points to the end of str1.
 */
byte *
find(byte *str1, byte *str2) {
	byte *s1, *s2;

	for (; *str1!='\0'; str1++) {
		s1=str1;
		s2=str2;
		for (; *s2!='\0' && *s1==*s2;) {
			s1++;
			s2++;
		}
		if ( *s2 == '\0' )
	    		break;
	}

	return(str1);
}

/* read into buf characters until either the buffer is filled
 * or a newline is reached.  The number of characters read
 * is returned.
 */
int
getline(int fd, byte *buf, int len) {
	byte *bp, c;
	int i, n;

	i = 0;
	bp = buf;
	while ((n=read(fd, bp, 1)) == 1) {
		if (*bp == '\r') continue;
		i += n;

		c = *bp++;
		if (c == '\n' || c == '\004' || i >= len-1)
			break;
	}
	if (n < 0)
		return(n);
	*bp = '\0';
	return(i);
}
