#include	<all.h>

enum
{
	MAXPLINE	= 100,
};

char*	places[] =
{
	"/lib/roads/place.local",
	"/lib/roads/place",
	0
};

/*
 * Copy and normalize place name.
 * Tab or null terminates name, blanks are ignored,
 * lower case is treated as upper case,
 * leave others as they are found.
 */
void
pcopy(char *to, char **fromp)
{
	char *from;
	int c, n;

	from = *fromp;
	for(n = MAXPLINE-2; n > 0; n--) {
		c = *from++;
		if(c == 0 || c == '\t')
			break;
		if(c == ' ')
			continue;
		if(c >= 'a' && c <= 'z')
			c += 'A' - 'a';
		*to++ = c;
	}
	*to++ = '0';
	*to = 0;
	*fromp = from;
}


int
getpl(char *aword, Loc *loc, int f)
{
	long top, bot, mid;
	char bword[MAXPLINE], linebuf[2*MAXPLINE], *p, *q;
	int c, serial, n;
	double lat, lng;

	serial = 0;
	bot = 0;		/* bot will index the first char on a line */
	top = seek(f, 0L, 2);	/* top will index a newline */
	mid = 0;
	n = 0;
	p = 0;
	while(bot < top) {
		if(!serial) {
			n = top - bot;
			if(n < sizeof(linebuf)) {
				mid = bot;
				seek(f, mid, 0);
				p = linebuf;
				n = read(f, p, n+1);
				serial = 1;
				continue;
			}
			mid = (bot + top) / 2;
			seek(f, mid, 0);
			n = read(f, linebuf, sizeof(linebuf));
			p = memchr(linebuf, '\n', n);
			if(p == 0)
				break;
			p++;
			mid += p-linebuf;
		}
		
		q = p;
		p = memchr(p, '\n', n);
		if(p == 0)
			break;
		*p++ = 0;
		if(serial) {
			mid += p-q;
			bot = mid;
			n -= p-q;
		}

		pcopy(bword, &q);
		c = strcmp(aword, bword);
		if(c > 0) {
			bot = mid;
			continue;
		}
		if(c < 0) {
			top = mid-1;
			continue;
		}

		lat = atof(q);
		q = strrchr(q, ' ');
		lng = atof(q);
		norm(loc, lat, lng);
		return 0;
	}
	return -1;
}


void
getplace(char *cmd, Loc *loc)
{
	char aword[MAXPLINE], **placep;
	int f, n;

	pcopy(aword, &cmd);

	*loc = map.here;	/* Leave alone if none found */
	for(placep = places; *placep; placep++) {
		f = open(*placep, OREAD);
		if(f >= 0) {
			n = getpl(aword, loc, f);
			close(f);
			if(n == 0)
				return;
		}
	}
	*strrchr(aword, '0') = 0;
	sprint(map.p1.line[2], "No place like '%s'", aword);
	drawpage(&map.p1);
}

void
getlatlng(char *cmd, Loc *loc)
{
	double lat, lng;
	char *p;

	while(*cmd == ' ')
		cmd++;
	lat = atof(cmd);
	p = strchr(cmd, ' ');
	if(p == 0) {
		*loc = map.here;
		sprint(map.p1.line[2], "Bad lat/lng");
		drawpage(&map.p1);
		return;
	}
	lng = atof(p);
	norm(loc, lat, lng);
}
