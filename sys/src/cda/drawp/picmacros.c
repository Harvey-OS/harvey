char picmacros[] = "\
define DOWNLJ %\n\
.RT \"$1\"\n\
\"\\v'-.2m'\\h'-.2m'\\X'. RT'\" ljust %\n\
\n\
define DOWNC %\n\
.nr ww \\w'$1'/2\n\
.RT \"$1\"\n\
\"\\v'-.2m'\\v'-\\n(wwu'\\h'-.2m'\\X'. RT'\" ljust %\n\
\n\
define DOWNRJ %\n\
.nr ww \\w'$1'\n\
.RT \"$1\"\n\
\"\\v'-.2m'\\v'-\\n(wwu'\\h'-.2m'\\X'. RT'\" ljust %\n\
\n\
define pin %\n\
	circle at $1,$2\n\
%\n\
\n\
define chip %\n\
#	circle rad 50 at $2,$3\n\
#	box wid $4 ht $5 with .sw at last circle.c-($6,$7)\n\
	box wid 60 ht 60 with .c at $2,$3\n\
	box wid $4 ht $5 with .sw at last box.c-($6,$7)\n\
	if $8 then {\n\
		DOWNC($1) at last box.c\n\
	} else {\n\
		\"$1\" center at last box.c\n\
	}\n\
%\n\
";
