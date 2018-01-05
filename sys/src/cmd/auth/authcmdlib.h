/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */


enum{
	PASSLEN		= 10,
	MAXNETCHAL	= 100000,		/* max securenet challenge */
	Maxpath		= 256,
};

#define	KEYDB		"/mnt/keys"
#define NETKEYDB	"/mnt/netkeys"
#define KEYDBBUF	(sizeof NETKEYDB)	/* enough for any keydb prefix */
#define AUTHLOG		"auth"

enum
{
	Nemail		= 10,
	Plan9		= 1,
	Securenet	= 2,
};

typedef struct
{
	char	*user;
	char	*postid;
	char	*name;
	char	*dept;
	char	*email[Nemail];
} Acctbio;

typedef struct {
	char	*keys;
	char	*msg;
	char	*who;
	Biobuf 	*b;
} Fs;

extern Fs fs[3];

void	checksum(char*, char*);
void	error(char*, ...);
void	fail(char*);
char*	findkey(char*, char*, char*);
char*	findsecret(char*, char*, char*);
int	getauthkey(char*);
int32_t	getexpiration(char *db, char *u);
void	getpass(char*, char*, int, int);
int	getsecret(int, char*);
int	keyfmt(Fmt*);
void	logfail(char*);
int	netcheck(void*, int32_t, char*);
char*	netdecimal(char*);
char*	netresp(char*, int32_t, char*);
char*	okpasswd(char*);
int	querybio(char*, char*, Acctbio*);
void	rdbio(char*, char*, Acctbio*);
int	readarg(int, char*, int);
int	readfile(char*, char*, int);
void	readln(char*, char*, int, int);
int32_t	readn(int, void*, int32_t);
char*	secureidcheck(char*, char*);
char*	setkey(char*, char*, char*);
char*	setsecret(char*, char*, char*);
int	smartcheck(void*, int32_t, char*);
void	succeed(char*);
void	wrbio(char*, Acctbio*);
int	writefile(char*, char*, int);

