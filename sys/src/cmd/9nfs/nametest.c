#include "all.h"

void	mapinit(char*, char*);

int	debug;
int	rpcdebug;
int	style = 'u';
Biobuf *in;
Unixid *ids;
Unixid **pids;
Unixidmap *mp;

void
main(int argc, char **argv)
{
	int id, arc; char *arv[4];
	char *l, *name;

	chatty = 1;
	ARGBEGIN{
	case '9':
	case 'u':
		style = ARGC();
		break;
	case 'D':
		++debug;
		break;
	}ARGEND
	if(argc <= 0){
		ids = readunixids("/fd/0", style);
		if(ids)
			idprint(1, ids);
		exits(ids ? 0 : "readunixids");
	}
	mapinit(argv[0], 0);
	in = Bopen("/fd/0", OREAD);
	while(l = Brdline(in, '\n')){	/* assign = */
		l[Blinelen(in)-1] = 0;
		arc = strparse(l, nelem(arv), arv);
		if(arc <= 0)
			continue;
		switch(arv[0][0]){
		case 'r':
			if(arc < 2)
				continue;
			mapinit(arv[1], arv[2]);
			break;
		case 'i':
			if(arc < 2)
				continue;
			id = strtol(arv[1], 0, 10);
			name = id2name(pids, id);
			print("%d -> %s\n", id, name);
			break;
		case 'n':
			if(arc < 2)
				continue;
			name = arv[1];
			id = name2id(pids, name);
			print("%s -> %d\n", name, id);
			break;
		case 'p':
			print("server=%s, client=%s\n", mp->server, mp->client);
			break;
		case 'P':
			idprint(1, *pids);
			break;
		case 'u':
			pids = &mp->u.ids;
			print("users...\n");
			break;
		case 'g':
			pids = &mp->g.ids;
			print("groups...\n");
			break;
		}
	}
	exits(0);
}

void
mapinit(char *file, char *client)
{
	if(file){
		print("reading %s...\n", file);
		if(readunixidmaps(file) < 0)
			exits("readunixidmaps");
		if(!client)
		client = "nslocum.research.bell-labs.com";
	}
	print("client = %s...\n", client);
	mp = pair2idmap("bootes", client, 0);
	if(mp == 0){
		fprint(2, "%s: pair2idmap failed\n", argv0);
		exits("pair2idmap");
	}
	pids = &mp->u.ids;
	print("[users...]\n");
}
