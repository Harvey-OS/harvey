#include	<u.h>
#include	<libc.h>
#include	<ctype.h>
#include	<bio.h>

/* Convert from Hawaii's GMT database format as produced by
 * their pscoast program to ours
 */

char		*mapbase = "/n/other/sape/map";

int r[] = {
	20, 10, 5, 2, 1
};

char t[] = "WRIriNn";

char *n[] = {
	"Coast",
	"River",
	"River",
	"Riverlet",
	"Riverlet",
	"Border",
	"Stateline",
};

void
main()
{
	int i, lat, lon, tp, res, ofd;
	Biobuf *bin;
	char fname[128];
	char *buf;
	long la, lo, lt,ln;

	for (i = 0; i < 5; i++) {
		res = r[i];

		for (lat = -90; lat < 90; lat += res)
			for (lon = 0; lon < 360; lon += res) {
				fprint(2, "%s/%dx%d/%d/%d:%d:%d\n",
					mapbase, res, res, lon, lon, lat, res);
				ofd = -1;
				for (tp = 0; tp < 7; tp++) {
					sprint(fname, "%s/%dx%d/%d/%c:%d:%d:%d",
						mapbase, res, res, lon, t[tp], lon, lat, res);
					if ((bin = Bopen(fname, OREAD)) == nil)
						continue;
					if (ofd < 0) {
						sprint(fname, "%s/%dx%d/%d/%d:%d:%d",
							mapbase, res, res, lon, lon, lat, res);
						ofd = create(fname, OWRITE|OTRUNC, 0664);
						if (ofd < 0)
							sysfatal("create %s: %r", fname);
					}
					lt = ln = 0;
					while ((buf = Brdline(bin, '\n')) && buf[0] != '\0') {
						if (buf[0] == '#')
							continue;
						if (buf[0] == '>') {
							fprint(ofd, ">%s\n", n[tp]);
							lt = ln = 0;
						} else if (isdigit(buf[0])) {
							char *p = buf;

							lo = (int)(strtod(p, &p)*1e6);
							la = (int)(strtod(p, &p)*1e6);
							fprint(ofd, "%ld %ld\n", lo-ln, la-lt);
							ln = lo;
							lt = la;
						}
						/* else ignore */
					}
					Bterm(bin);
				}
				if (ofd >= 0) {
					close(ofd);
					ofd = -1;
					USED(ofd);
				}
			}
	}
	exits(0);
}
