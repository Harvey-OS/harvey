#pragma	lib	"libworker.a"
#pragma src "/sys/src/libworker"

typedef char* (*Worker)(void *arg, void **aux);
int	getworker(Worker work, void *arg, Channel *rc);
void	workerdebug(int);

extern int (*workerthreadcreate)(void(*)(void*), void*, uint);
