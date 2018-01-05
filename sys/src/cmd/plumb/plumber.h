/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

typedef struct Exec Exec;
typedef struct Rule Rule;
typedef struct Ruleset Ruleset;

/*
 * Object
 */
enum
{
	OArg,
	OAttr,
	OData,
	ODst,
	OPlumb,
	OSrc,
	OType,
	OWdir,
};

/*
 * Verbs
 */
enum
{
	VAdd,	/* apply to OAttr only */
	VClient,
	VDelete,	/* apply to OAttr only */
	VIs,
	VIsdir,
	VIsfile,
	VMatches,
	VSet,
	VStart,
	VTo,
};

struct Rule
{
	int	obj;
	int	verb;
	char	*arg;		/* unparsed string of all arguments */
	char	*qarg;	/* quote-processed arg string */
	Reprog	*regex;
};

struct Ruleset
{
	int	npat;
	int	nact;
	Rule	**pat;
	Rule	**act;
	char	*port;
};

struct Exec
{
	Plumbmsg	*msg;
	char			*match[10];
	int			p0;		/* begin and end of match */
	int			p1;
	int			clearclick;	/* click was expanded; remove attribute */
	int			setdata;	/* data should be set to $0 */
	int			holdforclient;	/* exec'ing client; keep message until port is opened */
	/* values of $variables */
	char			*file;
	char 			*dir;
};

void		parseerror(char*, ...);
void		error(char*, ...);
void*	emalloc(int32_t);
void*	erealloc(void*, int32_t);
char*	estrdup(char*);
Ruleset**	readrules(char*, int);
void		startfsys(void);
Exec*	matchruleset(Plumbmsg*, Ruleset*);
void		freeexec(Exec*);
char*	startup(Ruleset*, Exec*);
char*	printrules(void);
void		addport(char*);
char*	writerules(char*, int);
char*	expand(Exec*, char*, char**);
void		makeports(Ruleset*[]);
void		printinputstack(void);
int		popinput(void);

Ruleset	**rules;
char		*user;
char		*home;
jmp_buf	parsejmp;
char		*lasterror;
char		**ports;
int		nports;
