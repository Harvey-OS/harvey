#pragma	src	"/sys/src/liblock"
#pragma	lib	"liblock.a"

typedef struct
{
	int	val;
} Lock;

extern	void	lockinit(void);
extern	void	lock(Lock*);
extern	void	unlock(Lock*);
extern	int	canlock(Lock*);
