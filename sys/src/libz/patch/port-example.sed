# inside main (that is the last function) turn `return 0` to proper exit.
/int main(argc, argv)/,$s/return 0/print("PASS\\n");exits("PASS")/

# do not include stdio
/stdio.h/D

# use native libc printing instead of stdio functions
#	but use standard error instead of stdout, as the other regression tests
s/ printf(/ fprint(2, /g
s/ fprintf/ fprint/g
s/stderr/2/g
s/stdout/2/g

# fix main signature
s/int main(argc, argv)/void main(argc, argv)/g
s/int  main/void main/g
