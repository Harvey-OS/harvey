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
		else if(!(substr(FILENAME, seclen+2, n-seclen-1) ~ /^[A-Z]+$/)){
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

$0 ~ /^\..*\([0-9]\)/ {
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
			print "Possible bad cross-reference format in", FILENAME
			print $0
			next
		}
		gsub(/[^0-9]/, "", section)
		Refs[section "/" toupper(name)]++
}

END {
	print "Checking Cross-Referenced Pages"
	for (i in Refs) {
		if (!(i in Pages))
			print "Need", tolower(i)
	}
	print ""
	print "Checking commands"
	getindex("/sys/man/1")
	getindex("/sys/man/4")
	getindex("/sys/man/7")
	getindex("/sys/man/8")
	getindex("/sys/man/9")
	getindex("/sys/man/10")
	Skipdirs["X11"] = 1
	Skipdirs["ape"] = 1
	Skipdirs["aux"] = 1
	Skipdirs["c++"] = 1
	Skipdirs["help"] = 1
	Skipdirs["lbp"] = 1
	Skipdirs["m"] = 1
	Skipdirs["scsi"] = 1
	getbinlist("/mips/bin")
	getbinlist("/rc/bin")
	for (i in List) {
		if (!(i in Index))
			print "Need", i, "(in " List[i] ")"
	}
	clearindex()
	clearlist()
	print ""
	print "Checking libraries"
	getindex("/sys/man/2")
	getindex9("/sys/man/9")
	getnmlist("/mips/lib/libauth.a")
	getnmlist("/mips/lib/libbio.a")
	getnmlist("/mips/lib/libc.a")
	getnmlist("/mips/lib/libfb.a")
	getnmlist("/mips/lib/libframe.a")
	getnmlist("/mips/lib/libg.a")
	getnmlist("/mips/lib/libip.a")
	getnmlist("/mips/lib/liblayer.a")
	getnmlist("/mips/lib/libmach.a")
	getnmlist("/mips/lib/libndb.a")
	getnmlist("/mips/lib/libregexp.a")
	getnmlist("/mips/lib/libstdio.a")
	getnmlist("/mips/lib/libpanel.a")
	for (i in List) {
		if (!(i in Index))
			print "Need", i, "(in " List[i] ")"
	}
}

func getindex(dir,    fname)
{
	fname = dir "/INDEX"
	while ((getline < fname) > 0)
		Index[$1] = dir
	close(fname)
}

func getindex9(dir,    fname)
{
	fname = dir "/INDEX"
	while ((getline < fname) > 0)
		if($2 ~ "(getflags|picopen|getcmap)")
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
		} else
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
