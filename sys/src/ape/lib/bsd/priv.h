/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

typedef struct Rock Rock;

enum
{
	Ctlsize=	128,

	/* states */
	Sopen=	0,
	Sbound,
	Sconnected,

	/* types of name */
	Tsys=	0,
	Tip,
	Tdom,
};

/*
 *  since BSD programs expect to perform both control and data functions
 *  through a single fd, we need to hide enough info under a rock to
 *  be able to open the control file when we need it.
 */
struct Rock
{
	Rock		*next;
	unsigned long	dev;		/* inode & dev of data file */
	unsigned long	inode;		/* ... */
	int		domain;		/* from socket call */
	int		stype;		/* ... */
	int		protocol;	/* ... */
	struct sockaddr	addr;		/* address from bind */
	int		reserved;	/* use a priveledged port # (< 1024) */
	struct sockaddr	raddr;		/* peer address */
	char		ctl[Ctlsize];	/* name of control file (if any) */
	int		other;		/* fd of the remote end for Unix domain */
};

extern Rock*	_sock_findrock(int, struct stat*);
extern Rock*	_sock_newrock(int);
extern void	_sock_srvname(char*, char*);
extern int	_sock_srv(char*, int);
extern int	_sock_data(int, char*, int, int, int, Rock**);
extern int	_sock_ipattr(char*);
extern void	_sock_ingetaddr(Rock*, struct sockaddr_in*, int*, char*);

extern void	_syserrno(void);
