/* delimited, authenticated, encrypted connection */
enum {
	Maxmsg	= 4096,		/* messages > Maxmsg bytes are truncated */
};

typedef struct SConn SConn;
struct SConn {
	void 	*chan;
	int 	secretlen;
	int 	(*secret)(SConn*, uchar*, int);
	int 	(*read)(SConn*, uchar*, int); /* <0 if error; errmess in buffer */
	int	(*write)(SConn*, uchar*, int);
	void	(*free)(SConn*);	/* also closes file descriptor */
};

SConn *newSConn(int);			/* arg is open file descriptor */

/*
 * secret(s,b,dir) sets secret for digest, encrypt, using the secretlen
 *		bytes in b to form keys 	for the two directions;
 *	  set dir=0 in client, dir=1 in server
 */

/* error convention: write !message in-band */
void	writerr(SConn*, char*);

/*
 * returns -1 upon error, with error message in buf
 * call with buf of size Maxmsg+1
 */
int	readstr(SConn*, char*);

void	*emalloc(ulong);		/* dies on failure; clears memory */
void	*erealloc(void*, ulong);
char	*estrdup(char*);
