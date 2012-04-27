#include <u.h>
#include <libc.h>

typedef struct Cnt Cnt;	/* debug */
struct Cnt
{
	int	sleep;
	int	zsleep;
	int	wake;
	int	zwake;
	int	uup;
	int	kup;
	int	udown;
	int	kdown;
	int	ualt;
	int	kalt;
};

#define dprint	if(semdebug)fprint

int semtrytimes = 100;
int semdebug;

static Cnt c;

static void
typesok(void)
{
	/*
	 * The C library uses long*, but the kernel is
	 * going to use int*; f*ck.
	 */
	assert(sizeof(int) == sizeof(long));
}

void
upsem(int *s)
{
	int n;

	assert(s != nil);
	typesok();
	n = ainc(s);
	dprint(2, "upsem: %#p = %d\n", s, n);
	if(n <= 0){
		ainc(&c.kup);
		semwakeup(s);
	}else
		ainc(&c.uup);
}

int
downsem(int *s, int dontblock)
{
	int n;
	int i;


	assert(s != nil);
	typesok();
	/* busy wait */
	for(i = 0; *s <= 0 && i < semtrytimes; i++)
		; // sleep(0);

	if(*s <= 0 && dontblock)
		return -1;
	n = adec(s);
	dprint(2, "downsem: %#p = %d\n", s, n);
	if(n < 0){
		ainc(&c.kdown);
		semsleep(s, dontblock);
		if(dontblock == 0)
			dprint(2, "downsem: %#p awaken\n", s);
	}else
		ainc(&c.udown);
	return 0;
}

int
altsems(int *ss[], int n)
{
	int i, w, r;

	typesok();
	assert(ss != nil);
	assert(n > 0);

	/* busy wait */
	for(w = 0; w < semtrytimes; w++){
		for(i = 0; i < n; i++)
			if(ss[i] == nil)
				sysfatal("altsems: nil sem");
			else if(*ss[i] > 0)
				break;
		if(i < n)
			break;
	}

	for(i = 0; i < n; i++)
		if(downsem(ss[i], 1) != -1){
			ainc(&c.ualt);
			return i;
		}

	ainc(&c.kalt);
	r = semalt(ss, n);
	return r;
}

void
semstats(void)
{
	print("sleep: %d\n", c.sleep);
	print("zsleep: %d\n", c.zsleep);
	print("wake: %d\n", c.wake);
	print("zwake: %d\n", c.zwake);
	print("uup: %d\n", c.uup);
	print("kup: %d\n", c.kup);
	print("udown: %d\n", c.udown);
	print("kdown: %d\n", c.kdown);
	print("ualt: %d\n", c.ualt);
	print("kalt: %d\n", c.kalt);
}

