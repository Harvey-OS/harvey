#include "../smtp/smtp.h"
#include "../smtp/y.tab.h"

typedef struct message message;
typedef struct Addr Addr;

struct Addr {
	String	*addr;
	Addr	*next;
};

struct message {
	message *prev;		/* doubly linked list for messages */
	message *next;
	message *extent;	/* singly linked list for range of messages */
	String	*sender;
	String	*date;
	String	*body;

	int pos;
	int size;
	int status;

	Field	*first;
	Addr	*addrs;		/* all addresses in the message */
	String	*subject;
	String	*reply_to822;
	String	*from822;
	String	*sender822;
	String	*date822;
	char	*body822;	/* first char after 822 headers */
	String	*mime;
};

/* message status */
#define DELETED 1

extern message *mlist,*mlast,*mzero;	/* list of messages */

extern String*	copyfield(message*, int);
extern message	*m_get(String*);
extern int	m_store(message*, Biobuf*);
extern int	m_print(message*, Biobuf*, int, int);
extern int	m_printcrnl(message*, Biobuf*, int);
extern int	read_mbox(char*, int);
extern int	reread_mbox(char*, int);
extern int	write_mbox(char *, int);
extern void	V(void);
extern int	P(char*);
extern int	notecatcher(void*, char*);
extern int	complain(char*, void*);

extern char	*header(message*);
extern int	whereis(message*);
extern int	prheader(message*);
extern int	delete(message*);
extern int	undelete(message*);
extern int	store(message*, char*, int, int);
extern int	remail(message*, char*, int);
extern int	printm(message*), seanprintm(message*);
extern int	pipemail(message*, char*, int);
extern int	escape(char*);
extern int	reply(message*, int);
extern int	replyall(message*, int);
extern int	help(void);

extern message *mzero;		/* zeroth message */
extern message *mlist;		/* first mail message */
extern message *mlast;		/* last mail message */

extern int fflg;
extern int iflg;
extern int writeable;
extern int Dflg;

extern Biobuf in;
extern Biobuf out;
