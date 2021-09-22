/*
 * functions to skip spaces or non-spaces
 */
#include <u.h>
#include <libc.h>
#include <ctype.h>

char *
skipspace(char *s)
{
	while (isspace(*s))
		s++;
	return s;
}

char *
skipword(char *s)
{
	s = skipspace(s);
	while (*s != '\0' && !isspace(*s))
		s++;
	return s;
}

/* returns -1 or first non-space char read */
int
rdspace(int fd)
{
	int n;
	char c;

	while ((n = read(fd, &c, 1)) == 1 && isspace(c))
		;
	if (n <= 0)
		return -1;
	return (uchar)c;
}

/*
 * fill buf with a non-empty word of max bytes from fd.
 * result will be NUL-terminated.  the terminating character,
 * usually whitespace is returned; if it is not -1 and not whitespace,
 * the word was truncated and there is more to read.
 */
int
rdword(int fd, char *buf, int max)
{
	int n, i;
	char c;

	i = n = 0;
	buf[i] = '\0';

	c = rdspace(fd);
	if (c == -1)
		return -1;		/* eof */

	/* else rdspace read a byte too far past spaces */
	if (--max <= 0)
		return 0;		/* terminated by count */
	if (c > 0)
		buf[i++] = c;

	while (--max > 0 && (n = read(fd, &c, 1)) == 1 && !isspace(c)) {
		buf[i++] = c;
		c = '\0';
	}
	if (i == 0 && n <= 0)		/* eof and empty word? */
		return -1;
	if (max <= 0)
		c = 0;			/* terminated by count */
	buf[i] = '\0';
	return c;
}
