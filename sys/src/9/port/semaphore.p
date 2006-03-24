/*
spin -a semaphore.p
pcc -DSAFETY -DREACH -DMEMLIM'='500 -o pan pan.c
pan -i
rm pan.* pan

*/

#define N 3

bit listlock;
byte value;
bit onlist[N];
bit waiting[N];
bit sleeping[N];
bit acquired[N];

inline lock(x)
{
	atomic { x == 0; x = 1 }
}

inline unlock(x)
{
	assert x==1;
	x = 0
}

inline sleep(cond)
{
	assert !sleeping[_pid];
	assert !interrupted;
	if
	:: cond
	:: atomic { else -> sleeping[_pid] = 1 } -> 
		!sleeping[_pid]
	fi;
	if
	:: skip
	:: interrupted = 1
	fi
}

inline wakeup(id)
{
	if
	:: sleeping[id] == 1 -> sleeping[id] = 0
	:: else
	fi
}

inline semqueue()
{
	lock(listlock);
	assert !onlist[_pid];
	onlist[_pid] = 1;
	unlock(listlock)
}

inline semdequeue()
{
	lock(listlock);
	assert onlist[_pid];
	onlist[_pid] = 0;
	waiting[_pid] = 0;
	unlock(listlock)
}

inline semwakeup(n)
{
	byte i, j;

	lock(listlock);
	i = 0;
	j = n;
	do
	:: (i < N && j > 0) ->
		if
		:: onlist[i] && waiting[i] -> 
			atomic { printf("kicked %d\n", i);
			waiting[i] = 0 };
			wakeup(i);
			j--
		:: else
		fi;
		i++
	:: else -> break
	od;
	/* reset i and j to reduce state space */
	i = 0;
	j = 0;
	unlock(listlock)
}

inline semrelease(n) 
{
	atomic { value = value+n; printf("release %d\n", n); };
	semwakeup(n)
}

inline canacquire()
{
	atomic { value > 0 -> value--; };
	acquired[_pid] = 1
}

#define semawoke() !waiting[_pid]

inline semacquire(block)
{
	if
	:: atomic { canacquire() -> printf("easy acquire\n"); } -> 
		goto out
	:: else
	fi;
	if
	:: !block -> goto out
	:: else
	fi;

	semqueue();
	do
	:: skip ->
		waiting[_pid] = 1;
		if
		:: atomic { canacquire() -> printf("hard acquire\n"); } ->
			break
		:: else
		fi;
		sleep(semawoke())
		if
		:: interrupted -> 
			printf("%d interrupted\n", _pid);
			break
		:: !interrupted
		fi
	od;
	semdequeue();
	if
	:: !waiting[_pid] ->
		semwakeup(1)
	:: else
	fi;
out:
	assert (!block || interrupted || acquired[_pid]);
	assert !(interrupted && acquired[_pid]);
	assert !waiting[_pid];
	printf("%d done\n", _pid);
}

active[N] proctype acquire()
{
	bit interrupted;

	semacquire(1);
	printf("%d finished\n", _pid);
	skip
}

active proctype release()
{
	byte k;

	k = 0;
	do
	:: k < N -> 
		semrelease(1);
		k++;
	:: else -> break
	od;
	skip
}

/*
 * If this guy, the highest-numbered proc, sticks
 * around, then everyone else sticks around.  
 * This makes sure that we get a state line for
 * everyone in a proc dump.  
 */
active proctype dummy()
{
end:	0;
}
