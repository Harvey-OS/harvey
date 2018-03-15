#include <u.h>
#include <libc.h>

// Test that the FPU exceptions are working

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

static float f = 10.0;

int handlenote(void *ureg, char *note)
{
	if (strstr(note, "Divide-By-Zero")) {
		pass();
	}
	fail("raised exception not divide by zero");
	return 0;
}

void
main(void)
{
	atnotify(handlenote, 1);
	f = f / 0.0f;
	fail("divide by zero exception not raised");
}
