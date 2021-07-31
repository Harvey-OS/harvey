/*
addition not done as ushorts:
*/
main(void)
{
	unsigned short us;

	us = 0xffff;

	if((int)(us += 1u) != 0)
		print("error\n");
}
