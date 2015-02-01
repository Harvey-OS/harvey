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
	vlong start;
	vlong end;
	vlong ctlstart;
	vlong ctlend;
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

	char *(*add)(Edit*, char*, vlong, vlong);
	char *(*del)(Edit*, Part*);
	char *(*ext)(Edit*, int, char**);
	char *(*help)(Edit*);
	char *(*okname)(Edit*, char*);
	void (*sum)(Edit*, Part*, vlong, vlong);
	char *(*write)(Edit*);
	void (*printctl)(Edit*, int);

	char *unit;
	void *aux;
	vlong dot;
	vlong end;

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
char *parseexpr(char *s, vlong xdot, vlong xdollar, vlong xsize, vlong *result);
int	ctldiff(Edit *edit, int ctlfd);
void *emalloc(ulong);
char *estrdup(char*);
