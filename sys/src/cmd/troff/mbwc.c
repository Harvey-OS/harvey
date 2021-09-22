/* ansi wide-character to multibyte-string conversions */
#include <u.h>
#include <libc.h>

int
mbtowc(Rune *rp, char *s, int len)
{
	int bytes, copy;
	char utf[UTFmax+1];

	if(s == nil)
		return 0;
	if(len < 1)
		return -1;
	copy = len > UTFmax? UTFmax: len;
	strncpy(utf, s, copy);
	utf[copy] = '\0';
	bytes = chartorune(rp, utf);
	if (bytes == 1 && *rp == Runeerror)
		return -1;
	return bytes;
}
