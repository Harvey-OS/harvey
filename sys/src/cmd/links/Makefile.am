# Makefile.am
# (c) 2002 Mikulas Patocka, Karel 'Clock' Kulhavy, Petr 'Brain' Kulhavy,
#          Martin 'PerM' Pergel

man_MANS=links.1

EXTRA_DIST=atheos.cpp $(man_MANS) BUGS TODO SITES PATCH-gpm-1.20.0-smooth-cursor mailcap.pl rebuild Unicode/*.cp Unicode/[!C]* intl/[!C]* parser/[!Cy]* graphics/*.c graphics/arrow.png graphics/gen graphics/system_font/*.png graphics/font/century_school-medium-roman-serif-vari/*.png graphics/font/century_school-bold-roman-serif-vari/*.png graphics/font/symbol-medium-roman-sans-vari/*.png graphics/font/courier-medium-roman-serif-mono/*.png graphics/font/japanese-medium-roman-sans-mono/*.png graphics/font/century_school-bold-roman-serif-vari/aliases graphics/font/century_school-medium-roman-serif-vari/aliases graphics/font/symbol-medium-roman-sans-vari/aliases doc/links_cal/*.png doc/links_cal/*.html graphics/Makefile graphics/Fontmap graphics/clip.c graphics/links.xpm graphics/makefont graphics/pdf2html graphics/spacer.png links_16x16_1.xpm links_16x16_2.xpm links_32x32.xpm

bin_PROGRAMS=links

if ATHEOS_GR
links_LDADD=atheos.o
else
endif

links_SOURCES=af_unix.c auth.c beos.c bfu.c bookmarks.c builtin.c cache.c charsets.c connect.c context.c cookies.c default.c dip.c directfb.c directfb_cursors.h dither.c dns.c drivers.c error.c file.c finger.c font_include.c framebuffer.c ftp.c gif.c html.c html_gr.c html_r.c html_tbl.c http.c https.c img.c imgcache.c ipret.c javascr.c javascript.c jpeg.c jsint.c kbd.c language.c links_icon.c listedit.c lru.c mailto.c main.c md5.c md5hl.c menu.c memory.c ns.c objreq.c os_dep.c pmshell.c png.c pomocny.c sched.c select.c session.c smb.c svgalib.c terminal.c tiff.c types.c url.c view.c view_gr.c win32.c x.c xbm.c links.h cfg.h os_dep.h os_depx.h setup.h codepage.h language.h codepage.inc entity.inc uni_7b.inc language.inc arrow.inc md5.h ns.h struct.h tree.h typy.h ipret.h javascript.h builtin.h builtin_keys.h bits.h

dist-hook:
#remove the symlinka:
	rm $(distdir)/README
	rm $(distdir)/INSTALL
	echo Links $(VERSION) > $(distdir)/README
	echo Links $(VERSION) -- How To Install > $(distdir)/INSTALL
	echo Links $(VERSION) | tr -c '\n' '-' >> $(distdir)/README
	echo Links $(VERSION) -- How To Install | tr -c '\n' '-' >> $(distdir)/INSTALL
	echo >> $(distdir)/README
	echo >> $(distdir)/INSTALL
	cat $(srcdir)/README >> $(distdir)/README
	cat $(srcdir)/INSTALL >> $(distdir)/INSTALL
	tar c $(distdir) | bzip2 -9 > links-$(VERSION).tar.bz2

CXXFLAGS=@CXXFLAGS@

atheos.o: atheos.cpp
	$(CXX) $(DEFS) $(INCLUDES) $(AM_CPPFLAGS) $(CPPFLAGS) $(AM_CXXFLAGS) $(CXXFLAGS) -c atheos.cpp

datadir = $(prefix)/@DATADIRNAME@
LIBS = @LIBS@

