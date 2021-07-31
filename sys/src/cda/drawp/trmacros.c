char trmacros[] = "\
.de RT		\\\" `Rotate Text'\n\
.br\n\
.nr dn 0	\\\" clear diversion vertical size\n\
.di dV		\\\" squirrel it away\n\
\\&\\\\$1\n\
.br\n\
.di		\\\" end diversion\n\
.mk vv		\\\" mark current vertical position on page\n\
.fl		\\\" paranoia\n\
.nr zz 2u*\\\\n(.vu		\\\" margin around bitmap\n\
.\\\"		divert to bitmap in driver ('{ name dx dy margin')\n\
\\X'{ RT0 \\\\n(dl \\\\n(dn \\\\n(zz'\n\
.sp -1\n\
.nr 01 \\\\n(.u\n\
.nf\n\
.dV		\\\" set the text\n\
.if \\\\n(01 .fi\n\
.fl\n\
\\X'}'		\\\" bitmap is done\n\
.sp -1\n\
.fl\n\
.sp |\\\\n(vvu\n\
.\\\"		rotate bitmap, delete original\n\
\\X': R RT RT0'\\X': D RT0'\n\
.sp -1\n\
..\n\
";
