#include <u.h>
#include <libc.h>
#include <../boot/boot.h>

Method *ilmp;

/*
 *  so that authentication will work, we assume that il always
 *  is configured in with the cyclone.
 */
void
configcyc(Method *mp)
{
	USED(mp);
	for(ilmp = method; ilmp->name; ilmp++)
		if(strcmp(ilmp->name, "il") == 0)
			break;
	if(ilmp->name)
		(*ilmp->config)(ilmp);
}

int
authcyc(void)
{
	if(ilmp->name)
		return (*ilmp->auth)();
	return -1;
}

int
connectcyc(void)
{
	return open("#C/cyc", ORDWR);
}
