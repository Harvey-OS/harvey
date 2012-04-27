#pragma	lib	"liberror.a"
#pragma src "/sys/src/liberror"

typedef struct Error Error;

struct Error {
	int	nerr;
	int	nlabel;
	jmp_buf label[1];	/* of [stack] actually */
};

#pragma	varargck	argpos	error	1
#pragma varargck		argpos	esmprint 1

void	errinit(int stack);
void	noerror(void);
int	nerrors(void);
void	error(char* msg, ...);
#define	catcherror()	setjmp((*__ep)->label[(*__ep)->nerr++])
extern Error**	__ep;
