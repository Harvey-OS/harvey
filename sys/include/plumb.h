#pragma	lib	"libplumb.a"
#pragma	src	"/sys/src/libplumb"

/*
 * Message format:
 *	source application\n
 *	destination port\n
 *	working directory\n
 *	type\n
 *	attributes\n
 *	nbytes\n
 *	n bytes of data
 */

typedef struct Plumbattr Plumbattr;
typedef struct Plumbmsg Plumbmsg;

struct Plumbmsg
{
	char		*src;
	char		*dst;
	char		*wdir;
	char		*type;
	Plumbattr	*attr;
	int		ndata;
	char		*data;
};

struct Plumbattr
{
	char		*name;
	char		*value;
	Plumbattr	*next;
};

int			plumbsend(int, Plumbmsg*);
Plumbmsg*	plumbrecv(int);
char*		plumbpack(Plumbmsg*, int*);
Plumbmsg*	plumbunpack(char*, int);
Plumbmsg*	plumbunpackpartial(char*, int, int*);
char*		plumbpackattr(Plumbattr*);
Plumbattr*	plumbunpackattr(char*);
Plumbattr*	plumbaddattr(Plumbattr*, Plumbattr*);
Plumbattr*	plumbdelattr(Plumbattr*, char*);
void			plumbfree(Plumbmsg*);
char*		plumblookup(Plumbattr*, char*);
int			plumbopen(char*, int);
int			eplumb(int, char*);
