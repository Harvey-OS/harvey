#include	<all.h>

void
getplace(char *cmd, Loc *loc)
{
	Biobuf bio;
	long top, bot, mid;
	char aword[500], bword[500], *p;
	int f, c, flag;
	double lat, lng;

	f = open("/lib/roads/place", OREAD);
	if(f < 0) {
		print("cant open place\n");
		return;
	}

	p = aword;
	for(;;) {
		c = *cmd++;
		if(c == 0)
			break;
		if(c == ' ' || c == '\t')
			continue;
		if(c >= 'a' && c <= 'z')
			c += 'A'-'a';
		*p++ = c;
	}
	p[0] = '0';
	p[1] = 0;

	Binit(&bio, f, OREAD);

	flag = 0;
	bot = 0;
	top = Bseek(&bio, 0L, 2);
	for(;;) {
		mid = (bot+top) / 2;
		if(flag)
			mid = bot;
		Bseek(&bio, mid, 0);
		if(!flag);
			for(;;) {
				c = Bgetc(&bio);
				mid++;
				if(c < 0 || c == '\n')
					break;
			}
		if(mid >= top) {
			if(flag)
				goto no;
			flag = 1;
			continue;
		}

		p = bword;
		for(;;) {
			c = Bgetc(&bio);
			if(c < 0 || c == '\t')
				break;
			if(c == ' ')
				continue;
			if(c >= 'a' && c <= 'z')
				c += 'A'-'a';
			*p++ = c;
		}
		p[0] = '0';
		p[1] = 0;

		c = strcmp(aword, bword);
		if(c > 0) {
			bot = mid;
			continue;
		}
		if(c < 0) {
			top = mid;
			continue;
		}
		break;
	}

	p = bword;
	for(;;) {
		c = Bgetc(&bio);
		if(c < 0 || c == '\n')
			break;
		*p++ = c;
	}
	lat = atof(bword);

	p = bword;
	while(*p == ' ')
		p++;
	p = strchr(p, ' ');
	if(p == 0)
		goto no;
	lng = atof(p);

	loc->lat = norm(lat/DEGREE(1), TWOPI);
	loc->lng = norm(-lng/DEGREE(1), TWOPI);
	loc->sinlat = sin(loc->lat);
	loc->coslat = cos(loc->lat);
	goto clf;

no:
	*strrchr(aword, '0') = 0;
	print("cant find '%s'\n", aword);

clf:
	Bclose(&bio);
	close(f);
}
