#include <u.h>
#include <libc.h>

static char x[1024];
static char s[64] = "  ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";

static void
fill(void)
{
	int i;

	for(i = 0; i < sizeof(x); i += sizeof(s)){
		memmove(&x[i], s, sizeof(s));
		x[i] = i>>8;
		x[i+1] = i;
	}
}

void
main(int argc, char *argv[])
{
	int i = 2560;

	if(argc > 1){
		argc--; argv++;
		i = atoi(*argv);
	}
	USED(argc);

	fill();

	while(i--)
		write(1, x, sizeof(x));
	exits(0);
}
