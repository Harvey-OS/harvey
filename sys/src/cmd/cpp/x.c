#define dbg(...) fprint(2, __VA_ARGS__)

void
main(void)
{
	dbg("hello");
	dbg("hello, %s", "world");
}

