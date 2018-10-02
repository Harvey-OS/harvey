#include <u.h>
#include <libc.h>

// Test that the FPU exceptions are working

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

static float f = 10.0;

int
handlenote(void *ureg, char *note)
{
	if (strstr(note, "Divide-By-Zero")) {
		pass();
	}
	fail("exception raised, but not divide by zero");
	return 1;
}

void
main(void)
{
	atnotify(handlenote, 1);
	f = f / 0.0f;
	fail("divide by zero exception not raised");
}
