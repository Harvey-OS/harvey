/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */


enum{
	Dump		= 0,		/* Actions must be in order of descending importance */
	HoldHeader,
	Hold,
	SaveLine,
	Lineoff,			/* Lineoff must be the last action code */
	Nactions,

	Nhash		= 128,

	regexp		= 1,		/* types: literal string or regular expression */
	string		= 2,

	MaxHtml		= 256,
	Hdrsize		= 4096,
	Bodysize	= 8192,
	Maxread		= 64*1024,
};

typedef struct spat 	Spat;
typedef struct pattern	Pattern;
typedef	struct patterns	Patterns;
struct	spat
{
	char*	string;
	int	len;
	int	c1;
	Spat*	next;
	Spat*	alt;
};

struct	pattern{
	struct	pattern *next;
	int	action;
	int	type;
	Spat*	alt;
	union{
		Reprog*	pat;
		Spat*	spat[Nhash];
	};
};

struct	patterns {
	char	*action;
	Pattern	*strings;
	Pattern	*regexps;
};

extern	int	debug;
extern	Patterns patterns[];
extern	char	header[];
extern	char	cmd[];

extern	void	conv64(char*, char*, char*, int);
extern	int	convert(char*, char*, char*, int, int);
extern	void*	Malloc(long n);
extern	int	matchpat(Pattern*, char*, Resub*);
extern	char*	readmsg(Biobuf*, int*, int*);
extern	void	parsepats(Biobuf*);
extern	void*	Realloc(void*, ulong);
extern	void	xprint(int, char*, Resub*);
