/* avoid loading floating-point printing library code */
#include <u.h>
#include <libc.h>
#include <stdio.h>

/* usual print formatter */
int
_efgfmt(Fmt *)
{
	return -1;
}

/* stdio *printf */
char *
dtoa(double, int, int, int *, int *, char **)
{
	return 0;
}

void
freedtoa(char *)
{
}

int
_IO_cvt_flt(FILE *, va_list *, int, int, int, char)
{
	return 0;
}

int
_IO_cvt_E(FILE *, va_list *, int, int, int)
{
	return 0;
}

int
_IO_cvt_G(FILE *, va_list *, int, int, int)
{
	return 0;
}

int
_IO_cvt_e(FILE *, va_list *, int, int, int)
{
	return 0;
}

int
_IO_cvt_f(FILE *, va_list *, int, int, int)
{
	return 0;
}

int
_IO_cvt_g(FILE *, va_list *, int, int, int)
{
	return 0;
}
