#include <alef.h>
#include <auth.h>


int
authdial()
{
	return dial("net!$auth!ticket", nil, nil, nil);
}
