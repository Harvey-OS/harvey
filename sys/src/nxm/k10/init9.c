extern void startboot(char*, char**);

void
main(char* argv0)
{
	startboot(argv0, &argv0);
}
