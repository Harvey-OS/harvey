#include <u.h>
#include <libc.h>

int nring = 32;
int niter = 65536;
int debug = 0;
char data[8192];
int i;
int id;
int intr = 0;

void
handler(void* v, char* s)
{
	print("%d: %d iterations\n", id, i);
	if(intr++ < 16)
		noted(NCONT);
	exits("too many interrupts");
}

void
main(int argc, char* argv[])
{
	int pid;

	if(notify(handler)) {
		sysfatal("notify: %r");
	}

	if(argc > 1)
		nring = strtoull(argv[1], 0, 0);

	if(argc > 2)
		niter = strtoull(argv[2], 0, 0);
	debug = argc > 3;

	print("Running with %d pipes %d times\n", nring, niter);
	int* pfd = malloc(sizeof(*pipe) * nring * 2);

	if(pfd == nil)
		sysfatal(smprint("alloc %d pipes: %r", nring * 2));

	for(i = 0; i < nring; i++)
		if(pipe(&pfd[i * 2]) < 0)
			sysfatal(
			    smprint("create pipe pair %d of %d: %r", i, nring));

	for(i = 0; debug && i < nring; i++)
		print("pfd[%d,%d]\n", pfd[i * 2], pfd[i * 2 + 1]);
	for(i = 0, id = 0; i < nring - 1; i++, id++) {
		pid = fork();
		if(pid == 0)
			break;
		if(pid < 0)
			sysfatal("fork");
	}

	if(debug)
		print("Forked. pid %d\n", getpid());
	for(i = 0; debug && i < nring; i++)
		print("pfd[%d,%d]\n", pfd[i * 2], pfd[i * 2 + 1]);
	for(i = 0; i < niter; i++) {
		if(debug)
			print("%d: write %d\n", id, pfd[(id % nring) * 2 + 1]);
		int amt = write(pfd[(id % nring) * 2 + 1], data, sizeof(data));
		if(amt < sizeof(data))
			sysfatal(smprint("%d: write only got %d of %d: %r", id,
			                 amt, sizeof(data)));
		if(debug)
			print("%d: read %d\n", id, pfd[(id % nring) * 2]);
		amt = read(pfd[(id % nring) * 2], data, sizeof(data));
		if(amt < sizeof(data))
			sysfatal(smprint("%d: read only got %d of %d: %r", id,
			                 amt, sizeof(data)));
	}
}
