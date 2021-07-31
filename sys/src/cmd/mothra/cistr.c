#include <u.h>
#include <libc.h>
#include <libg.h>
#include <panel.h>
#include <ctype.h>
#include "mothra.h"
int cistrcmp(char *s1, char *s2){
	int c1, c2;

	for(; *s1; s1++, s2++){
		c1 = isupper(*s1) ? tolower(*s1) : *s1;
		c2 = isupper(*s2) ? tolower(*s2) : *s2;
		if (c1 < c2) return -1;
		if (c1 > c2) return 1;
	}
	return 0;
}
int cistrncmp(char *s1, char *s2, int n){
	int c1, c2;

	for(; *s1 && n!=0; s1++, s2++, --n){
		c1 = isupper(*s1) ? tolower(*s1) : *s1;
		c2 = isupper(*s2) ? tolower(*s2) : *s2;
		if (c1 < c2) return -1;
		if (c1 > c2) return 1;
	}
	return 0;
}
