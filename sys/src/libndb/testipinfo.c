#include <u.h>
#include <libc.h>
#include <bio.h>
#include <ndb.h>
#include <ip.h>

void
main(void)
{
	Biobuf	in;
	char	*p;
	Ipinfo	ii;
	Ndb *db;

	fmtinstall('E', eipconv);
	fmtinstall('I', eipconv);

	db = ndbopen(0);
	Binit(&in, 0, OREAD);
	for(;;){
		print("> ");
		p = Brdline(&in, '\n');
		if(p == 0)
			exits(0);
		p[Blinelen(&in)-1] = 0;
		if(ipinfo(db, p, 0, 0, &ii) < 0)
			continue;
		print("%s ether=%E ip=%I\n\tmask=%I net=%I bootf=%s\n",
			ii.domain, ii.etheraddr, ii.ipaddr, ii.ipmask,
			ii.ipnet, ii.bootf);
		print("\tipgw=%I fs=%I au=%I\n", ii.gwip, ii.fsip, ii.auip);
	}
}
