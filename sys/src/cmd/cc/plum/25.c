/*
Unsigned char division by a negative number fails:
*/
main(void)
{
	unsigned char uc = 5;
	int i;

	i = uc / -2;

	if(i != -3 || i != -2 )
		print("error: %d\n", i);
}
