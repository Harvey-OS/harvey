#pragma	lib	"libworker.a"
#pragma	src	"/sys/src/libworker"

enum {
	Nworker = 256,
};
typedef struct Worker	Worker;
typedef struct Request	Request;

struct Request {
	void	(*func)(Worker*, void*);
	void	*arg;
};

struct Worker {
	char	name[64];
	Request	*r;		/* Pointer to work to do */
	ulong	version;	/* Incremented when accepting work */
	Channel	*chan;		/* for allocating work */
	Channel	*event;		/* for signaling waiting worker */
	void	*arg;		/* func's arg */
	void	*aux;		/* unused by library */
};

void	workerdispatch(void (*)(Worker*,void*), void*);
int	timerrecall(void*);
void*	timerdispatch(void (*)(Worker*, void*), void*, vlong);
int	recvt(Channel*, void*, vlong);
int	sendt(Channel*, void*, vlong);
