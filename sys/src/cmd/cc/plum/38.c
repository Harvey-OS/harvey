/*
The following generates bogus code: it prints '6 != 0',
so somehow the second arg to iequals is passed wrongly
	bug in 2c
*/
void
iequals(int i, int j)
{
	if(i != j)
		print("%d != %d\n", i, j);
}

main(void)
{
	struct fields
	{
		unsigned x1:1;
		unsigned x2:2;
		unsigned x3:3;
	} b, b2, *p = &b2;

	p->x3=7;
	b.x2 = 2;
	iequals(p->x3*=b.x2, 6);
}
