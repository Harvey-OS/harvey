/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#pragma	varargck	argpos	editerror	1

typedef struct Addr	Addr;
typedef struct Address	Address;
typedef struct Cmd	Cmd;
typedef struct List	List;
typedef struct String	String;

struct String
{
	int	n;		/* excludes NUL */
	Rune	*r;		/* includes NUL */
	int	nalloc;
};

struct Addr
{
	char	type;	/* # (char addr), l (line addr), / ? . $ + - , ; */
	union{
		String	*re;
		Addr	*left;		/* left side of , and ; */
	};
	uint32_t	num;
	Addr	*next;			/* or right side of , and ; */
};

struct Address
{
	Range	r;
	File	*f;
};

struct Cmd
{
	Addr	*addr;			/* address (range of text) */
	String	*re;			/* regular expression for e.g. 'x' */
	union{
		Cmd	*cmd;		/* target of x, g, {, etc. */
		String	*text;		/* text of a, c, i; rhs of s */
		Addr	*mtaddr;		/* address for m, t */
	};
	Cmd	*next;			/* pointer to next element in {} */
	short	num;
	uint16_t	flag;			/* whatever */
	uint16_t	cmdc;			/* command character; 'x' etc. */
};

extern struct cmdtab{
	uint16_t	cmdc;		/* command character */
	uint8_t	text;		/* takes a textual argument? */
	uint8_t	regexp;		/* takes a regular expression? */
	uint8_t	addr;		/* takes an address (m or t)? */
	uint8_t	defcmd;		/* default command; 0==>none */
	uint8_t	defaddr;	/* default address */
	uint8_t	count;		/* takes a count e.g. s2/// */
	char	*token;		/* takes text terminated by one of these */
	int	(*fn)(Text*, Cmd*);	/* function to call with parse tree */
}cmdtab[];

#define	INCR	25	/* delta when growing list */

struct List
{
	int	nalloc;
	int	nused;
	union{
		void	*listptr;
		void*	*ptr;
		uint8_t*	*uint8_tptr;
		String*	*stringptr;
	};
};

enum Defaddr{	/* default addresses */
	aNo,
	aDot,
	aAll,
};

int	nl_cmd(Text*, Cmd*), a_cmd(Text*, Cmd*), b_cmd(Text*, Cmd*);
int	c_cmd(Text*, Cmd*), d_cmd(Text*, Cmd*);
int	B_cmd(Text*, Cmd*), D_cmd(Text*, Cmd*), e_cmd(Text*, Cmd*);
int	f_cmd(Text*, Cmd*), g_cmd(Text*, Cmd*), i_cmd(Text*, Cmd*);
int	k_cmd(Text*, Cmd*), m_cmd(Text*, Cmd*), n_cmd(Text*, Cmd*);
int	p_cmd(Text*, Cmd*);
int	s_cmd(Text*, Cmd*), u_cmd(Text*, Cmd*), w_cmd(Text*, Cmd*);
int	x_cmd(Text*, Cmd*), X_cmd(Text*, Cmd*), pipe_cmd(Text*, Cmd*);
int	eq_cmd(Text*, Cmd*);

String	*allocstring(int);
void	freestring(String*);
String	*getregexp(int);
Addr	*newaddr(void);
Address	cmdaddress(Addr*, Address, int);
int	cmdexec(Text*, Cmd*);
void	editerror(char*, ...);
int	cmdlookup(int);
void	resetxec(void);
void	Straddc(String*, int);
