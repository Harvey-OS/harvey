extern void startboot(char*, char**);

void
_main(char *argv0)
{
	startboot(argv0, &argv0);
}
