#include <u.h>
#include <libc.h>
#include <libg.h>

char	*items[] = {
	"English",
	"ひらがな",
	"カタカナ",
	"русский",
	"Ελληνικα ",
	0
};

char *which[] = {
	"english",
	"hiragana",
	"katakana",
	"russian",
	"greek",
};

struct Menu menu = { items, 0, 0 };

void
main(int argc, char **argv)
{
	int fd, lang;
	struct Mouse mouse;

	USED(argc, argv);
	fd = open("/mnt/kanactl/data", OWRITE);
	if (fd<0)
		berror("kwindow: can't open /mnt/kanactl/data");
	binit(0, 0, 0);
	einit(Emouse);
	for (;;) {
		mouse = emouse();
		if (mouse.buttons & 02) {
			lang = menuhit(02, &mouse, &menu);
			if (lang>=0 && lang<sizeof(which)/sizeof(which[0])) {
				write(fd, which[lang], strlen(which[lang]));
				fprint(2, "%s\n", items[lang]);
			}
		}
	}
}

void
ereshaped(Rectangle r)
{
	screen.r = r;
}
