/*
 * pANS stdio -- tmpnam
 */
#include "iolib.h"

char *tmpnam(char *s){
	static char name[]="/tmp/tn000000000000";
	char *p;
	do{
		p=name+7;
		while(*p=='9') *p++='0';
		if(*p=='\0') return NULL;
		++*p;
	}while(access(name, 0)==0);
	if(s){
		strcpy(s, name);
		return s;
	}
	return name;
}
