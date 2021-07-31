#include <u.h>
#include <libc.h>
#include <ip.h>

void
main(void)
{
	Ipifc *ifc, *list;
	int i;

	fmtinstall('I', eipconv);
	fmtinstall('M', eipconv);

	for(i = 0; i < 10; i++){
		list = readipifc("/net", nil);
		for(ifc = list; ifc; ifc = ifc->next)
			fprint(2, "ipifc %s %d %I %M %I\n", ifc->dev,
				ifc->mtu, ifc->ip, ifc->mask, ifc->net);
		fprint(2, "------\n");
	}
}
