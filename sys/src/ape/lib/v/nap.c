int
nap(int n)
{
	register i;

	while(n-- > 0){
		for(i = 0; i < 1000*1000*10; i++)
			;
	}
	return 0;
}
