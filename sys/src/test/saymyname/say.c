#include <u.h>
#include <libc.h>
#include <stdio.h>

void
main()
{
	char nombre[25];
	printf("Your name?: ");
	scanf("%s", &nombre);
	printf("\n\nHello %s\n", nombre);
}
