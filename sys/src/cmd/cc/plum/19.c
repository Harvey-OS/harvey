/*
Should ignore a later "extern" if variable is already
known to be file scope.  The following should link
instead of saying 'main: undefined external: s11 in main'
*/
static	int s11 = 11;
main(void)
{
	extern int s11;

	s11 = 1;
}
