#include <alef.h>

intern	int sconv(Printspec*);

intern	int	(*fmtconv[10])(Printspec*) =
{
	sconv,
};

