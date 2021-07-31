/* Copyright 1990, AT&T Bell Labs */
#include <u.h>
#include <libc.h>
#include <stdio.h>

/* A record header is immediately followed by its data,
   dlen bytes long.  According to variable "keyed", a
   key of len klen follows, or is superposed on, the data.
   All comparisons are strictly lexicographic. */

typedef	struct	rec	Rec;
typedef	struct	list	List;
typedef	struct	pos	Pos;
typedef	struct	field	Field;

struct	rec
{
	Rec*	next;
	short	len[2];		/* lengths of data and key strings */
	uchar	content[1];	/* variable-length */
};

#define	dlen	len[0]		/* (virtual field) length of data string */
#define	klen	len[1]		/* (virtual field) length of key string */

#define	succ(r)	(Rec*)((r)->content + ((r)->dlen \
		+ (keyed?(r)->klen:0) + sizeof(r) - 1)/sizeof(r)*sizeof(r))

#define	key(r)	((r)->content+(r)->dlen)	/* ptr to key */
#define	data(r)	(r)->content			/* ptr to data */

struct	list
{
	Rec*	head;		/* pointer to first record */
	Rec*	tail;		/* pointer to last record */
};

struct	pos
{
	short	fieldno;
	short	charno;
};
	
	/* describes a field of the key
           coder(dataptr, keyptr, len, fieldptr) encodes
           data of specified length into a key, observing
           parameters of the field */

struct	field
{
	int	(*coder)(uchar*, uchar*, int, Field*);
	uchar*	trans;		/* translation table */
	uchar*	keep;		/* deletion table */
	uchar	rflag;		/* sort in reverse order */
	uchar	bflag;		/* skip initial blanks */
	uchar	eflag;		/* bflag on end posn */
	uchar	lflag;		/* this is the last field */
	Pos	begin;		/* where the key begins */
	Pos	end;		/* where it ends */
};

extern	List*	stack;
extern	List*	stackmax;
extern	Rec*	buffer;
extern	uchar*	bufmax;

extern	Field	fields[];
extern	int	nfields;

extern	char*	oname;		/* output file name */
extern	char*	tname[];	/* possible temporary directories */
extern	int	firstfile;	/* temp files in use */
extern	int	nextfile;

extern	int	keyed;		/* key is separate from data */
extern	int	simplekeyed;	/* key not separate but not trivial */
extern	int	cflag;
extern	int	mflag;
extern	int	uflag;
extern	int	rflag;
extern	int	signedrflag;	/* 1 or -1 for fields[0].rflag 0 or 1*/

				/* translation and deletion tables */
extern	uchar	ident[];
extern	uchar	fold[];
extern	uchar	all[];
extern	uchar	dict[];
extern	uchar	ascii[];

	/* key-making functions and room function, which provides
	   a bound on how much space the key will take given the
           length of the data.  Room for Mcode is 1,
	   for tcode 2*len+1, for ncode (len+5)/2.
           Virtual function would be nice here */
   
extern	int	tcode(uchar*, uchar*, int, Field*);
extern	int	Mcode(uchar*, uchar*, int, Field*);
extern	int	ncode(uchar*, uchar*, int, Field*);
#define room(len) (2*(len)+2)

extern	int	getline(Rec*, uchar*, FILE*);
extern	void	printout(Rec*, FILE*, char*);
extern	void	sort(List*, int);
extern	void	mergetemp(void);
extern	void	merge(int, char**);
extern	void	mergeinit(void);
extern	int	check(char*);
extern	int	fieldarg(char*, char*);
extern	int	fieldcode(uchar*, uchar*, int, uchar*);
extern	void	fieldwrapup(void);
extern	void	tabinit(void);
extern	void	tabfree(void);
extern	void	optiony(char*);
extern	void	warn(char*, char*, int);
extern	void	fatal(char*, char*, int);
extern	FILE*	fileopen(char*, char*);
extern	void	fileclose(FILE*, char*);
extern	char*	filename(int);
extern	void	cleanfiles(int);
extern	int	overwrite(int, char**);

