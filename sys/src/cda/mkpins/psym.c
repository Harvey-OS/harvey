#include <u.h>
#include <libc.h>
#include "mkpin.h"

struct hshtab *
lookup( char *string)
{
register struct hshtab *hp;
register char *sp;
register ihash;
int	loopcnt;

	if(strlen(string) > NCHARS - 2) {
		fprintf(stderr, "NCHARS too small\n");
		exit(1);
	}

	/*
	 * hash
	 */
	loopcnt = 0;
	for(sp=string,ihash=0; *sp; ) {
		ihash = ((ihash<<3) + *sp++)%HSHSIZ;
	}
	hp = &hshtab[ihash];


	/*
	 * lookup
	 */
	while(strcmp(string,hp->name)) {
		if( !(hp->flag & INUSE)) {
			strcpy(hp->name,string);
			break;
		}
		if(++hp >= &hshtab[HSHSIZ]) {
			hp = hshtab;
			if(loopcnt++) {
				fprintf(stderr,"hash table overflow\n");
				exit(1);
			}
		}
	}
	return(hp);
}
