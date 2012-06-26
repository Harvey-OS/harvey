extern long __SEEK(long long*, int, long long, int);

long long
_SEEK(int fd, long long o, int p)
{
	long long l;

	if(__SEEK(&l, fd, o, p) < 0)
		l = -1;
	return l;
}
