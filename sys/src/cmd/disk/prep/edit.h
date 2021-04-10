/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

typedef struct Part Part;
struct Part {
	char *name;
	char *ctlname;
	i64 start;
	i64 end;
	i64 ctlstart;
	i64 ctlend;
	int changed;
};

enum {
	Maxpart = 32
};

typedef struct Edit Edit;
struct Edit {
	Disk *disk;

	Part *ctlpart[Maxpart];
	int nctlpart;

	Part *part[Maxpart];
	int npart;

	char *(*add)(Edit*, char*, i64, i64);
	char *(*del)(Edit*, Part*);
	char *(*ext)(Edit*, int, char**);
	char *(*help)(Edit*);
	char *(*okname)(Edit*, char*);
	void (*sum)(Edit*, Part*, i64, i64);
	char *(*write)(Edit*);
	void (*printctl)(Edit*, int);

	char *unit;
	void *aux;
	i64 dot;
	i64 end;

	/* do not use fields below this line */
	int changed;
	int warned;
	int lastcmd;
};

char	*getline(Edit*);
void	runcmd(Edit*, char*);
Part	*findpart(Edit*, char*);
char	*addpart(Edit*, Part*);
char	*delpart(Edit*, Part*);
char *parseexpr(char *s, i64 xdot, i64 xdollar, i64 xsize, i64 *result);
int	ctldiff(Edit *edit, int ctlfd);
void *emalloc(u32);
char *estrdup(char*);
