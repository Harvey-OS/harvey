typedef struct Lock	Lock;
typedef struct QLock	QLock;
typedef	struct	RWlock	RWlock;

struct Lock
{
	ulong	lock;
};

struct QLock
{
	Lock;				/* for manipulations */
	uchar	locked;			/* is the lock held */
	void	*head;			/* processes waiting for the lock */
	void	*tail;
};

struct	RWlock
{
	Lock	ref;
	int	nread;
	QLock	x;		/* lock for exclusion */
	QLock	k;		/* lock for each kind of access */
};

void	lockinit(void);
void	lock(Lock*);
void	unlock(Lock*);
int	canlock(Lock*);
void	qlock(QLock*);
void	qunlock(QLock*);
int	canqlock(QLock*);
void	rlock(RWlock*);
void	runlock(RWlock*);
void	wlock(RWlock*);
void	wunlock(RWlock*);

void	sharedata(void);
