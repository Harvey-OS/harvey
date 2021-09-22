#include	<u.h>
#include	<libc.h>
#include	<bio.h>
#include	"gen.h"
#include	<httpd.h>
#include	"/sys/src/cmd/ip/httpd/httpsrv.h"
enum
{
	Side	= 50,
	Nomove	= TNULL<<12,	
};

char*	setgame(char*);
void	htmlbd(void);
void	pprint(char *fmt, ...);
void	addptrs(void);

int	mateflag;

char	egk1tab[512];
char	egk2tab[512];
int	egitab[1024];
char	eghtab[64];
char	eggtab[1024];
int	egftab[64];
Hio*	hout;

#pragma	varargck	argpos	pprint	1

#include "eg.h"
