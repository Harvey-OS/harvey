#include	<u.h>
#include	<libc.h>
#include	<stdio.h>

main()
{
	register c;
	int tab1[256];
	int r, i;

	for(c = 0; c < 128; c++)
		tab1[c] = c;
	for(c = 128; c < 256; c++)
		tab1[c] = -1;

	while(scanf("%x %x", &c, &r) == 2){
		c &= 0xff;
		tab1[c] = r;
	}
	printf("long tabcyr1[256] =\n{\n");
	for(c = 0; c < 128; c += 16){
		for(i = 0; i < 16; i++)
			printf("0x%2.2x,", tab1[c+i]);
		printf("\n");
	}
	for(c = 128; c < 256; c += 8){
		for(i = 0; i < 8; i++)
			if(tab1[c+i] < 0)
				printf("    -1,");
			else
				printf("0x%4.4x,", tab1[c+i]);
		printf("\n");
	}
	printf("};\n");
	exits(0);
}
