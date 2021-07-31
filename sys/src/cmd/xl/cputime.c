double
cputime(void)
{
	long t[4];
	long times(long*);
	int i;

	times(t);
	for(i=1; i<4; i++)
		t[0] += t[i];
	return t[0] / 100.;
}

long
seek(int f, long o, int p)
{
	long lseek(int, long, int);

	return lseek(f, o, p);
}

int
create(char *n, int m, long p)
{
	int creat(char*, int);

	if(m != 1)
		return -1;
	return creat(n, p);
}
