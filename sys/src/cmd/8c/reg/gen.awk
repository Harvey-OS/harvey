#!/bin/awk -f

BEGIN	{
		count = global = main = lhs = op = op0 = op1 = 0;
	}
function undent(s) {
		match(s, "[^ \t].*");
		return substr(s, RSTART, RLENGTH);
	}
/^[ \t]*#/ {
		next;
	}
/^\$global/ {
		a = b = c = d;
		global = main = lhs = op = op0 = op1 = 0;
		count++;
		print "#include <u.h>";
		print "#include <libc.h>";
		print "";
		global = 1;
		next;
	}
/^\$main/ {
		global = main = lhs = op = op0 = op1 = 0;
		count++;
		print "";
		print "void";
		print "main(void)";
		print "{";
		main = 1;
		next;
	}
/^\$lhs/ {
		global = main = lhs = op = op0 = op1 = 0;
		count++;
		lhs = 1;
		next;
	}
/^\$op0/ {
		global = main = lhs = op = op0 = op1 = 0;
		count++;
		op0 = 1;
		next;
	}
/^\$op1/ {
		global = main = lhs = op = op0 = op1 = 0;
		count++;
		op1 = 1;
		next;
	}
/^\$op/ {
		global = main = lhs = op = op0 = op1 = 0;
		count++;
		op = 1;
		next;
	}
/^[ \t]+#/ {
		next;
	}
global	{
		print undent($0);
		next;
	}
main	{
		print;
		next;
	}
lhs	{
		lhss[undent($0)] = 0;
		next;
	}
op	{
		ops[undent($0)] = 0;
		next;
	}
op0	{
		op0s[undent($0)] = 0;
		next;
	}
op1	{
		op1s[undent($0)] = 0;
		next;
	}
	{
		print "#error " $0;
		exit 1;
	}
END	{
		if(count != 6) {
			print "#error missing section";
			exit 1;
		}
		print "";
		for(l in lhss) for(o in ops) for(a in op0s) for(b in op1s) {
			s = l " = " a " " o " " b ";";
			print "\t" s;
			gsub("%", "%%", s);
			print "\tprint(\"" s "\\t%.llux\\n\", (uvlong)(" l "));"
		}
		print "}";
	}
