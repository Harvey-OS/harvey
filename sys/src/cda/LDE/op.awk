BEGIN {printf("struct oplist { char * name; int numb;} ops[] = {\n")}

	/[^#]define/ {printf("\"%s\", %d,\n", $3, $4)}
	/#define/ {printf("\"%s\", %d,\n", $2, $3)}

END {	printf(" (char *) 0, 0 };\nchar buf[30];\nchar *\ngetop(int n)\n{\n\tstruct oplist* op;\n")
	printf("\tfor(op = ops; op->numb; ++op)\n\t\tif(n == op->numb) return(op->name);\n")
	printf("\tsprintf(buf, \"%%s%%x\", \"UNKNOWN OPCODE\", n);\n")
	printf("\treturn(buf);\n}\n")}
