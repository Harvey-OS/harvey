#include <u.h>
#include <libc.h>

#ifdef PTHREAD

static pthread_mutex_t initmutex = PTHREAD_MUTEX_INITIALIZER;

static void
lockinit(Lock *lk)
{
	pthread_mutexattr_t attr;

	pthread_mutex_lock(&initmutex);
	if(lk->init == 0){
		pthread_mutexattr_init(&attr);
		pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_NORMAL);
		pthread_mutex_init(&lk->mutex, &attr);
		pthread_mutexattr_destroy(&attr);
		lk->init = 1;
	}
	pthread_mutex_unlock(&initmutex);
}

void
lock(Lock *lk)
{
	if(!lk->init)
		lockinit(lk);
	if(pthread_mutex_lock(&lk->mutex) != 0)
		abort();
}

int
canlock(Lock *lk)
{
	int r;

	if(!lk->init)
		lockinit(lk);
	r = pthread_mutex_trylock(&lk->mutex);
	if(r == 0)
		return 1;
	if(r == EBUSY)
		return 0;
	abort();
}

void
unlock(Lock *lk)
{
	if(pthread_mutex_unlock(&lk->mutex) != 0)
		abort();
}

#else 

/* old, non-pthread systems */

int
canlock(Lock *lk)
{
	return !tas(&lk->key);
}

void
lock(Lock *lk)
{
	int i;

	/* easy case */
	if(canlock(lk))
		return;

	/* for multi processor machines */
	for(i=0; i<100; i++)
		if(canlock(lk))
			return;

	for(i=0; i<100; i++) {
		osyield();
		if(canlock(lk))
			return;
	}

	/* looking bad - make sure it is not a priority problem */
	for(i=0; i<12; i++) {
		osmsleep(1<<i);
		if(canlock(lk))
			return;
	}

	/* we are in trouble */
	for(;;) {
		if(canlock(lk))
			return;
		iprint("lock loop %ld: val=%d &lock=%ux pc=%p\n", getpid(), lk->key, lk, getcallerpc(&lk));
		osmsleep(1000);
	}
}

void
unlock(Lock *lk)
{
	assert(lk->key);
	lk->key = 0;
}

#endif

void
ilock(Lock *lk)
{
	lock(lk);
}

void
iunlock(Lock *lk)
{
	unlock(lk);
}
