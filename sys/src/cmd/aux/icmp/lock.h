/*
 *  a mutex lock
 */
typedef struct Lock	Lock;

struct Lock
{
	int	parfd[2];
};

void	lockinit(Lock*);
void	lock(Lock*);
void	unlock(Lock*);
