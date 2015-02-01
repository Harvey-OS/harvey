/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

typedef struct Stringtab	Stringtab;
struct Stringtab {
	Stringtab *link;
	Stringtab *hash;
	char *str;
	int n;
	int count;
	int date;
};

typedef struct Hash Hash;
struct Hash
{
	int sorted;
	Stringtab **stab;
	int nstab;
	int ntab;
	Stringtab *all;
};

Stringtab *findstab(Hash*, char*, int, int);
Stringtab *sortstab(Hash*);

int Bwritehash(Biobuf*, Hash*);	/* destroys hash */
void Breadhash(Biobuf*, Hash*, int);
void freehash(Hash*);
Biobuf *Bopenlock(char*, int);
