#include "../smtp/smtp.h"
#include "../smtp/y.tab.h"

typedef struct message message;

struct message {
	message *prev;		/* doubly linked list for messages */
	message *next;
	message *extent;	/* singly linked list for range of messages */
	String	*sender;
	String	*date;
	String	*body;
	String	*subject;
	Field	*first;		/* rfc822 fields */
	char	*body822;	/* first char after 822 headers */
	int pos;
	int size;
	int status;
};

/* message status */
#define DELETED 1

extern message *mlist,*mlast,*mzero;	/* list of messages */

extern String*	copyfield(message*, int);
extern message	*m_get(String*);
extern int	m_store(message*, Biobuf*);
extern int	m_print(message*, Biobuf*, int, int);
extern int	read_mbox(char*, int);
extern int	reread_mbox(char*, int);
extern int	write_mbox(char *, int);
extern void	V(void);
extern int	P(char*);
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
extern int	help(void);
extern void	catchint(void*, char*);

extern message *mzero;		/* zeroth message */
extern message *mlist;		/* first mail message */
extern message *mlast;		/* last mail message */

extern int fflg;
extern int writeable;

extern Biobuf in;
extern Biobuf out;
