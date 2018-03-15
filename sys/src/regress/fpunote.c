#include <u.h>
#include <libc.h>

// Test that the FPU can be used in note handlers

void
pass(void) {
	print("PASS\n");
	exits("PASS");
}

void
fail(const char *msg) {
	print("FAIL - %s\n", msg);
	exits("FAIL");
}

static float f = 0;

int handlenote(void *ureg, char *note)
{
	// Use the FPU in the note handler - suicides in case of failure
	f = 1.0;
	pass();
	return 0;
}

void
main(void)
{
	atnotify(handlenote, 1);
	f = f / 0.0f;
	fail("divide by zero exception not raised");
}
