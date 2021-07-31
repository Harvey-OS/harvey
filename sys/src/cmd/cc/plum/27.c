/*
ambiguity of typedef vs identifier should be resolved in
favor of the typedef.  the following initializer should
initialize j.
	works, but doesnt allow init
*/
main(void)
{
	typedef int I;
	struct {
		const I : 2;
		int j : 2;
	} x = {1}; /* "const I" is the type of un-named field */

	if(x.j != 1)
		print("error: %d\n", x.j);
}
