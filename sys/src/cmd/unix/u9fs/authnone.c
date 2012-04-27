#include <plan9.h>
#include <fcall.h>
#include <u9fs.h>

static char*
noneauth(Fcall *rx, Fcall *tx)
{
	USED(rx);
	USED(tx);
	return "u9fs authnone: no authentication required";
}

static char*
noneattach(Fcall *rx, Fcall *tx)
{
	USED(rx);
	USED(tx);
	return nil;
}

Auth authnone = {
	"none",
	noneauth,
	noneattach,
};
