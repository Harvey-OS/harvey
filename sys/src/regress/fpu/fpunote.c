#include <u.h>
#include <libc.h>

// Test that the FPU can be used in note handler

void
pass(void) {
	print("PASS\n");
	exits(nil);
}

void
fail(const char *msg) {
	print("FAIL - %s\n", msg);
	exits(msg);
}

static float correctnote = 0.0f;

int
handlenote(void *ureg, char *note)
{
	if (strstr(note, "mynote")) {
		// suicide here if no fpu support for note handlers
		correctnote = 1.2f;
		pass();
	}
	return 1;
}

void
main(void)
{
	atnotify(handlenote, 1);
	postnote(PNPROC, getpid(), "mynote");
}
