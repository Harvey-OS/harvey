#include <alef.h>

int	countpid;

void
counter(chan(int) c)
{
	int i;

	countpid = getpid();
	for(i = 2; ; i++)
		c <-= i;
}

void
filter(int prime, chan(int) listen, chan(int) send)
{
	int i;

	for(;;)
		if((i=<-listen)%prime)
			send<-=i;
}

void
sieve(chan(int) in, chan(int) out)
{
	int prime;
	chan(int) newc;

	for(;;) {
		prime = <-in;
		out <-= prime;
		alloc newc;
		task filter(prime, in, newc);
		in = newc;
	}
}

void
main(void)
{
	byte c;
	chan(int) count, primes;

	alloc count, primes;

	proc counter(count);
	task sieve(count, primes);

	for(c = 0; c < 100; c++)
		print("%d ", <-primes);

	postnote(PNPROC, countpid, "kill");
	print("done\n");
	exits(nil);
}
