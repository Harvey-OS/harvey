# Usage: awk -f checkman.awk man?/*.?
#
# Checks:
#	- .TH is first line, and has proper name section number
#	- sections are in order NAME, SYNOPSIS, DESCRIPTION, EXAMPLES,
#		FILES, SOURCE, SEE ALSO, DIAGNOSTICS, BUGS
#	- there's a manual page for each cross-referenced page

BEGIN {

#	.SH sections should come in the following order

	Weight["NAME"] = 1
	Weight["SYNOPSIS"] = 2
	Weight["DESCRIPTION"] = 4
	Weight["EXAMPLE"] = 8
	Weight["EXAMPLES"] = 16
	Weight["FILES"] = 32
	Weight["SOURCE"] = 64
	Weight["SEE ALSO"] = 128
	Weight["DIAGNOSTICS"] = 256
	Weight["SYSTEM CALLS"] = 512
	Weight["BUGS"] = 1024

	Skipdirs["X11"] = 1
	Skipdirs["ape"] = 1
	Skipdirs["aux"] = 1
	Skipdirs["aviation"] = 1
	Skipdirs["c++"] = 1
	Skipdirs["fb"] = 1
	Skipdirs["pub"] = 1
	Skipdirs["games"] = 1
	Skipdirs["gnu"] = 1
	Skipdirs["lml"] = 1
	Skipdirs["type1"] = 1
	Skipdirs["service.alt"] = 1

	Omitted["411"] = 1
	Omitted["Kill"] = 1
	Omitted["cb"] = 1
	Omitted["edmail"] = 1
	Omitted["mousereset"] = 1
	Omitted["postalias"] = 1
	Omitted["mksacfs"] = 1
	Omitted["sacfs"] = 1
	Omitted["stock"] = 1
	Omitted["eg"] = 1
	Omitted["i"] = 1
	Omitted["netlib_find"] = 1
	Omitted["uuencode"] = 1
	Omitted["uudecode"] = 1
	Omitted["P"] = 1
	Omitted["charon"] = 1
	Omitted["tcp17032"] = 1
	Omitted["tcp17033"] = 1
	Omitted["tcp666"] = 1
	Omitted["tcp667"] = 1
	Omitted["tcp7330"] = 1
	Omitted["tcp22"] = 1
	Omitted["tcp79"] = 1
	Omitted["tcp1723"] = 1
	Omitted["pump"] = 1
	Omitted["allmail"] = 1

	Omittedlib["brk_"] = 1
	Omittedlib["creadimage"] = 1
	Omittedlib["main"] = 1
	Omittedlib["opasstokey"] = 1
	Omittedlib["oseek"] = 1
	Omittedlib["sysr1"] = 1
}

FNR==1	{
		n = length(FILENAME)
		seclen = 0
		if (substr(FILENAME, 2, 1) == "/")
			seclen = 1
		else if (substr(FILENAME, 3, 1) == "/")
			seclen = 2
		if(seclen == 0)
			print "FILENAME", FILENAME, "not of form [0-9][0-9]?/*"
		else if(!(substr(FILENAME, seclen+2, n-seclen-1) ~ /^[A-Z]+(.html)?$/)){
			section = substr(FILENAME, 1, seclen)
			name = substr(FILENAME, seclen+2, n-seclen-1)
			if($1 != ".TH" || NF != 3)
				print "First line of", FILENAME, "not a proper .TH"
			else if($2 != toupper(name) || substr($3, 1, seclen) != section){
				if($2!="INTRO" || name!="0intro")
					print ".TH of", FILENAME, "doesn't match filename"
			}else
				Pages[section "/" $2] = 1
		}
		Sh = 0
	}

$1 == ".SH" {
		if(inex)
			print "Unterminated .EX in", FILENAME, ":", $0
		inex = 0;
		if (substr($2, 1, 1) == "\"") {
			if (NF == 2) {
				print "Unneeded quote in", FILENAME, ":", $0
				$2 = substr($2, 2, length($2)-2)
			} else if (NF == 3) {
				$2 = substr($2, 2) substr($3, 1, length($3)-1)
				NF = 2
			}
		}
		if(Sh == 0 && $2 != "NAME")
			print FILENAME, "has no .SH NAME"
		w = Weight[$2]
		if (w) {
			if (w < Sh)
				print "Heading", $2, "out of order in", FILENAME
			Sh += w
		}
}

$1 == ".EX" {
		if(inex)
			print "Nested .EX in", FILENAME, ":", $0
		inex = 1
}

$1 == ".EE" {
		if(!inex)
			print "Bad .EE in", FILENAME, ":", $0
		inex = 0;
}

$1 == ".TF" {
		smallspace = 1
}

$1 == ".PD" || $1 == ".SH" || $1 == ".SS" || $1 == ".TH" {
		smallspace = 0
}

$1 == ".RE" {
		lastre = 1
}

$1 == ".PP" {
		if(smallspace && !lastre)
			print "Possible missing .PD at " FILENAME ":" FNR
		smallspace = 0
}

$1 != ".RE" {
		lastre = 0
}

$0 ~ /^\.[A-Z].*\([1-9]\)/ {
		if ($1 == ".IR" && $3 ~ /\([0-9]\)/) {
			name = $2
			section = $3
		}else if ($1 == ".RI" && $2 == "(" && $4 ~ /\([0-9]\)/) {
			name = $3
			section = $4
		}else if ($1 == ".IR" && $3 ~ /9.\([0-9]\)/) {
			name = $2
			section = "9"
		}else if ($1 == ".RI" && $2 == "(" && $4 ~ /9.\([0-9]\)/) {
			name = $3
			section = "9"
		} else {
			print "Possible bad cross-reference format in", FILENAME ":" FNR
			print $0
			next
		}
		gsub(/[^0-9]/, "", section)
		Refs[section "/" toupper(name)]++
}

END {
	print "Checking Cross-Referenced Pages"
	for (i in Refs) {
		if (!(i in Pages)){
			split(tolower(i), a, "/")
			print "grep -n " a[2] ".*" a[1] " ?/* # Need " tolower(i)
		}
	}
	print ""
	print "Checking commands"
	getindex("/sys/man/1")
	getindex("/sys/man/4")
	getindex("/sys/man/7")
	getindex("/sys/man/8")
	getbinlist("/386/bin")
	getbinlist("/rc/bin")
	for (i in List) {
		if (!(i in Index) && !(i in Omitted))
			print "Need", i, "(in " List[i] ")"
	}
	print ""
	for (i in List) {
		if (!(i in Index) && (i in Omitted))
			print "Omit", i, "(in " List[i] ")"
	}
	clearindex()
	clearlist()
	print ""
	print "Checking libraries"
	getindex("/sys/man/2")
	getnmlist("/386/lib/lib9p.a")
	getnmlist("/386/lib/libauth.a")
	getnmlist("/386/lib/libauthsrv.a")
	getnmlist("/386/lib/libbin.a")
	getnmlist("/386/lib/libbio.a")
	getnmlist("/386/lib/libc.a")
	getnmlist("/386/lib/libcontrol.a")
	getnmlist("/386/lib/libdisk.a")
	getnmlist("/386/lib/libdraw.a")
	getnmlist("/386/lib/libflate.a")
	getnmlist("/386/lib/libframe.a")
	getnmlist("/386/lib/libgeometry.a")
	getnmlist("/386/lib/libhtml.a")
	getnmlist("/386/lib/libhttpd.a")
	getnmlist("/386/lib/libip.a")
	getnmlist("/386/lib/libmach.a")
	getnmlist("/386/lib/libmemdraw.a")
	getnmlist("/386/lib/libmemlayer.a")
	getnmlist("/386/lib/libmp.a")
	getnmlist("/386/lib/libndb.a")
	getnmlist("/386/lib/libplumb.a")
	getnmlist("/386/lib/libregexp.a")
	getnmlist("/386/lib/libsec.a")
	getnmlist("/386/lib/libstdio.a")
	getnmlist("/386/lib/libString.a")
	getnmlist("/386/lib/libthread.a")
	for (i in List) {
		if (!(i in Index) && !(i in Omittedlib))
			print "Need", i, "(in " List[i] ")"
	}
	print ""
	for (i in List) {
		if (!(i in Index) && (i in Omittedlib))
			print "Omit", i, "(in " List[i] ")"
	}
}

func getindex(dir,    fname)
{
	fname = dir "/INDEX"
	while ((getline < fname) > 0)
		Index[$1] = dir
	close(fname)
}

func getbinlist(dir,    cmd, subdirs, nsd)
{
	cmd = "ls -p -l " dir
	nsd = 0
	while (cmd | getline) {
		if ($1 ~ /^d/) {
			if (!($10 in Skipdirs))
				subdirs[++nsd] = $10
		} else if ($10 !~ "^_")
			List[$10] = dir
	}
	for ( ; nsd > 0 ; nsd--)
		getbinlist(dir "/" subdirs[nsd])
	close(cmd)
}

func getnmlist(lib,    cmd)
{
	cmd = "nm -g -h " lib
	while (cmd | getline) {
		if (($1 == "T" || $1 == "L") && $2 !~ "^_")
			List[$2] = lib
	}
	close(cmd)
}

func clearindex(    i)
{
	for (i in Index)
		delete Index[i]
}

func clearlist(    i)
{
	for (i in List)
		delete List[i]
}
