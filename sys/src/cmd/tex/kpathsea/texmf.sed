s%@prefix@%/sys/lib/%g
s%@exec_prefix@%/sys/lib/%g
s%@bindir@%/sys/lib//bin%g
s%@scriptdir@%/sys/lib//bin%g
s%@libdir@%/sys/lib//lib%g
s%@datadir@%/sys/lib//share%g
s%@infodir@%/sys/lib//info%g
s%@includedir@%/sys/lib//include%g
s%@manext@%1%g
s%@mandir@%/sys/lib//man/man1%g
s%@texmf@%/sys/lib/texmf%g
s%@web2cdir@%/sys/lib/texmf/web2c%g
s%@vartexfonts@%/var/tmp/texfonts%g
s%@texinputdir@%/sys/lib/texmf/tex%g
s%@mfinputdir@%/sys/lib/texmf/metafont%g
s%@mpinputdir@%/sys/lib/texmf/metapost%g
s%@fontdir@%/sys/lib/texmf/fonts%g
s%@fmtdir@%/sys/lib/texmf/web2c%g
s%@basedir@%/sys/lib/texmf/web2c%g
s%@memdir@%/sys/lib/texmf/web2c%g
s%@texpooldir@%/sys/lib/texmf/web2c%g
s%@mfpooldir@%/sys/lib/texmf/web2c%g
s%@mppooldir@%/sys/lib/texmf/web2c%g
s%@dvips_plain_macrodir@%/sys/lib/texmf/tex/plain/dvips%g
s%@dvilj_latex2e_macrodir@%/sys/lib/texmf/tex/latex/dvilj%g
s%@dvipsdir@%/sys/lib/texmf/dvips%g
s%@psheaderdir@%/sys/lib/texmf/dvips%g
s%@default_texsizes@%300:600%g
s%/sys/lib/texmf%\$TEXMF%g
/^ *TEXMFMAIN[ =]/s%\$TEXMF%/sys/lib/texmf%
/^[% ]*TEXMFLOCAL[ =]/s%\$TEXMF%/sys/lib/texmf%
/^ *TEXMFCNF[ =]/s%@web2c@%/sys/lib/texmf/web2c%
