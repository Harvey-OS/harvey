#include <stdlib.h>
#include <string.h>
#include <stdio.h>

main(int argc, char **argv)
{
	char *f, *s;
	int n;

	if(argc != 2){
		fprintf(stderr, "Usage: dirname string\n");
		exit(1);
	}
	s = argv[1];
	f = s + strlen(s) - 1;
	while(f > s && *f == '/')
		f--;
	*++f = 0;
	/* now f is after last char of string, trailing slashes removed */

	for(; f >= s; f--)
		if(*f == '/'){
			f++;
			break;
		}
	if(f < s) {
		*s = '.';
		s[1] = 0;
	} else {
		--f;
		while(f > s && *f == '/')
			f--;
		f[1] = 0;
	}

	printf("%s\n", s);
	return 0;
}
