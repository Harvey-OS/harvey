/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

typedef struct Bytes Bytes;
typedef struct URLwin URLwin;

enum
{
	STACK		= 8192,
	EVENTSIZE	= 256,
};

struct Bytes
{
	uchar	*b;
	long		n;
	long		nalloc;
};

struct URLwin
{
	int		infd;
	int		outfd;
	int		type;

	char		*url;
	Item		*items;
	Docinfo	*docinfo;
};

extern	char*	url;
extern	int		aflag;
extern	int		width;
extern	int		defcharset;

extern	char*	loadhtml(int);

extern	char*	readfile(char*, char*, int*);
extern	int	charset(char*);
extern	void*	emalloc(ulong);
extern	char*	estrdup(char*);
extern	char*	estrstrdup(char*, char*);
extern	char*	egrow(char*, char*, char*);
extern	char*	eappend(char*, char*, char*);
extern	void		error(char*, ...);

extern	void		growbytes(Bytes*, char*, long);

extern	void		rendertext(URLwin*, Bytes*);
extern	void		rerender(URLwin*);
extern	void		freeurlwin(URLwin*);

#pragma	varargck	argpos	error	1
